#!/usr/bin/env python3
"""Opt-in, LAN TLS bridge for the development phone and Mac preview."""
import argparse
import base64
import hmac
import ipaddress
import json
import os
from pathlib import Path
import secrets
import signal
import socket
import ssl
import struct
import subprocess
import tempfile
import time
from http.server import BaseHTTPRequestHandler, HTTPServer


class Session:
    """One process-local enrollment; no saved trust or phone content."""
    def __init__(self, clock=time.monotonic, saved=None, save=None, require_approval=False):
        self.require_approval = require_approval
        self.offered = False
        self.approved = False
        self.offer_deadline = 0
        self.clock = clock
        saved = saved or {}
        self._save = save or (lambda state: None)
        self.token = saved.get('token') or secrets.token_urlsafe(32)
        self.code = f'{secrets.randbelow(1000000):06d}'
        self.deadline = clock() + 300
        self.session = saved.get('session')
        self.attempts = 0
        self.revoked = False
        self.active = False
        self.paused = False
        self.last_seen = 0.0

    @property
    def connected(self):
        return self.active and self.clock() - self.last_seen < 6

    def request(self, token, operation, payload):
        if not hmac.compare_digest(token.encode('utf-8'), self.token.encode()):
            return 401, {'error': 'unauthorized'}
        if not isinstance(payload, dict) or type(payload.get('v')) is not int or payload['v'] != 1:
            return 400, {'error': 'invalid_version'}
        if operation == '/offer':
            if self.clock() >= self.deadline and self.session is None:
                return 403, {'error': 'enrollment_expired'}
            if self.session is None and self.require_approval:
                self.offered = True
                self.approved = False
                self.offer_deadline = self.clock() + 60
            return 200, {'v': 1, 'code': self.code, 'paired': self.session is not None}
        previous_session, previous_active, previous_paused = self.session, self.active, self.paused
        if operation == '/pair':
            if self.revoked or self.clock() >= self.deadline or self.attempts >= 10:
                return 403, {'error': 'enrollment_expired'}
            code = payload.get('code')
            if not isinstance(code, str) or not code.isascii() or len(code) != 6 or not code.isdigit() or not hmac.compare_digest(code, self.code):
                self.attempts += 1
                return 403, {'error': 'incorrect_code'}
            if self.require_approval and (not self.offered or not self.approved or self.clock() >= self.offer_deadline):
                return 403, {'error': 'approval_required'}
            self.attempts += 1
            if self.session is not None:
                return 409, {'error': 'already_paired'}
            self.session = secrets.token_urlsafe(24)
            self.active = True
            self.paused = False
            self.offered = False
            self.approved = False
        else:
            session = payload.get('session')
            if not isinstance(session, str) or self.session is None or not hmac.compare_digest(session.encode('utf-8'), self.session.encode()):
                return 401, {'error': 'session_expired'}
            if operation == '/resume':
                self.active = True
                self.paused = False
            elif operation == '/disconnect':
                self.active = False
                self.paused = True
            elif operation == '/forget':
                self.session = None
                self.active = False
                self.paused = False
                self.offered = False
                self.approved = False
                self.revoked = False
                self.code = f'{secrets.randbelow(1000000):06d}'
                self.deadline = self.clock() + 300
                self.attempts = 0
            elif operation != '/status':
                return 404, {'error': 'unknown_operation'}
        if operation in ('/pair', '/forget'):
            try:
                self._save({'token': self.token, 'session': self.session})
            except Exception:
                self.session, self.active, self.paused = previous_session, previous_active, previous_paused
                return 503, {'error': 'storage_unavailable'}
        self.last_seen = self.clock()
        return 200, {'v': 1, 'session': self.session, 'connected': self.connected,
                     'cluster': 'Mac preview', 'navigation': False, 'paused': self.paused}


class BridgeServer(HTTPServer):
    allow_reuse_address = True
    def get_request(self):
        connection, address = super().get_request()
        connection.settimeout(2)
        try:
            return self.tls.wrap_socket(connection, server_side=True), address
        except Exception:
            connection.close()
            raise


