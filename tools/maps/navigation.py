#!/usr/bin/env python3
"""Prepare bounded v2 map objects, or send them over an already-paired BLE link.

No location simulation, pairing approval, bond replacement or vehicle control.
The optional fixture route is explicitly a host demonstration, not routing.
"""
import argparse
import asyncio
import hashlib
import json
import math
from pathlib import Path
import struct
import time
import zlib

WORLD = 2 * math.pi * 6378137.0
PREFIX = '813b1240-8fd9-4d67-a53b-9f0a2531'
CAP, COMMAND, STATUS = (PREFIX + suffix for suffix in ('c019', 'c01a', 'c01b'))


def mercator(lat, lon):
    lat = math.radians(max(-85.05112878, min(85.05112878, lat)))
    return math.radians(lon) * 6378137, 6378137 * math.log(math.tan(math.pi / 4 + lat / 2))


def clip(a, b, left, bottom, right, top):
    dx, dy = b[0] - a[0], b[1] - a[1]
    lo, hi = 0.0, 1.0
    for p, q in ((-dx, a[0] - left), (dx, right - a[0]), (-dy, a[1] - bottom), (dy, top - a[1])):
        if p == 0:
            if q < 0:
                return None
        else:
            t = q / p
            if p < 0:
                lo = max(lo, t)
            else:
                hi = min(hi, t)
            if lo > hi:
                return None
    return ((a[0] + dx * lo, a[1] + dy * lo), (a[0] + dx * hi, a[1] + dy * hi))


def object_blob(kind, source, count, payload, z=0, x=0, y=0, route=0, revision=0, start=0, end=0, distance=0, seconds=0, partial=False):
    b = bytearray(64)
    b[:8] = bytes((82, 49, 53, 79, 2, kind, 64, int(partial)))
    struct.pack_into('<IHHQI', b, 8, source, 1, 1, route, revision)
    struct.pack_into('<BBHIIIIIII', b, 28, z, 0, count, x, y, start, end, distance, seconds, zlib.crc32(payload))
    b += payload
    if len(b) > 32768:
        raise ValueError('Object exceeds firmware encoded limit; choose finer cells or simplify explicitly')
    return bytes(b)


