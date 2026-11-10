"""Stamp CRC32 + update counter into both application slots and glue the images.

Layout constants come from cfg.json and must match boot_rules.h.

The CRC is zlib's: reflected 0xEDB88320, init 0xFFFFFFFF, final complement -
bit-for-bit what src/utils/crc32.h computes on the device, which is the whole
point of this script.

Usage: postbuild.py <bin_file_basename> <config_file> <output_directory>
"""

import json
import os.path
import struct
import sys
import zlib

bin_base, config_file, output_dir = sys.argv[1:4]

cfg = json.load(open(config_file))
boot_size = int(cfg['bootloader_size_in_bytes'], 16)
slot_size = int(cfg['firmware_size_in_bytes'], 16)
app_size = int(cfg['application_size'], 16)
counter = int(cfg['update_counter'], 16)


def slot(path):
    """Pad one application to its slot and append CRC32 + update counter."""
    image = bytearray(open(path, 'rb').read().ljust(app_size, b'\xff'))
    image += struct.pack('<III', zlib.crc32(image), counter, ~counter & 0xFFFFFFFF)
    assert len(image) == slot_size, f'{path}: {len(image)} != {slot_size}'
    return image


name = os.path.basename(bin_base)
app0 = slot(f'{bin_base}_app0.bin')
app1 = slot(f'{bin_base}_app1.bin')

boot = open(f'{bin_base}_bootloader.bin', 'rb').read().ljust(boot_size, b'\xff')

open(os.path.join(output_dir, f'{name}_app0_app1.bin'), 'wb').write(app0 + app1)
open(os.path.join(output_dir, f'{name}_boot_app0_app1.bin'), 'wb').write(boot + app0 + app1)
