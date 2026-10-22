"""Validate IDF flash metadata and partition bounds, then merge with esptool 4.8.1."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

FLASH_SIZE = 0x400000


def partitions(data):
    result = []
    for offset in range(0, len(data) - 31, 32):
        magic, kind, subtype, start, size, label, flags = struct.unpack_from('<HBBII16sI', data, offset)
        if magic != 0x50AA:
            break  # checksum/end marker
        if not size or start < 0x9000 or start + size > FLASH_SIZE:
            raise ValueError('Partition outside the 4 MB device')
        if any(start < b + length and b < start + size for _, b, length in result):
            raise ValueError('Overlapping partitions')
        result.append((kind, start, size))
    if not result:
        raise ValueError('No partition entries')
    return result


def prepare(build):
    metadata = json.loads((build / 'flasher_args.json').read_text())
    if metadata['extra_esptool_args']['chip'] != 'esp32s3':
        raise ValueError('Wrong chip target')
    settings = metadata['flash_settings']
    if settings['flash_size'] != '4MB':
        raise ValueError('Wrong flash capacity')
    images = []
    for address, name in metadata['flash_files'].items():
        start = int(address, 0)
        path = build / name
        # PlatformIO renames IDF's application/bootloader/partition outputs.
        if not path.is_file():
            if address == metadata['app']['offset']:
                path = build / 'firmware.bin'
            elif address == metadata['bootloader']['offset']:
                path = build / 'bootloader.bin'
            elif address == metadata['partition-table']['offset']:
                path = build / 'partitions.bin'
        data = path.read_bytes()
        if not data or start + len(data) > FLASH_SIZE:
            raise ValueError('Image outside the 4 MB device')
        images.append((start, path, data))
    images.sort()
    for left, right in zip(images, images[1:]):
        if left[0] + len(left[2]) > right[0]:
            raise ValueError('Overlapping flash images')
    table = int(metadata['partition-table']['offset'], 0)
    app = int(metadata['app']['offset'], 0)
    if images[0][0] != 0 or table != 0x8000 or app != 0x10000 or len(images) != 3:
        raise ValueError('Unexpected bootloader/table/application layout')
    entries = partitions(next(data for start, _, data in images if start == table))
    app_data = next(data for start, _, data in images if start == app)
    if not any(kind == 0 and start == app and len(app_data) <= size for kind, start, size in entries):
        raise ValueError('Application exceeds its partition')
    # Only the application image may overlap a partition's data region.
    if table + len(next(data for start, _, data in images if start == table)) > min(e[1] for e in entries):
        raise ValueError('Partition table overlaps partition contents')
    output = build / 'r15-display.factory.bin'
    command = [sys.executable, '-m', 'esptool', '--chip', 'esp32s3', 'merge_bin',
               '--flash_mode', settings['flash_mode'], '--flash_freq', settings['flash_freq'],
               '--flash_size', '4MB', '-o', str(output)]
    for start, path, _ in images:
        command.extend([hex(start), str(path)])
    subprocess.run(command, check=True)
    if output.stat().st_size > FLASH_SIZE:
        raise ValueError('Merged image exceeds flash')
    report = {'chip': 'esp32s3', 'flash_bytes': FLASH_SIZE,
              'image_bytes': output.stat().st_size,
              'sha256': hashlib.sha256(output.read_bytes()).hexdigest(),
              'inputs': [{'offset': hex(start), 'file': str(path.relative_to(build)),
                          'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
                         for start, path, data in images]}
    (build / 'r15-display-image.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Validated partition bounds and application fit: {output}')


if __name__ == '__main__':
    prepare(Path(sys.argv[1]).resolve())
