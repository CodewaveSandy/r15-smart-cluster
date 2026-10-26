"""Exercise flash ordering/guards with fake tools; never access a device."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

source = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='r15-vehicle-flash-') as directory:
    root = Path(directory)
    (root / 'scripts').mkdir()
    shutil.copy2(source / 'scripts/flash-vehicle.sh', root / 'scripts/flash-vehicle.sh')
    build = root / 'build/vehicle/sevensemi-s3'
    build.mkdir(parents=True)
    (build / 'firmware.factory.bin').write_bytes(b'fake image')
    pio = root / 'pio'
    pio.write_text('#!/bin/bash\necho "$*" >> "$TEST_PIO_LOG"\n')
    pio.chmod(0o755)
    esptool = root / 'esptool.py'
    esptool.write_text('''import os,sys
from pathlib import Path
args=sys.argv[1:]
with open(os.environ['TEST_FLASH_LOG'],'a') as f:f.write(' '.join(args)+'\\n')
if 'flash-id' in args:
 print('Detected flash size: '+os.environ.get('TEST_CAPACITY','8MB'))
 print('MAC: '+os.environ.get('TEST_MAC','00:11:22:33:44:55'))
if 'read-flash' in args:
 with open(args[-1],'wb') as f:f.truncate(8388608 if not os.environ.get('TEST_SHORT') else 10)
if 'verify-flash' in args and os.environ.get('TEST_VERIFY_FAIL'):sys.exit(1)
''')
    env = dict(os.environ, R15_PIO=str(pio), R15_VEHICLE_PYTHON=sys.executable,
               R15_VEHICLE_ESPTOOL=str(esptool), TEST_FLASH_LOG=str(root/'flash.log'),
               TEST_PIO_LOG=str(root/'pio.log'))
    def run(args, **extra):
        (root/'flash.log').write_text('')
        result = subprocess.run(['bash',str(root/'scripts/flash-vehicle.sh'),*args],
                                env=dict(env,**extra),capture_output=True,text=True)
        return result, (root/'flash.log').read_text()
    result, log = run(['--build-only'])
    assert result.returncode == 0 and not log, result.stderr
    result, log = run([])
    assert result.returncode == 2 and not log
    result, log = run(['--port','/dev/does-not-exist'])
    assert result.returncode != 0 and not log
    for extra in ({'TEST_CAPACITY':'4MB'}, {'TEST_MAC':'b8:f8:62:7f:2c:68'},
                  {'TEST_SHORT':'1'}, {'TEST_VERIFY_FAIL':'1'}):
        result, log = run(['--port','/dev/null'], **extra)
        assert result.returncode != 0 and 'write-flash' not in log, (result.stdout,log)
    result, log = run(['--port','/dev/null'])
    assert result.returncode == 0, result.stderr
    lines = log.splitlines()
    assert len(lines) == 5 and 'flash-id' in lines[0] and 'read-flash' in lines[1]
    assert 'verify-flash' in lines[2] and 'write-flash' in lines[3] and 'verify-flash' in lines[4]
    assert list((root/'build/vehicle/backups').glob('*/vehicle-8mb.bin.sha256'))
    assert '--after no-reset-stub' in lines[0]
    for line in lines[1:4]:
        assert '--before no-reset --after no-reset-stub' in line
    assert '--before no-reset --after hard-reset' in lines[4]
    saved = next((root/'build/vehicle/backups').glob('*/vehicle-8mb.bin.sha256'))
    backup = str(saved).removesuffix('.sha256')
    args = ['--port','/dev/null','--resume-backup',backup]
    result, log = run(args)
    assert result.returncode == 0 and 'read-flash' not in log, result.stderr
    assert len(log.splitlines()) == 4 and 'write-flash' in log
    for extra in ({'TEST_MAC':'ff:ee:dd:cc:bb:aa'}, {'TEST_VERIFY_FAIL':'1'}):
        result, log = run(args, **extra)
        assert result.returncode != 0 and 'write-flash' not in log
    Path(backup).write_bytes(b'incomplete')
    result, log = run(args)
    assert result.returncode != 0 and 'write-flash' not in log

print('Vehicle flash guards, backup verification and write ordering passed (mock tools).')
