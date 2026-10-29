"""Real USB clock helper against a pseudo-terminal; never opens a hardware port."""
from pathlib import Path
import os
import pty
import select
import signal
import subprocess
import sys
import time
from datetime import datetime, timedelta, timezone
import importlib.util

root = Path(__file__).resolve().parents[2]
source = root / "tools/clock/sync_clock.py"
spec = importlib.util.spec_from_file_location("clock_sync", source)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
assert module.payload(1709164800, datetime(2024, 2, 29, tzinfo=timezone(timedelta(minutes=330)))) == b"@TIME 1709164800 330\n"
assert module.payload(1709164800, datetime(2024, 2, 29, tzinfo=timezone(timedelta(hours=-4)))) == b"@TIME 1709164800 -240\n"
master, slave = pty.openpty()
process = subprocess.Popen([sys.executable, str(source), os.ttyname(slave)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
try:
    assert select.select([process.stdout], [], [], 5)[0], "Clock helper did not start"
    assert "Clock sync active" in process.stdout.readline()
    os.write(master, b"@CLOCK REQUEST\n")
    data = bytearray()
    deadline = time.monotonic() + 8
    while b"@TIME " not in data or b"\n" not in data[data.find(b"@TIME "):]:
        assert time.monotonic() < deadline, "No sync frame received"
        if select.select([master], [], [], 0.1)[0]:
            data.extend(os.read(master, 1024))
    frame = bytes(data[data.find(b"@TIME "):]).splitlines()[0].split()
    stamp, offset = int(frame[1]), int(frame[2])
    assert abs(stamp - int(time.time())) <= 5 and -720 <= offset <= 840
    os.write(master, f"@CLOCK OK {stamp}\n".encode())
    assert select.select([process.stdout], [], [], 5)[0], "Acknowledgement was not reported"
    assert "Display clock confirmed" in process.stdout.readline()
    process.send_signal(signal.SIGINT)
    stdout, stderr = process.communicate(timeout=5)
    assert process.returncode == 0, stderr
    assert "Clock sync stopped" in stdout
finally:
    if process.poll() is None:
        process.kill()
        process.communicate()
    os.close(master)
    os.close(slave)
print("Clock helper: timezone payloads, real serial framing, acknowledgement and clean shutdown pass.")
