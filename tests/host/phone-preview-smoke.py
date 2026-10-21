"""Live bridge -> actual LVGL host IPC + Dart TLS client. Requires built host/Flutter."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='r15-preview-smoke-') as temp:
    config = Path(temp) / 'bootstrap.json'
    server = subprocess.Popen([sys.executable, str(root / 'tools/phone_bridge/bridge.py'),
        '--host', '127.0.0.1', '--port', '0', '--test-no-approval', '--config', str(config)], stdout=subprocess.PIPE, text=True)
    preview = None
    diagnostics = ''
    try:
        for _ in range(100):
            if config.exists(): break
            if server.poll() is not None: raise RuntimeError('Bridge startup failed')
            time.sleep(.05)
        settings = json.loads(config.read_text())
        code = server.stdout.readline().split('pairing code: ')[1].split(' ')[0]
        preview = subprocess.Popen([str(root / 'build/lvgl/r15-preview.app/Contents/MacOS/r15-preview')],
            env=dict(os.environ, SDL_VIDEODRIVER='dummy', R15_PHONE_BRIDGE_SOCKET=settings['R15_PREVIEW_SOCKET']),
            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        for _ in range(100):
            if Path(settings['R15_PREVIEW_SOCKET']).exists(): break
            if preview.poll() is not None: raise RuntimeError('LVGL preview startup failed')
            time.sleep(.05)
        subprocess.run(['flutter', 'test', '--no-pub', 'test/preview_tls_test.dart'],
            cwd=root / 'mobile', env=dict(os.environ, R15_TLS_TEST_CONFIG=str(config), R15_TLS_TEST_CODE=code),
            check=True, timeout=60)
    finally:
        if preview:
            preview.terminate()
            preview.wait(timeout=5)
            diagnostics = preview.stderr.read()
            preview.stderr.close()

        server.terminate()
        server.wait(timeout=5)
        server.stdout.close()

if diagnostics.strip():
    raise RuntimeError('Unexpected host runtime diagnostics: ' + diagnostics)