def prepare(args):
    with args.package.open('rb') as stream:
        raw = stream.read(65537)
    if not 64 <= len(raw) <= 65536 or raw[:8] != b'R15B\x01\x00\x40\x00' or zlib.crc32(raw[64:]) != struct.unpack_from('<I', raw, 44)[0]:
        raise ValueError('Invalid regional R15B input')
    lat0, lon0 = struct.unpack_from('<ii', raw, 8)
    count = struct.unpack_from('<H', raw, 32)[0]
    south, west, north, east = struct.unpack_from('<iiii', raw, 16)
    if not 1 <= count <= 2048 or raw[34:36] != b'\0\0' or any(raw[48:64]) or struct.unpack_from('<I', raw, 36)[0] != len(raw) - 64:
        raise ValueError('Invalid regional counts/reserved fields/body length')
    if not (-850000000 <= south < north <= 850000000 and -1800000000 <= west < east <= 1800000000 and south <= lat0 <= north and west <= lon0 <= east):
        raise ValueError('Invalid regional geographic bounds')
    k = 6371008.8 * math.pi / 180 / 1e7
    bounds = ((west - lon0) * k * math.cos(math.radians(lat0 / 1e7)), (south - lat0) * k,
              (east - lon0) * k * math.cos(math.radians(lat0 / 1e7)), (north - lat0) * k)
    if max(abs(value) for value in bounds) > 5001:
        raise ValueError('Regional local-plane bounds exceed the v1 input profile')
    source = hashlib.sha256(b'R15NAV2 roads-v2-partial-coverage OSM attribution-1\0' + hashlib.sha256(raw).digest()).digest()
    source_id = struct.unpack_from('<I', source)[0]
    roads, offset = [], 64
    for _ in range(count):
        kind, n, length, reserved = struct.unpack_from('<BBBB', raw, offset)
        if kind > 4 or not 2 <= n <= 8 or length > 23 or reserved:
            raise ValueError('Invalid road record')
        name = raw[offset + 4:offset + 4 + length]
        name.decode('ascii')
        if any(value < 32 or value > 126 for value in name) or (name and not name.strip()):
            raise ValueError('Invalid regional road label')
        points = []
        local = []
        for i in range(n):
            east, north = struct.unpack_from('<hh', raw, offset + 4 + length + i * 4)
            if not (bounds[0] - 1 <= east <= bounds[2] + 1 and bounds[1] - 1 <= north <= bounds[3] + 1):
                raise ValueError('Regional point lies outside declared coverage')
            local.append((east, north))
            lat = lat0 / 1e7 + north / (6371008.8 * math.pi / 180)
            lon = lon0 / 1e7 + east / (6371008.8 * math.pi / 180 * math.cos(math.radians(lat0 / 1e7)))
            points.append((lat, lon))
        if len(set(local)) < 2:
            raise ValueError('Degenerate regional road')
        roads.append((kind, name, points))
        offset += 4 + length + n * 4
    if offset != len(raw):
        raise ValueError('Unexpected regional trailing data')
    cells = {}
    size, tiles = WORLD / (1 << args.zoom), 1 << args.zoom
    for road_index, (kind, name, points) in enumerate(roads):
        world = [mercator(*point) for point in points]
        for a, b in zip(world, world[1:]):
            xmin, xmax = sorted((int((a[0] + WORLD / 2) // size), int((b[0] + WORLD / 2) // size)))
            ymin, ymax = sorted((int((WORLD / 2 - a[1]) // size), int((WORLD / 2 - b[1]) // size)))
            if (xmax - xmin + 1) * (ymax - ymin + 1) > 256:
                raise ValueError('Segment crosses too many cells; use a coarser explicit zoom')
            for x in range(xmin, xmax + 1):
                for y in range(max(0, ymin), min(tiles - 1, ymax) + 1):
                    left, top = -WORLD / 2 + x * size, WORLD / 2 - y * size
                    segment = clip(a, b, left, top - size, left + size, top)
                    if segment is None:
                        continue
                    uv = [(max(0, min(4096, round((p[0] - left) / size * 4096))), max(0, min(4096, round((top - p[1]) / size * 4096)))) for p in segment]
                    if uv[0] == uv[1]:
                        continue
                    records = cells.setdefault((x % tiles, y), [])
                    if records and records[-1][2] == road_index and records[-1][3][-1] == uv[0] and len(records[-1][3]) < 8:
                        records[-1][3].append(uv[1])
                    else:
                        records.append((kind, name, road_index, list(uv)))
    # Select a smaller, explicit fixture region rather than silently truncating
    # dense source tiles or pretending the desktop tool is the app downloader.
    if len(cells) > 64:
        raise ValueError('Fixture preparation exceeds 64 cells; use a smaller regional extract')
    args.output.mkdir(parents=True, exist_ok=True)
    manifest = {'schema': 2, 'sourceSha256': source.hex(), 'style': 1, 'attribution': 1,
                'inputSha256': hashlib.sha256(raw).hexdigest(), 'objects': [], 'demoRoute': args.demo_route}

    def save(identifier, blob, extra):
        name = f'{identifier}.r15o'
        (args.output / name).write_bytes(blob)
        manifest['objects'].append({'id': identifier, 'file': name, 'bytes': len(blob), 'sha256': hashlib.sha256(blob).hexdigest(), **extra})

    south, west, north, east = struct.unpack_from('<iiii', raw, 16)
    source_left, source_bottom = mercator(south / 1e7, west / 1e7)
    source_right, source_top = mercator(north / 1e7, east / 1e7)
    for identifier, ((x, y), records) in enumerate(sorted(cells.items()), 1):
        if len(records) > 512:
            raise ValueError(f'Cell z{args.zoom} exceeds 512 road chunks; prepare explicitly finer cells (no data was truncated)')
        payload = b''.join(bytes((kind, len(points), len(name), 0)) + name + b''.join(struct.pack('<HH', *point) for point in points) for kind, name, _, points in records)
        left, top = -WORLD / 2 + x * size, WORLD / 2 - y * size
        partial = left < source_left or left + size > source_right or top > source_top or top - size < source_bottom
        save(identifier, object_blob(1, source_id, len(records), payload, args.zoom, x, y, partial=partial), {'kind': 1, 'z': args.zoom, 'x': x, 'y': y, 'partialCoverage': partial})
    if args.demo_route:
        # One real road polyline; an illustration, never a calculated journey.
        _, _, points = max(roads, key=lambda r: len(r[2]))
        distance = max(1, round(sum(math.hypot(*(a - b for a, b in zip(mercator(*p), mercator(*q)))) for p, q in zip(points, points[1:])) * math.cos(math.radians(lat0 / 1e7))))
        payload = b''.join(struct.pack('<ii', round(lat * 1e7), round(lon * 1e7)) for lat, lon in points)
        route = int.from_bytes(hashlib.sha256(payload).digest()[:8], 'little') or 1
        for identifier, kind in ((1000000, 2), (1000001, 3)):
            save(identifier, object_blob(kind, source_id, len(points), payload, route=route, revision=1, end=distance, distance=distance, seconds=0), {'kind': kind, 'route': route, 'revision': 1})
        manifest['demoFixE7'] = [round(points[0][0] * 1e7), round(points[0][1] * 1e7)]
    (args.output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    index = f'R15NAV2 {source.hex()} 1\n' + ''.join(f"{o['id']} {o['file']}\n" for o in manifest['objects'])
    (args.output / 'preview.index').write_text(index)
    (args.output / 'LICENSE.txt').write_text('© OpenStreetMap contributors. Derived from the supplied regional snapshot. ODbL 1.0. https://openstreetmap.org/copyright\nHost demo route, when present, is NOT calculated routing or live guidance. Preserve input provenance and licence.\n')
    print(f"Prepared {len(cells)} complete cells; {len(manifest['objects'])} objects. No network, board access or GPS.")


def elapsed():
    return time.monotonic_ns() // 1000000


def wire(op, context, session, seq, body=b'', obj=0, offset=0):
    b = struct.pack('<4sBBBBIIIIIIIHH', b'R15V', 2, op, 40, 0, *context, session, seq, obj, offset, len(body), 0) + body
    return b + struct.pack('<I', zlib.crc32(b))


def reply(raw, context, session):
    b = bytes(raw)
    if len(b) < 44 or b[:5] != b'R15Y\x02' or b[6:8] != b'\x28\0' or zlib.crc32(b[:-4]) != struct.unpack_from('<I', b, len(b) - 4)[0] or struct.unpack_from('<III', b, 8) != context or struct.unpack_from('<I', b, 20)[0] != session or struct.unpack_from('<HH', b, 36) != (len(b) - 44, 0):
        raise ValueError('Invalid or changed navigation context')
    return b


async def send(args):
    from bleak import BleakClient, BleakScanner
    with (args.bundle / 'manifest.json').open('rb') as stream:
        manifest_bytes = stream.read(131073)
    if len(manifest_bytes) > 131072:
        raise ValueError('Manifest exceeds the bounded developer-tool limit')
    manifest = json.loads(manifest_bytes)
    if manifest.get('schema') != 2:
        raise ValueError('Unsupported bundle')
    if not isinstance(manifest.get('objects'), list) or len(manifest['objects']) > 128:
        raise ValueError('Invalid bounded object manifest')
    if manifest.get('demoRoute'):
        print('Demo route objects will NOT be activated or sent by this radio tool.', flush=True)
    device = await BleakScanner.find_device_by_filter(lambda d, a: d.address == args.device and (a.local_name or d.name or '').startswith('R15 Cluster'), timeout=15)
    if device is None:
        raise ValueError('Explicitly selected, already-paired cluster not found')
    async with BleakClient(device) as client:
        cap = bytes(await asyncio.wait_for(client.read_gatt_char(CAP), 8))
        if len(cap) != 64 or cap[:6] != b'R15J\x02\x40' or cap[6] != 15 or zlib.crc32(cap[:-4]) != struct.unpack_from('<I', cap, 60)[0]:
            raise ValueError('V2 downstream unavailable')
        context = struct.unpack_from('<III', cap, 8)
        if not all(context):
            raise ValueError('Current secured connection/display context not ready')
        session, sequence = (elapsed() & 0xffffffff) or 1, 0
        maximum = min(240, args.mtu - 3)
        queue = asyncio.Queue(maxsize=8)

        def notification(_, raw):
            if queue.full():
                queue.get_nowait()
            queue.put_nowait(bytes(raw))

        await client.start_notify(STATUS, notification)

        async def ack(target, packets):
            for attempt in range(4):
                deadline = time.monotonic() + 2
                while time.monotonic() < deadline:
                    try:
                        raw = await asyncio.wait_for(queue.get(), deadline - time.monotonic())
                    except asyncio.TimeoutError:
                        break
                    if len(raw) >= 24 and struct.unpack_from('<III', raw, 8) == context and struct.unpack_from('<I', raw, 20)[0] != session:
                        continue  # Pre-OPEN display heartbeat, never an ACK.
                    b = reply(raw, context, session)
                    if b[5] != 64:
                        continue  # Bench sends the explicit bundle, never fetches network tiles.
                    accepted = struct.unpack_from('<I', b, 24)[0]
                    if accepted >= target:
                        if b[41]:
                            raise ValueError(f'Display error {b[41]}; accepted offset {struct.unpack_from("<I", b, 32)[0]}')
                        return b
                raw = bytes(await client.read_gatt_char(STATUS))
                if len(raw) >= 24 and struct.unpack_from('<III', raw, 8) == context and struct.unpack_from('<I', raw, 20)[0] != session:
                    for packet in packets:
                        await client.write_gatt_char(COMMAND, packet, response=True)
                    continue
                b = reply(raw, context, session)
                if b[5] == 64 and struct.unpack_from('<I', b, 24)[0] >= target and not b[41]:
                    return b
                for packet in packets:
                    await client.write_gatt_char(COMMAND, packet, response=True)
                if attempt < 3:
                    await asyncio.sleep((0.25, 0.5, 1)[attempt])
            raise ValueError('Display ACK timeout; reconnect and reuse verified hashes')

        async def control(op, body=b'', obj=0, offset=0):
            nonlocal sequence
            sequence = (sequence + 1) & 0xffffffff or 1
            packet = wire(op, context, session, sequence, body, obj, offset)
            await client.write_gatt_char(COMMAND, packet, response=True)
            return await ack(sequence, [packet])

        await control(1, bytes.fromhex(manifest['sourceSha256']) + struct.pack('<HHB3x', 1, manifest['attribution'], 4))
        started, useful = elapsed(), 0
        for obj in manifest['objects']:
            if obj['kind'] != 1:
                continue
            path = (args.bundle / obj['file']).resolve()
            if path.parent != args.bundle.resolve():
                raise ValueError('Bundle path escape')
            with path.open('rb') as stream:
                blob = stream.read(32769)
            digest = hashlib.sha256(blob).digest()
            if not 64 <= len(blob) <= 32768 or digest.hex() != obj['sha256']:
                raise ValueError('Bundle hash/length mismatch')
            b = await control(4, struct.pack('<I', len(blob)) + digest + bytes((1, 0, 0, 0)), obj['id'])
            offset = struct.unpack_from('<I', b, 32)[0]
            while offset < len(blob):
                if not b[43]:
                    raise ValueError('Downstream credit unavailable; wait/reconnect before retrying the verified bundle')
                packets = []
                for _ in range(min(4, cap[7], b[43])):
                    if offset >= len(blob):
                        break
                    chunk = blob[offset:offset + maximum - 44]
                    sequence = (sequence + 1) & 0xffffffff or 1
                    packets.append(wire(5, context, session, sequence, chunk, obj['id'], offset))
                    offset += len(chunk)
                for packet in packets:
                    await client.write_gatt_char(COMMAND, packet, response=True)
                b = await ack(sequence, packets)
                accepted = struct.unpack_from('<I', b, 32)[0]
                if accepted != offset:
                    raise ValueError('Missing range: restart at the display accepted offset with new control sequences')
                print(f"Object {obj['id']}: {offset}/{len(blob)} bytes accepted", flush=True)
            if b[40] != 6:
                await control(6, obj=obj['id'])
                deadline = time.monotonic() + 15
                while time.monotonic() < deadline:
                    b = reply(await client.read_gatt_char(STATUS), context, session)
                    if b[41]:
                        raise ValueError(f'Object verification error {b[41]}')
                    if b[40] in (4, 6):
                        break
                    await asyncio.sleep(0.1)
                else:
                    raise ValueError('Object verification timeout')
            useful += len(blob)
            print(f'Verified useful bytes {useful}; elapsed {(elapsed() - started) / 1000:.1f}s', flush=True)
        print('Each sent cell verified; the bounded display cache may evict remote cells. No GPS, route activation or bike controls were sent.', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    p = commands.add_parser('prepare')
    p.add_argument('--package', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--zoom', type=int, choices=range(10, 19), default=15)
    p.add_argument('--demo-route', action='store_true', help='Explicit host-only illustrative road line; not routing')
    p = commands.add_parser('send')
    p.add_argument('--bundle', type=Path, required=True)
    p.add_argument('--device', required=True, help='Already-paired OS identifier; tool never enrolls/replaces a bond')
    p.add_argument('--mtu', type=int, choices=(96, 247), default=96, help='Verified actual bound; CoreBluetooth does not expose negotiated MTU')
    args = parser.parse_args()
    try:
        prepare(args) if args.command == 'prepare' else asyncio.run(send(args))
    except (ValueError, OSError, KeyError, struct.error, asyncio.TimeoutError) as error:
        parser.exit(1, f'Navigation stopped: {error}\n')
    except KeyboardInterrupt:
        parser.exit(130, 'Cancelled; connection closed.\n')
    except Exception as error:
        parser.exit(1, f'Navigation transport stopped ({type(error).__name__}); close other clients and reconnect.\n')


if __name__ == '__main__':
    main()
