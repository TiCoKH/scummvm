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
Build poolradamigasound.dat from the Amiga Pool of Radiance v1.3 executable.

Music: the songs, patterns, instruments, envelopes, samples and player
state of the music player ("(c)Wally Beben 1988 music on the Amiga") are
hunk 28 of the "program" file (loaded at 0x2611A8). All relocations of that
hunk point into the hunk itself, so its pointers in the file are offsets
within it; engines/goldbox/sound/amiga_music.cpp uses them as they are.

Sound effects: the driver in hunk 33 (SOUND_Play/PAULA_*, loaded at
0x2893D4) has a period table, 15 effects of 8 bytes and the info of 13
samples; the samples are hunk 29 (loaded at 0x274450).

Everything is kept, including what v1.3 never plays: effects 0, 1, 2, 7
and 14 (and samples 0, 1, 2 and 7, used only by them) and any pattern or
instrument of the music hunk that songs 1-6 do not reach. The driver's
sample pointers 13 and 14 have no info entry and point past the samples
(to the mouse pointer sprites of hunk 30), so they are not sounds. No other
code of the program uses the audio hardware.

Usage: create_amiga_sound.py <path to data/program> <output .dat>

File format (all integers little-endian unless noted):
  uint32 BE  magic 'GBAM'
  uint16     version (2)
  uint16     song count
  uint32     load address of the music hunk in the original (0x2611A8)
  uint32     music data size
  byte       music data[size]   hunk 28 as stored in the program
  uint16     period count
  uint16     effect count
  uint16     sample count
  uint16     periods[period count]
  per effect: byte sample, pitch, param, volume, mode, duration, decay,
              repeats (0xFF: plays until stopped)
  per sample: uint32 offset in the sample data, uint16 length, repeat
              offset, repeat length (bytes), byte loop flag, byte 0
  uint32     sample data size
  byte       sample data[size]  hunk 29, 8-bit signed PCM
"""

import hashlib
import struct
import sys

PROGRAM_SIZE = 459028
PROGRAM_MD5 = "0b19c9ec015fb0d0a29cd67d8275eee9"

MUSIC_HUNK = 28
MUSIC_ADDRESS = 0x2611A8
MUSIC_SIZE = 0x132A8
SONG_COUNT = 6

SFX_SAMPLE_HUNK = 29
SFX_SAMPLE_SIZE = 0xE3AC
SFX_DRIVER_HUNK = 33
SFX_DRIVER_ADDRESS = 0x2893D4
SFX_PERIODS = 0x289948       # 60 words
SFX_EFFECTS = 0x289A36       # 15 x 8 bytes
SFX_SAMPLE_PTRS = 0x289AAE   # longs, offsets into hunk 29
SFX_SAMPLE_INFO = 0x289AEA   # 8 bytes: length, repeat offset, repeat length, loop
PERIOD_COUNT = 60
EFFECT_COUNT = 15
SAMPLE_COUNT = 13


def read_hunks(program):
	"""Hunk contents of an AmigaDOS program (without relocation)."""
	def long(pos):
		return struct.unpack(">I", program[pos:pos + 4])[0]
	pos = 4
	while long(pos):
		pos += 4 + long(pos) * 4
	pos += 4
	first, last = long(pos + 4), long(pos + 8)
	count = last - first + 1
	pos += 12 + count * 4
	hunks = [None] * count
	index = 0
	while pos < len(program) and index < count:
		block = long(pos) & 0x3FFFFFFF
		pos += 4
		if block in (0x3E9, 0x3EA):
			if hunks[index] is not None:
				index += 1
			size = long(pos) * 4
			hunks[index] = program[pos + 4:pos + 4 + size]
			pos += 4 + size
		elif block == 0x3EB:
			if hunks[index] is not None:
				index += 1
			hunks[index] = bytes(long(pos) * 4)
			pos += 4
		elif block == 0x3EC:
			while long(pos):
				pos += 8 + long(pos) * 4
			pos += 4
		elif block == 0x3F0:
			while long(pos):
				pos += 8 + long(pos) * 4
			pos += 4
		elif block == 0x3F1:
			pos += 4 + long(pos) * 4
		elif block == 0x3F2:
			index += 1
		else:
			raise ValueError("unknown hunk block 0x%x" % block)
	return hunks


def main():
	if len(sys.argv) != 3:
		print(__doc__)
		return 1

	with open(sys.argv[1], "rb") as f:
		program = f.read()
	if len(program) != PROGRAM_SIZE or hashlib.md5(program).hexdigest() != PROGRAM_MD5:
		print("Unsupported program file: expected the Amiga v1.3 executable")
		return 1

	hunks = read_hunks(program)
	music = hunks[MUSIC_HUNK]
	samples = hunks[SFX_SAMPLE_HUNK]
	driver = hunks[SFX_DRIVER_HUNK]
	if len(music) != MUSIC_SIZE or len(samples) != SFX_SAMPLE_SIZE:
		print("Unexpected hunk layout in the program file")
		return 1

	def driver_bytes(address, size):
		offset = address - SFX_DRIVER_ADDRESS
		return driver[offset:offset + size]

	out = bytearray(b"GBAM")
	out += struct.pack("<HHII", 2, SONG_COUNT, MUSIC_ADDRESS, len(music))
	out += music

	out += struct.pack("<HHH", PERIOD_COUNT, EFFECT_COUNT, SAMPLE_COUNT)
	for i in range(PERIOD_COUNT):
		out += struct.pack("<H", struct.unpack(">H", driver_bytes(SFX_PERIODS + i * 2, 2))[0])
	out += driver_bytes(SFX_EFFECTS, EFFECT_COUNT * 8)
	for i in range(SAMPLE_COUNT):
		# Relocated against hunk 29 in the original: offsets in the file.
		offset = struct.unpack(">I", driver_bytes(SFX_SAMPLE_PTRS + i * 4, 4))[0]
		length, rep_offset, rep_length, loop = struct.unpack(
			">4H", driver_bytes(SFX_SAMPLE_INFO + i * 8, 8))
		if offset + length > len(samples):
			print("Sample %d is outside the sample hunk" % i)
			return 1
		out += struct.pack("<IHHHBB", offset, length, rep_offset, rep_length, loop & 0xFF, 0)
	out += struct.pack("<I", len(samples))
	out += samples

	with open(sys.argv[2], "wb") as f:
		f.write(out)
	print("Wrote %s (%d bytes)" % (sys.argv[2], len(out)))
	return 0


if __name__ == "__main__":
	sys.exit(main())
