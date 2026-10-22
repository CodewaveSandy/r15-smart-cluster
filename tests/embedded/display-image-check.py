"""Bounds checks for the flash packager; no hardware access."""
import importlib.util
from pathlib import Path
import struct
import unittest

source = Path(__file__).resolve().parents[2] / 'tools/display/prepare_image.py'
spec = importlib.util.spec_from_file_location('display_image', source)
image = importlib.util.module_from_spec(spec)
spec.loader.exec_module(image)


def entry(start, size, kind=1):
    return struct.pack('<HBBII16sI', 0x50AA, kind, 0, start, size, b'test', 0)


class PartitionBounds(unittest.TestCase):
    def test_exact_end_and_adjacent(self):
        self.assertEqual(len(image.partitions(entry(0x9000, 0x7000) + entry(0x10000, 0x3F0000, 0))), 2)

    def test_above_flash(self):
        with self.assertRaises(ValueError):
            image.partitions(entry(0x10000, 0x3F0001, 0))

    def test_zero_size(self):
        with self.assertRaises(ValueError):
            image.partitions(entry(0x10000, 0))

    def test_overlapping_even_if_unsorted(self):
        with self.assertRaises(ValueError):
            image.partitions(entry(0x10000, 0x300000, 0) + entry(0x9000, 0x8000))

    def test_boot_region(self):
        with self.assertRaises(ValueError):
            image.partitions(entry(0x8000, 0x1000))

    def test_missing_table(self):
        with self.assertRaises(ValueError):
            image.partitions(bytes(32))


if __name__ == '__main__':
    unittest.main()
