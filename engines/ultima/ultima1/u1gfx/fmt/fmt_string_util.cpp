/* ScummVM - Graphic Adventure Engine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "ultima/ultima1/u1gfx/fmt/fmt_string_util.h"

namespace Ultima {
namespace Ultima1 {

// ---------------------------------------------------------------------------
// Decode one UTF-8 codepoint from *p, advance *p past it.
// Returns 0xFFFFFFFF on encoding error.
// ---------------------------------------------------------------------------
static uint32 decodeUtf8(const byte **p) {
	uint32 cp;
	byte b = *(*p)++;

	if (b < 0x80) {
		cp = b;
	} else if ((b & 0xE0) == 0xC0) {
		cp = b & 0x1F;
		cp = (cp << 6) | (*(*p)++ & 0x3F);
	} else if ((b & 0xF0) == 0xE0) {
		cp = b & 0x0F;
		cp = (cp << 6) | (*(*p)++ & 0x3F);
		cp = (cp << 6) | (*(*p)++ & 0x3F);
	} else if ((b & 0xF8) == 0xF0) {
		cp = b & 0x07;
		cp = (cp << 6) | (*(*p)++ & 0x3F);
		cp = (cp << 6) | (*(*p)++ & 0x3F);
		cp = (cp << 6) | (*(*p)++ & 0x3F);
	} else {
		cp = 0xFFFFFFFF;
	}
	return cp;
}

// ---------------------------------------------------------------------------
// Convert a JIS X 0208 codepoint (row/col 1-based) to Shift-JIS two bytes.
// row: 1-94, col: 1-94
// ---------------------------------------------------------------------------
static void jisToSjis(uint32 row, uint32 col, byte &hi, byte &lo) {
	if (row % 2 == 1) {
		hi = (byte)((row + 1) / 2 + (row < 63 ? 0x70 : 0xB0));
		lo = (byte)(col + (col <= 63 ? 0x3F : 0x40));
	} else {
		hi = (byte)(row / 2 + (row < 63 ? 0x70 : 0xB0));
		lo = (byte)(col + 0x9E);
	}
}

// ---------------------------------------------------------------------------
// Convert Unicode codepoint to Shift-JIS bytes, appended to out.
// Only covers the BMP range needed for DARKNESS strings (JIS X 0208 subset).
// Unmapped codepoints are silently skipped.
// ---------------------------------------------------------------------------
static void cpToSjis(uint32 cp, Common::String &out) {
	// ASCII passthrough
	if (cp < 0x80) {
		out += (char)cp;
		return;
	}

	// PUA U+E000..U+E0FF -> custom font 0xEB 0xNN
	if (cp >= 0xE000 && cp <= 0xE0FF) {
		out += (char)0xEB;
		out += (char)(cp & 0xFF);
		return;
	}

	// Unicode -> JIS X 0208 row/col via well-known offset tables.
	// We use the standard Unicode->SJIS mapping for the ranges present
	// in DARKNESS strings. The mapping is: look up the Unicode->JIS
	// row/col pair, then convert to SJIS.
	//
	// Rather than embed a full 7000-entry table, we cover the blocks
	// actually used: hiragana, katakana, CJK unified ideographs (common),
	// and a handful of punctuation/symbols.

	uint32 row = 0, col = 0;

	if (cp >= 0x3041 && cp <= 0x3096) {
		// Hiragana: U+3041 = JIS row 4 col 2
		uint32 off = cp - 0x3041;
		row = 4; col = 2 + off;
	} else if (cp >= 0x30A1 && cp <= 0x30F6) {
		// Katakana: U+30A1 = JIS row 5 col 2
		uint32 off = cp - 0x30A1;
		row = 5; col = 2 + off;
	} else if (cp >= 0x4E00 && cp <= 0x9FFF) {
		// CJK unified ideographs: mapped via JIS X 0208 level-1/2.
		// This range requires a lookup table; emit a placeholder '?' for
		// now — the Ghidra script will supply the actual SJIS bytes
		// directly so this path is only hit for hand-authored strings.
		out += '?';
		return;
	} else {
		// Punctuation / symbols used in DARKNESS strings
		switch (cp) {
		case 0x3000: row =  1; col =  1; break; // ideographic space
		case 0x3001: row =  1; col =  2; break; // 、
		case 0x3002: row =  1; col =  3; break; // 。
		case 0xFF01: row =  1; col = 15; break; // ！
		case 0xFF1F: row =  1; col = 16; break; // ？
		case 0xFF1A: row =  1; col = 17; break; // ：
		case 0xFF0D: row =  1; col = 18; break; //－
		case 0x300C: row =  1; col = 20; break; // 「
		case 0x300D: row =  1; col = 21; break; // 」
		default:
			out += '?';
			return;
		}
	}

	if (row && col) {
		byte hi, lo;
		jisToSjis(row, col, hi, lo);
		out += (char)hi;
		out += (char)lo;
	}
}

Common::String fmtUtf8ToSjis(const char *utf8) {
	Common::String out;
	const byte *p = (const byte *)utf8;

	while (*p) {
		uint32 cp = decodeUtf8(&p);
		if (cp == 0xFFFFFFFF)
			break;
		cpToSjis(cp, out);
	}

	return out;
}

} // End of namespace Ultima1
} // End of namespace Ultima
