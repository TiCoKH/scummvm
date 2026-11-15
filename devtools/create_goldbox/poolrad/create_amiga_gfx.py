#!/usr/bin/env python3
# ScummVM - Graphic Adventure Engine
#
# ScummVM is the legal property of its developers, whose names
# are too numerous to list here. Please refer to the COPYRIGHT
# file distributed with this source distribution.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""
Build poolradamigagfx.dat from the Amiga Pool of Radiance v1.3 executable.

The Amiga version draws window borders from bitmaps built into its "program"
file (BitMap_TopBorder etc., blitted by DRAW_BitmapBorder) instead of the
8x8 frame tiles the DOS version uses.

Usage: create_amiga_gfx.py <path to data/program> <output .dat>

File format (all integers little-endian unless noted):
  uint32 BE  magic 'GBGX'
  uint16     version (1)
  uint16     image count
  per image:
    uint16   id
    uint16   width
    uint16   height
    byte     pixels[width * height]  (palette indices 0-15)
"""

import hashlib
import struct
import sys

PROGRAM_SIZE = 459028
PROGRAM_MD5 = "0b19c9ec015fb0d0a29cd67d8275eee9"

# Program address of the chip data hunk minus its offset in the file.
ADDRESS_TO_FILE = 0x2175C4

# id, width used, height, bytes per row, plane addresses (v1.3 program).
# The ids name the edge the image is drawn on; the original symbol names
# BitMap_RightBorder/BitMap_LeftBorder are drawn on the left/right edges.
IMAGES = [
	(0, 320, 8, 40, (0x2611B0, 0x2612F0, 0x261430, 0x261570)),    # BitMap_TopBorder
	(1, 320, 8, 40, (0x2616D0, 0x261810, 0x261950, 0x261A90)),    # BitMap_BottomBorder
	(2, 8, 190, 2, (0x262200, 0x26237C, 0x2624F8, 0x262674)),     # BitMap_RightBorder
	(3, 8, 190, 2, (0x261BF0, 0x261D6C, 0x261EE8, 0x262064)),     # BitMap_LeftBorder
	(4, 8, 8, 2, (0x262810, 0x262820, 0x262830, 0x262840)),       # BitMap_CornersBorder
]


def decode_planar(program, width, height, bytes_per_row, planes):
	pixels = bytearray(width * height)
	for plane_index, address in enumerate(planes):
		offset = address - ADDRESS_TO_FILE
		data = program[offset:offset + bytes_per_row * height]
		for y in range(height):
			for x in range(width):
				if (data[y * bytes_per_row + x // 8] >> (7 - x % 8)) & 1:
					pixels[y * width + x] |= 1 << plane_index
	return bytes(pixels)


def main():
	if len(sys.argv) != 3:
		print(__doc__)
		return 1

	with open(sys.argv[1], "rb") as f:
		program = f.read()
	if len(program) != PROGRAM_SIZE or hashlib.md5(program).hexdigest() != PROGRAM_MD5:
		print("Unsupported program file: expected the Amiga v1.3 executable")
		return 1

	out = bytearray(b"GBGX")
	out += struct.pack("<HH", 1, len(IMAGES))
	for image_id, width, height, bytes_per_row, planes in IMAGES:
		out += struct.pack("<HHH", image_id, width, height)
		out += decode_planar(program, width, height, bytes_per_row, planes)

	with open(sys.argv[2], "wb") as f:
		f.write(out)
	print("Wrote %s (%d bytes)" % (sys.argv[2], len(out)))
	return 0


if __name__ == "__main__":
	sys.exit(main())