class Handler(BaseHTTPRequestHandler):
    # No URL, auth, body or pairing secrets in access logs.
    def log_message(self, *_args):
        pass

    def do_POST(self):
        try:
            length = self.headers.get('Content-Length', '')
            if not length.isdigit() or not 0 < int(length) <= 512 or self.headers.get('Transfer-Encoding'):
                self.send_error(413)
                return
            data = json.loads(self.rfile.read(int(length)))
            if self.path in ('/pair', '/resume', '/status') and not self.server.publish():
                status, reply = 503, {'error': 'preview_unavailable'}
            else:
                status, reply = self.server.session.request(
                    self.headers.get('Authorization', ''), self.path, data)
            self.server.publish()
            encoded = json.dumps(reply, separators=(',', ':')).encode()
            self.send_response(status)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Content-Length', str(len(encoded)))
            self.send_header('Cache-Control', 'no-store')
            self.end_headers()
            self.wfile.write(encoded)
        except (ValueError, OSError):
            self.close_connection = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', type=Path, required=True)
    parser.add_argument('--test-no-approval', action='store_true', help='Isolated test fixture only')
    parser.add_argument('--native-helper', type=Path)
    parser.add_argument('--port', type=int, default=8765)
    parser.add_argument('--host', required=True, help='Mac LAN IPv4 address, reachable from the phone')
    args = parser.parse_args()
    ipaddress.IPv4Address(args.host)
    os.umask(0o077)
    args.config.parent.mkdir(parents=True, exist_ok=True)
    # The launcher refuses a second owner; use an OS file lock as well.
    import fcntl
    lock = (args.config.parent / 'bridge.lock').open('w')
    try:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        raise SystemExit('A phone bridge is already running. Stop that preview first.')
    persisted = {}
    if args.native_helper:
        result = subprocess.run([str(args.native_helper), '--load'], check=True, capture_output=True, timeout=30)
        persisted = json.loads(result.stdout) or {}
    identity = persisted.get('identity') or secrets.token_hex(16)
    def save_trust(state):
        persisted.update(state)
        persisted['identity'] = identity
        if args.native_helper:
            subprocess.run([str(args.native_helper), '--save'], input=json.dumps(persisted).encode(), check=True, stdout=subprocess.DEVNULL, timeout=30)
    with tempfile.TemporaryDirectory(prefix='r15-phone-') as runtime:
        runtime = Path(runtime)
        key, cert = runtime / 'key.pem', runtime / 'cert.pem'
        ca_key, ca_cert = runtime / 'ca-key.pem', runtime / 'ca.pem'
        conf = runtime / 'openssl.cnf'
        conf.write_text('[req]\ndistinguished_name=dn\nprompt=no\n'
                        '[dn]\nCN=R15 Mac preview\n[server]\n'
                        f'subjectAltName=IP:{args.host}\n'
                        'basicConstraints=critical,CA:FALSE\n'
                        'keyUsage=critical,digitalSignature,keyEncipherment\n'
                        'extendedKeyUsage=serverAuth\n'
                        '[ca]\nbasicConstraints=critical,CA:TRUE\n'
                        'keyUsage=critical,keyCertSign,cRLSign\n')
        def openssl(*arguments):
            subprocess.run(['openssl', *map(str, arguments)], check=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if 'ca_key' in persisted and 'ca_cert' in persisted:
            ca_key.write_text(persisted['ca_key'])
            ca_cert.write_text(persisted['ca_cert'])
        else:
            openssl('req', '-x509', '-newkey', 'rsa:2048', '-nodes',
                    '-keyout', ca_key, '-out', ca_cert, '-days', '365',
                    '-config', conf, '-extensions', 'ca', '-subj', '/CN=R15 Preview Development CA')
            persisted.update(ca_key=ca_key.read_text(), ca_cert=ca_cert.read_text())
        csr = runtime / 'server.csr'
        openssl('req', '-new', '-newkey', 'rsa:2048', '-nodes',
                '-keyout', key, '-out', csr, '-config', conf)
        openssl('x509', '-req', '-in', csr, '-CA', ca_cert, '-CAkey', ca_key,
                '-CAcreateserial', '-out', cert, '-days', '1',
                '-extfile', conf, '-extensions', 'server')
        server = BridgeServer((args.host, args.port), Handler)
        server.timeout = 0.25
        server.tls = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        server.tls.minimum_version = ssl.TLSVersion.TLSv1_2
        server.tls.load_cert_chain(cert, key)
        server.session = Session(saved=persisted, save=save_trust, require_approval=not args.test_no_approval)
        save_trust({'token': server.session.token, 'session': server.session.session})
        status_socket = str(runtime / 'status.sock')
        control = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
        control.bind(str(runtime / 'control.sock'))
        control.setblocking(False)
        metadata_path = runtime / 'nearby.json'
        metadata_path.write_text(json.dumps({'id': identity, 'name': 'R15 Cluster', 'host': args.host, 'port': server.server_port}))
        advertiser = None
        advertiser_log = runtime / 'advertiser.log'
        if args.native_helper:
            # LaunchServices establishes the app's own TCC identity instead of
            # attributing Bluetooth access to the shell/Python parent.
            native_app = args.native_helper.parent.parent.parent
            if native_app.suffix != '.app':
                raise SystemExit('BLE discovery requires the bundled R15 Nearby.app. Restart through scripts/preview.sh.')
            advertiser = subprocess.Popen(['/usr/bin/open', '-W', '-n', '--stdout', str(advertiser_log),
                                           '--stderr', str(advertiser_log), str(native_app), '--args', str(metadata_path)])
        advertiser_offset = 0
        advertiser_exited = False
        sender = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
        sender.setblocking(False)
        def publish():
            try:
                frame = (b'R15D2:C' if server.session.connected else b'R15D2:D') + struct.pack('<Q', time.clock_gettime_ns(time.CLOCK_MONOTONIC) // 1000000) + bytes([int(server.session.session is not None) | (2 if server.session.offered and time.monotonic() < server.session.offer_deadline else 0) | (4 if server.session.approved and time.monotonic() < server.session.offer_deadline else 0)]) + server.session.code.encode('ascii')
                sender.sendto(frame, status_socket)
                return True
            except OSError:
                return False
        server.publish = publish
        config = {'R15_PREVIEW_CERT': base64.b64encode(ca_cert.read_bytes()).decode(),
                  'R15_PREVIEW_TOKEN': server.session.token,
                  'R15_PREVIEW_SOCKET': status_socket, 'R15_PREVIEW_HOST': args.host,
                  'R15_PREVIEW_ID': identity, 'R15_PREVIEW_PID': str(os.getpid()), 'R15_PREVIEW_PORT': str(server.server_port)}
        temp = args.config.with_suffix('.tmp')
        temp.write_text(json.dumps(config))
        temp.replace(args.config)
        print(f'WI-FI MAC PREVIEW — pairing code: {server.session.code} (expires in 5 minutes)', flush=True)
        print('Open Connect cluster in the app to search nearby. Confirm pairing once; saved phones reconnect automatically.', flush=True)
        stopped = False
        def stop(*_args):
            nonlocal stopped
            stopped = True
        signal.signal(signal.SIGTERM, stop)
        signal.signal(signal.SIGINT, stop)
        try:
            while not stopped:
                if advertiser:
                    if advertiser_log.exists():
                        with advertiser_log.open() as log:
                            log.seek(advertiser_offset)
                            output = log.read(4096)
                            advertiser_offset = log.tell()
                        if output:
                            print(output, end='', flush=True)
                    if advertiser.poll() is not None and not advertiser_exited:
                        advertiser_exited = True
                        print('R15 BLE advertiser exited. Nearby discovery is unavailable; check macOS Bluetooth permissions and restart the preview.', flush=True)
                server.handle_request()
                for _ in range(8):
                    try:
                        command = control.recv(2)
                    except BlockingIOError:
                        break
                    if command == b'P' and server.session.session is None:
                        server.session.deadline = time.monotonic() + 300
                        server.session.attempts = 0
                    elif command == b'A' and server.session.offered and time.monotonic() < server.session.offer_deadline:
                        server.session.approved = True
                    elif command == b'R' and server.session.offered:
                        server.session.offered = False
                        server.session.approved = False
                        server.session.code = f'{secrets.randbelow(1000000):06d}'
                    elif command == b'D':
                        server.session.active = False
                        server.session.paused = True
                    elif command == b'F' and server.session.session:
                        server.session.request(server.session.token, '/forget', {'v': 1, 'session': server.session.session})
                publish()
        finally:
            server.session.active = False
            publish()
            if advertiser:
                advertiser.terminate()
                advertiser.wait(timeout=5)
            control.close()
            sender.close()
            server.server_close()
            args.config.unlink(missing_ok=True)


if __name__ == '__main__':
    main()
