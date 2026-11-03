#!/usr/bin/env python3
"""Import a small Overpass road snapshot into the host LVGL map format.

No tiles, credentials, routing or live device data. OSM-derived output remains
ODbL data with provenance beside it; generated downloads/output stay in build/.
"""
import argparse
import datetime
import hashlib
import json
import math
import struct
import zlib
from pathlib import Path
import unicodedata
import urllib.parse
import urllib.request

BOUNDS = (22.546, 88.337, 22.574, 88.365)
MAX_ROADS = 2048
MAX_POINTS = 8
MAX_DOWNLOAD = 8 * 1024 * 1024
ENDPOINT = 'https://overpass-api.de/api/interpreter'
QUERY = '[out:json][timeout:45];way["highway"]["area"!="yes"](%s);out tags geom;' % ','.join(map(str, BOUNDS))


def clip(a, b, box):
    dx, dy = b[0] - a[0], b[1] - a[1]
    left, bottom, right, top = box
    low, high = 0.0, 1.0
    for p, q in zip((-dx, dx, -dy, dy), (a[0] - left, right - a[0], a[1] - bottom, top - a[1])):
        if p == 0:
            if q < 0:
                return None
            continue
        t = q / p
        if p < 0:
            low = max(low, t)
        else:
            high = min(high, t)
        if low > high:
            return None
    return ((a[0] + low * dx, a[1] + low * dy), (a[0] + high * dx, a[1] + high * dy))


