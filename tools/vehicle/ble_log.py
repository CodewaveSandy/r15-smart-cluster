"""Read only bounded @BLE diagnostics; do not send data or request a reset."""
import argparse
import time
import serial

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('port')
parser.add_argument('--seconds', type=float, default=60)
args = parser.parse_args()
if not 1 <= args.seconds <= 300:
    parser.error('--seconds must be between 1 and 300')
link = serial.Serial()
link.port, link.baudrate, link.timeout = args.port, 115200, 0.2
link.dtr = link.rts = False
link.open()
print('Listening for BLE faults. Now try Pair phone and Connect. Ctrl-C stops.', flush=True)
buffer = b''
try:
    deadline = time.monotonic() + args.seconds
    while time.monotonic() < deadline:
        buffer = (buffer + link.read(1024))[-4096:]
        while b'@BLE ' in buffer:
            start = buffer.index(b'@BLE ')
            end = buffer.find(b'\n', start)
            if end < 0:
                buffer = buffer[start:]
                break
            print(buffer[start:end].decode('ascii', errors='replace'), flush=True)
            buffer = buffer[end + 1:]
except KeyboardInterrupt:
    pass
finally:
    link.close()
