"""Session contract and real TLS/IPC integration; no phone content or saved trust."""
import importlib.util
import json
from pathlib import Path
import socket
import ssl
import subprocess
import sys
import tempfile
import time
import unittest
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('bridge', ROOT / 'tools/phone_bridge/bridge.py')
bridge = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bridge)


class SessionTests(unittest.TestCase):
    def setUp(self):
        self.now = 100.0
        self.state = bridge.Session(lambda: self.now)

    def call(self, path, **payload):
        return self.state.request(self.state.token, path, {'v': 1, **payload})

    def test_auth_version_and_code(self):
        self.assertEqual(self.state.request('wrong', '/pair', {'v': 1})[0], 401)
        self.assertEqual(self.state.request(self.state.token, '/pair', {'v': True})[0], 400)
        self.assertEqual(self.call('/pair', code='bad')[0], 403)
        self.assertEqual(self.call('/pair', code='é')[0], 403)
        self.assertEqual(self.state.request('é', '/pair', {'v': 1})[0], 401)
        self.assertFalse(self.state.connected)
        self.assertEqual(self.call('/pair', code=self.state.code)[0], 200)
        self.assertTrue(self.state.connected)
        self.assertEqual(self.call('/pair', code=self.state.code)[0], 409)

    def test_heartbeat_disconnect_resume_forget(self):
        _, data = self.call('/pair', code=self.state.code)
        session = data['session']
        self.now += 6
        self.assertFalse(self.state.connected)
        self.assertEqual(self.call('/status', session='wrong')[0], 401)
        self.assertFalse(self.state.connected)
        self.assertTrue(self.call('/status', session=session)[1]['connected'])
        self.assertFalse(self.call('/disconnect', session=session)[1]['connected'])
        self.assertFalse(self.call('/status', session=session)[1]['connected'])
        self.assertTrue(self.call('/resume', session=session)[1]['connected'])
        self.assertIsNone(self.call('/forget', session=session)[1]['session'])
        self.assertEqual(self.call('/status', session=session)[0], 401)
        self.assertEqual(self.call('/pair', code=self.state.code)[0], 200)

    def test_expiry_attempt_limit_and_reboot(self):
        self.now += 300
        self.assertEqual(self.call('/pair', code=self.state.code)[0], 403)
        other = bridge.Session(lambda: 0)
        for _ in range(10):
            self.assertEqual(other.request(other.token, '/pair', {'v': 1, 'code': 'wrong'})[0], 403)
        self.assertEqual(other.request(other.token, '/pair', {'v': 1, 'code': other.code})[0], 403)
        self.assertNotEqual(other.token, self.state.token)

    def test_invalid_payload_and_unknown_operation(self):
        self.assertEqual(self.state.request(self.state.token, '/pair', [1])[0], 400)
        _, data = self.call('/pair', code=self.state.code)
        self.assertEqual(self.call('/unknown', session=data['session'])[0], 404)

    def test_saved_trust_restart_approval_and_revocation(self):
        saved = {}
        original = bridge.Session(save=lambda state: saved.update(state), require_approval=True)
        def request(state, path, **data):
            return state.request(state.token, path, {'v': 1, **data})
        offer = request(original, '/offer')[1]
        self.assertTrue(original.offered)
        self.assertEqual(request(original, '/pair', code=offer['code'])[1]['error'], 'approval_required')
        original.approved = True
        paired = request(original, '/pair', code=offer['code'])[1]
        restarted = bridge.Session(saved=saved, save=lambda state: saved.update(state), require_approval=True)
        self.assertFalse(restarted.connected)
        self.assertEqual(restarted.token, original.token)
        self.assertTrue(request(restarted, '/resume', session=paired['session'])[1]['connected'])
        self.assertIsNone(request(restarted, '/forget', session=paired['session'])[1]['session'])
        again = bridge.Session(saved=saved)
        self.assertIsNone(again.session)
        self.assertEqual(request(again, '/resume', session=paired['session'])[0], 401)

    def test_storage_failure_does_not_publish_saved_pair(self):
        def fail(_state): raise OSError('isolated failure')
        state = bridge.Session(save=fail)
        response = state.request(state.token, '/pair', {'v': 1, 'code': state.code})
        self.assertEqual(response[0], 503)
        self.assertIsNone(state.session)
        self.assertFalse(state.connected)


class TlsIntegration(unittest.TestCase):
    def test_real_tls_pair_and_ipc(self):
        with tempfile.TemporaryDirectory(prefix='r15-bridge-test-') as temp:
            config_path = Path(temp) / 'bootstrap.json'
            process = subprocess.Popen([sys.executable, str(ROOT / 'tools/phone_bridge/bridge.py'),
                '--config', str(config_path), '--host', '127.0.0.1', '--port', '0', '--test-no-approval'], stdout=subprocess.PIPE, text=True)
            receiver = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
            try:
                for _ in range(100):
                    if config_path.exists(): break
                    self.assertIsNone(process.poll())
                    time.sleep(.05)
                config = json.loads(config_path.read_text())
                line = process.stdout.readline()
                code = line.split('pairing code: ')[1].split(' ')[0]
                import base64
                context = ssl.create_default_context(cadata=base64.b64decode(config['R15_PREVIEW_CERT']).decode())
                def request(path, body, token=None):
                    req = Request(f"https://127.0.0.1:{config['R15_PREVIEW_PORT']}{path}",
                        data=json.dumps(body).encode(), headers={'Authorization': token or config['R15_PREVIEW_TOKEN']})
                    with urlopen(req, context=context, timeout=3) as response:
                        return json.load(response)
                with self.assertRaises(HTTPError) as unavailable:
                    request('/pair', {'v': 1, 'code': code})
                self.assertEqual(unavailable.exception.code, 503)
                receiver.bind(config['R15_PREVIEW_SOCKET'])
                receiver.settimeout(2)
                with self.assertRaises(HTTPError) as rejected:
                    request('/pair', {'v': 1, 'code': code}, token='bad')
                self.assertEqual(rejected.exception.code, 401)
                data = request('/pair', {'v': 1, 'code': code})
                self.assertTrue(data['connected'])
                for _ in range(32):
                    if receiver.recv(32)[:7] == b'R15D2:C': break
                else:
                    self.fail('Connected observation not published')
                result = request('/disconnect', {'v': 1, 'session': data['session']})
                self.assertFalse(result['connected'])
                # A mismatched certificate must never be accepted.
                with self.assertRaises(URLError) as untrusted:
                    with urlopen(Request(f"https://127.0.0.1:{config['R15_PREVIEW_PORT']}/pair", data=b'{}'), timeout=3):
                        pass
                self.assertIsInstance(untrusted.exception.reason, ssl.SSLCertVerificationError)
            finally:
                receiver.close()
                process.terminate()
                process.wait(timeout=5)
                process.stdout.close()
            self.assertFalse(config_path.exists())


if __name__ == '__main__':
    unittest.main()