def simplify(points, tolerance=2.0):
    """Iterative Douglas-Peucker, preserving ends and bounding recursion/work."""
    if len(points) < 3:
        return points
    keep = {0, len(points) - 1}
    pending = [(0, len(points) - 1)]
    while pending:
        start, end = pending.pop()
        ax, ay = points[start]
        bx, by = points[end]
        dx, dy = bx - ax, by - ay
        length = dx * dx + dy * dy
        best, index = tolerance * tolerance, None
        for i in range(start + 1, end):
            x, y = points[i]
            t = max(0.0, min(1.0, ((x - ax) * dx + (y - ay) * dy) / length)) if length else 0.0
            distance = (x - ax - t * dx) ** 2 + (y - ay - t * dy) ** 2
            if distance > best:
                best, index = distance, i
        if index is not None:
            keep.add(index)
            pending.extend(((start, index), (index, end)))
    return [points[i] for i in sorted(keep)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=Path('build/maps/kolkata/osm-roads.json'))
    parser.add_argument('--output', type=Path, default=Path('build/maps/kolkata/kolkata.r15map'))
    parser.add_argument('--download', action='store_true', help='Fetch once if the raw snapshot is absent; existing source is reused.')
    args = parser.parse_args()
    if not args.source.exists():
        if not args.download:
            parser.error('Source absent; use --download for the first small Overpass request.')
        args.source.parent.mkdir(parents=True, exist_ok=True)
        request = urllib.request.Request(ENDPOINT, data=urllib.parse.urlencode({'data': QUERY}).encode(), headers={
            'User-Agent': 'R15SmartCluster-MapPreview/0.1 (+https://github.com/CodewaveSandy/r15-smart-cluster)',
            'Content-Type': 'application/x-www-form-urlencoded',
        })
        print('Downloading central Kolkata roads once from Overpass...', flush=True)
        with urllib.request.urlopen(request, timeout=75) as response:
            raw = response.read(MAX_DOWNLOAD + 1)
        if len(raw) > MAX_DOWNLOAD:
            parser.error('Snapshot exceeds the 8 MiB import limit.')
        parsed = json.loads(raw)
        if parsed.get('remark'):
            parser.error('Overpass reported an incomplete response: ' + str(parsed['remark']))
        args.source.write_bytes(raw)
    if args.source.stat().st_size > MAX_DOWNLOAD:
        parser.error('Source exceeds the 8 MiB import limit.')
    raw = args.source.read_bytes()
    data = json.loads(raw)
    if data.get('remark') or not isinstance(data.get('elements'), list):
        parser.error('Incomplete/invalid Overpass source.')
    south, west, north, east = BOUNDS
    origin_lat, origin_lon = (south + north) / 2, (west + east) / 2
    # Small-area equirectangular projection on the WGS84 sphere approximation.
    # It is a local rendering approximation, not navigation/survey calibration.
    radius = 6371008.8
    def project(lat, lon):
        if not math.isfinite(lat) or not math.isfinite(lon) or not -90 <= lat <= 90 or not -180 <= lon <= 180:
            raise ValueError('Invalid geographic coordinate')
        return (math.radians(lon - origin_lon) * radius * math.cos(math.radians(origin_lat)),
                math.radians(lat - origin_lat) * radius)
    lower, upper = project(south, west), project(north, east)
    box = (*lower, *upper)
    roads, source_points, output_points, normalized_names = [], 0, 0, 0
    classes = {}
    major = {'motorway', 'trunk', 'primary', 'secondary', 'tertiary', 'motorway_link', 'trunk_link', 'primary_link', 'secondary_link', 'tertiary_link'}
    allowed = major | {'residential', 'service', 'living_street', 'unclassified', 'road', 'footway', 'pedestrian', 'path', 'steps', 'track', 'busway'}
    for element in data['elements']:
        tags, geometry = element.get('tags', {}), element.get('geometry', [])
        highway = tags.get('highway')
        if element.get('type') != 'way' or highway not in allowed or len(geometry) < 2:
            continue
        if len(geometry) > 10000:
            parser.error('A way exceeds the 10,000-point safety limit.')
        classes[highway] = classes.get(highway, 0) + 1
        name = tags.get('name:en', tags.get('name', ''))
        ascii_name = unicodedata.normalize('NFKD', name).encode('ascii', 'ignore').decode()
        ascii_name = ''.join(c for c in ascii_name if 32 <= ord(c) <= 126)[:23]
        if not ascii_name.strip():
            ascii_name = ''
        normalized_names += name != ascii_name
        points = [project(float(p['lat']), float(p['lon'])) for p in geometry]
        source_points += len(points)
        # Split disconnected clipped portions; never join across missing geometry.
        pieces, current = [], []
        for a, b in zip(points, points[1:]):
            segment = clip(a, b, box)
            if segment is None:
                if current:
                    pieces.append(current)
                    current = []
                continue
            ca, cb = segment
            if current and math.dist(current[-1], ca) > 0.1:
                pieces.append(current)
                current = []
            if not current:
                current.append(ca)
            current.append(cb)
        if current:
            pieces.append(current)
        for piece in pieces:
            rounded = []
            for x, y in simplify(piece):
                point = (int(math.copysign(math.floor(abs(x) + 0.5), x)), int(math.copysign(math.floor(abs(y) + 0.5), y)))
                if not rounded or rounded[-1] != point:
                    rounded.append(point)
            for start in range(0, len(rounded) - 1, MAX_POINTS - 1):
                chunk = rounded[start:start + MAX_POINTS]
                if len(chunk) < 2:
                    continue
                kind = 1 if highway in major else 4 if highway == 'service' else 3 if highway in {'footway', 'path', 'pedestrian', 'steps', 'track'} else 0
                roads.append((kind, ascii_name, chunk))
                output_points += len(chunk)
                if len(roads) > MAX_ROADS:
                    parser.error('Road capacity exceeded; choose a smaller area. No roads were silently dropped.')
    if not roads:
        parser.error('No usable road geometry.')
    roads.sort(key=lambda r: (-r[0], not bool(r[1]), r[1]))
    lines = ['R15MAP 1', 'Kolkata, West Bengal', f'{origin_lat:.8f} {origin_lon:.8f}', f'{len(roads)}']
    for kind, name, points in roads:
        lines.append(f'{kind} {len(points)} {json.dumps(name)} ' + ' '.join(f'{x} {y}' for x, y in points))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text('\n'.join(lines) + '\n', encoding='ascii')
    metadata = {
        'source': 'OpenStreetMap contributors', 'license': 'ODbL-1.0', 'license_url': 'https://www.openstreetmap.org/copyright',
        'endpoint': ENDPOINT, 'query': QUERY, 'bbox_south_west_north_east': BOUNDS,
        'origin_lat_lon': [origin_lat, origin_lon], 'osm_timestamp': data.get('osm3s', {}).get('timestamp_osm_base'),
        'imported_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'source_sha256': hashlib.sha256(raw).hexdigest(),
        'ways_by_highway': classes, 'road_chunks': len(roads), 'source_points': source_points, 'output_points': output_points,
        'simplification_tolerance_m': 2.0, 'normalized_or_truncated_names': normalized_names,
        'note': 'Road snapshot only. No route, live traffic, legal access decision or GPS. Source/generated map are ODbL data; not firmware assets.',
    }
    # Deterministic wire package and matching local phone geometry. All exports
    # come from the same clipped/simplified source; no hosted tile dependency.
    body = bytearray()
    features = []
    for kind, name, points in roads:
        body += struct.pack('<BBBB', kind, len(points), len(name), 0) + name.encode('ascii')
        body += b''.join(struct.pack('<hh', x, y) for x, y in points)
        coordinates = [[origin_lon + math.degrees(x / (radius * math.cos(math.radians(origin_lat)))),
                        origin_lat + math.degrees(y / radius)] for x, y in points]
        features.append({'type': 'Feature', 'properties': {'kind': kind, 'name': name},
                         'geometry': {'type': 'LineString', 'coordinates': coordinates}})
    if len(body) + 64 > 65536:
        parser.error('Binary region exceeds 65536 bytes; choose a smaller area. No roads dropped.')
    timestamp = metadata['osm_timestamp']
    if not timestamp:
        parser.error('OSM source timestamp required for wire package provenance.')
    source_epoch = int(datetime.datetime.fromisoformat(timestamp.replace('Z', '+00:00')).timestamp())
    header = bytearray(64)
    struct.pack_into('<4sBBH6iHHIII', header, 0, b'R15B', 1, 0, 64,
                     round(origin_lat * 1e7), round(origin_lon * 1e7),
                     *(round(v * 1e7) for v in BOUNDS), len(roads), 0, len(body), source_epoch, zlib.crc32(body))
    package = bytes(header + body)
    wire = args.output.with_suffix('.r15b')
    wire.write_bytes(package)
    args.output.with_suffix('.geojson').write_text(json.dumps({'type': 'FeatureCollection', 'features': features}, separators=(',', ':')) + '\n')
    manifest = {'format': 'R15B', 'version': 1, 'bytes': len(package), 'sha256': hashlib.sha256(package).hexdigest(),
                'body_crc32': f'{zlib.crc32(body):08x}', 'road_chunks': len(roads), 'points': output_points,
                'bbox_south_west_north_east': BOUNDS, 'origin_lat_lon': [origin_lat, origin_lon],
                'osm_timestamp': timestamp, 'source_sha256': metadata['source_sha256'],
                'attribution': '© OpenStreetMap contributors', 'license': 'ODbL-1.0',
                'license_url': metadata['license_url']}
    args.output.with_suffix('.manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Wire package: {wire} ({len(package)} bytes), SHA256 {manifest["sha256"]}')
    args.output.with_suffix('.provenance.json').write_text(json.dumps(metadata, indent=2) + '\n')
    args.output.with_suffix('.LICENSE.txt').write_text('Map data © OpenStreetMap contributors, ODbL 1.0.\nhttps://www.openstreetmap.org/copyright\nOpen Database License: https://opendatacommons.org/licenses/odbl/1-0/\nDerived map data remains subject to the ODbL. Software licensing is separate.\n')
    print(f'Imported {len(roads)} road chunks / {output_points} points: {args.output}')
    print(f'OSM timestamp: {metadata["osm_timestamp"]}; name fallbacks/truncations: {normalized_names}')


if __name__ == '__main__':
    main()
