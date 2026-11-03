#!/usr/bin/env python3
"""Paired BLE bench sender for R15B v1. No synthetic GPS or vehicle control.

Install pinned bleak==1.1.1 in a local build/ venv. Pair through the normal
cluster/OS numeric comparison first; this tool never approves or replaces bonds.
"""
import argparse
import asyncio
import hashlib
from pathlib import Path
import struct
import time
import zlib

PREFIX = '813b1240-8fd9-4d67-a53b-9f0a2531'
CAP, COMMAND, STATUS = (PREFIX + suffix for suffix in ('c016', 'c017', 'c018'))


def elapsed():
    return time.monotonic_ns() // 1000000


def status(data, context):
    b = bytes(data)
    if len(b) != 80 or b[:6] != b'R15S\x01\x50' or zlib.crc32(b[:-4]) != struct.unpack_from('<I', b, 76)[0]:
        raise RuntimeError('Invalid display status')
    if b[6] > 6 or b[7] > 10 or b[68] > 7 or b[69] > 1 or b[70:72] != b'\0\0':
        raise RuntimeError('Unsupported/noncanonical display status')
    if struct.unpack_from('<III', b, 8) != context:
        raise RuntimeError('Display/vehicle/connection changed; restart from BEGIN')
    return b


async def transfer(args):
    from bleak import BleakClient, BleakScanner
    package = args.package.read_bytes()
    if not 64 <= len(package) <= 65536 or package[:8] != b'R15B\x01\x00\x40\x00':
        raise RuntimeError('Not a bounded R15B v1 package')
    body_size, _, body_crc = struct.unpack_from('<III', package, 36)
    if body_size != len(package) - 64 or zlib.crc32(package[64:]) != body_crc:
        raise RuntimeError('Package length/body CRC mismatch')
    print('Looking for the explicitly selected R15 Cluster; use normal OS pairing first.', flush=True)
    device = await BleakScanner.find_device_by_filter(
        lambda d, a: d.address == args.device and (a.local_name or d.name or '').startswith('R15 Cluster'), timeout=15)
    if device is None:
        raise RuntimeError('Selected R15 Cluster not found')
    async with BleakClient(device) as client:
        cap = bytes(await asyncio.wait_for(client.read_gatt_char(CAP), 8))
        if len(cap) != 64 or cap[:6] != b'R15K\x01\x40' or zlib.crc32(cap[:-4]) != struct.unpack_from('<I', cap, 60)[0]:
            raise RuntimeError('Invalid map capability')
        context = struct.unpack_from('<III', cap, 8)
        if cap[6] != 7 or not all(context) or len(package) > struct.unpack_from('<I', cap, 20)[0]:
            raise RuntimeError('Downstream map resources unavailable or package too large')
        # CoreBluetooth does not expose negotiated MTU; an explicit conservative
        # value (default96) avoids guessing. Use247 only after actual verification.
        mtu = args.mtu
        maximum = min(240, mtu - 3, struct.unpack_from('<H', cap, 28)[0])
        if mtu < 96 or maximum < 93:
            raise RuntimeError('MTU96 or greater required')
        latest = None
        event = asyncio.Event()

        def receive(_, data):
            nonlocal latest
            latest = (bytes(data), elapsed())
            event.set()

        await asyncio.wait_for(client.start_notify(STATUS, receive), 8)
        baseline = None
        for _ in range(20):
            raw_status = bytes(await asyncio.wait_for(client.read_gatt_char(STATUS), 8))
            # A newly secured connection must first reach the display through
            # the unchanged phone-status path; no command is issued meanwhile.
            if len(raw_status) == 80 and struct.unpack_from('<III', raw_status, 8) == context:
                baseline = status(raw_status, context)
                if baseline[69]:
                    break
            await asyncio.sleep(0.25)
        if baseline is None or not baseline[69]:
            raise RuntimeError('Display has no current-session command credit; reconnect')
        sequence = 0
        generation = (elapsed() & 0xffffffff) or 1

        async def command(kind, body=b'', gen=0, dispatch=False):
            nonlocal sequence, latest
            sequence = (sequence + 1) & 0xffffffff or 1
            if dispatch:
                body = struct.pack('<Q', elapsed())
            header = struct.pack('<4sBBBBIIIIIHH', b'R15X', 1, kind, 32, 0, *context, gen, sequence, len(body), 0)
            raw = header + body
            raw += struct.pack('<I', zlib.crc32(raw))
            if len(raw) > maximum:
                raise RuntimeError('Command exceeds verified MTU/value limit')
            latest = None
            for attempt in range(4):
                event.clear()
                await asyncio.wait_for(client.write_gatt_char(COMMAND, raw, response=True), 8)
                deadline = time.monotonic() + 2
                while time.monotonic() < deadline:
                    try:
                        await asyncio.wait_for(event.wait(), max(0.01, deadline - time.monotonic()))
                    except asyncio.TimeoutError:
                        break
                    event.clear()
                    if latest:
                        data, received = latest
                        b = status(data, context)
                        if struct.unpack_from('<I', b, 24)[0] == sequence:
                            if b[7]:
                                raise RuntimeError(f'Display rejected operation: error={b[7]}')
                            return b, received, body
                b = status(await asyncio.wait_for(client.read_gatt_char(STATUS), 8), context)
                if struct.unpack_from('<I', b, 24)[0] == sequence:
                    if b[7]:
                        raise RuntimeError(f'Display rejected operation: error={b[7]}')
                    return b, elapsed(), body
                if attempt < 3:
                    await asyncio.sleep((0.5, 1, 2)[attempt])
            raise RuntimeError('Display acknowledgement timeout; reconnect before restarting')

        async def anchor():
            _, t1, body = await command(1, dispatch=True)
            t0 = struct.unpack('<Q', body)[0]
            if t1 - t0 > 500:
                raise RuntimeError('End-to-end time anchor RTT exceeds500ms')
            await command(2, struct.pack('<IQQ', sequence, t0, t1))
            return time.monotonic()

        anchored_at = await anchor()
        digest = hashlib.sha256(package).digest()
        b, _, _ = await command(3, struct.pack('<I', len(package)) + digest, generation)
        offset = struct.unpack_from('<I', b, 28)[0]
        while offset < len(package):
            if time.monotonic() - anchored_at >= 25:
                anchored_at = await anchor()
            chunk = package[offset:offset + maximum - 40]
            b, _, _ = await command(4, struct.pack('<I', offset) + chunk, generation)
            new_offset = struct.unpack_from('<I', b, 28)[0]
            if new_offset != offset + len(chunk):
                raise RuntimeError('Unexpected display accepted offset')
            offset = new_offset
            print(f'Loading map: {offset}/{len(package)} bytes ({offset * 100 // len(package)}%)', flush=True)
            await asyncio.sleep(0.067)
        if b[6] != 4:
            await command(5, gen=generation)
        print('All bytes acknowledged; checking full map...', flush=True)
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            b = status(await asyncio.wait_for(client.read_gatt_char(STATUS), 8), context)
            if b[6] == 4 and struct.unpack_from('<I', b, 36)[0] == generation:
                print('Display confirmed ACTIVE. Map only; live phone GPS remains unavailable.', flush=True)
                return
            if b[6] == 5:
                raise RuntimeError(f'Display validation failed: error={b[7]}')
            await asyncio.sleep(0.25)
        raise RuntimeError('Activation timeout')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--device', required=True, help='Explicit OS BLE identifier of the already paired cluster')
    p.add_argument('--package', required=True, type=Path)
    p.add_argument('--mtu', type=int, default=96, choices=(96, 247), help='Verified actual limit; CoreBluetooth does not expose it (default96)')
    args = p.parse_args()
    try:
        asyncio.run(transfer(args))
    except (RuntimeError, OSError, asyncio.TimeoutError) as error:
        p.exit(1, f'Map transfer stopped: {error}\n')
    except Exception as error:
        p.exit(1, f'Map transport stopped ({type(error).__name__}); close other clients and reconnect.\n')
    except KeyboardInterrupt:
        p.exit(130, 'Map transfer cancelled; connection closed.\n')


if __name__ == '__main__':
    main()
