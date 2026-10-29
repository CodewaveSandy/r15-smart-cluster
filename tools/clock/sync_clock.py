"""Keep the display's RAM clock synchronized to this Mac over USB UART0."""
import argparse
from datetime import datetime
import time


def payload(epoch=None, local=None):
    stamp = int(time.time() if epoch is None else epoch)
    local = datetime.now().astimezone() if local is None else local
    offset = int(local.utcoffset().total_seconds() // 60)
    if not 1577836800 <= stamp <= 4102444799 or not -720 <= offset <= 840:
        raise ValueError("Mac date/time or timezone is outside the supported range")
    return f"@TIME {stamp} {offset}\n".encode("ascii")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", help="Display USB serial port, e.g. /dev/cu.usbserial-10")
    args = parser.parse_args()
    import serial
    connection = serial.Serial(port=None, baudrate=115200, timeout=0.1, write_timeout=1, exclusive=True)
    connection.dtr = False
    connection.rts = False
    connection.port = args.port
    try:
        connection.open()
        print("Clock sync active. Keep running for automatic resync after reset. Ctrl+C stops.", flush=True)
        buffer = bytearray()
        next_sync = time.monotonic() + 2
        pending_epoch = None
        warned = False
        sent_at = 0
        while True:
            buffer.extend(connection.read(1024))
            request = False
            while b"\n" in buffer:
                line, _, buffer = buffer.partition(b"\n")
                line = line.strip()
                if line == b"@CLOCK REQUEST":
                    request = True
                if pending_epoch is not None and line == f"@CLOCK OK {pending_epoch}".encode():
                    print("Display clock confirmed:", datetime.now().astimezone().isoformat(timespec="seconds"), flush=True)
                    pending_epoch = None
                    warned = False
            if len(buffer) > 4096:
                buffer.clear()
            now = time.monotonic()
            if request or now >= next_sync:
                data = payload()
                connection.write(data)
                pending_epoch = int(data.split()[1])
                sent_at = now
                next_sync = now + 30
            if pending_epoch is not None and now - sent_at > 10 and not warned:
                print("No clock acknowledgement yet. Check display port/firmware; retrying automatically.", flush=True)
                warned = True
    except KeyboardInterrupt:
        print("\nClock sync stopped. A device reset or power loss requires sync again.")
    except (serial.SerialException, OSError, ValueError) as error:
        parser.exit(1, f"Clock sync failed: {error}\n")
    finally:
        connection.close()


if __name__ == "__main__":
    main()
