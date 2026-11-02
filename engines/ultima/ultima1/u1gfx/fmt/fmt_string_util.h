/* ScummVM - Graphic Adventure Engine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef ULTIMA_ULTIMA1_GFX_FMT_STRING_UTIL_H
#define ULTIMA_ULTIMA1_GFX_FMT_STRING_UTIL_H

#include "common/str.h"

namespace Ultima {
namespace Ultima1 {

/**
 * Convert a UTF-8 string from FMTownsResources to the raw byte sequence
 * expected by the DARKNESS text renderer.
 *
 * Encoding rules:
 *   - ASCII bytes (0x00-0x7F) pass through unchanged.
 *   - Standard UTF-8 multibyte sequences are converted to Shift-JIS.
 *   - PUA codepoints U+E000..U+E0FF (UTF-8: EE 80 80 .. EE 83 BF) are
 *     emitted as the two-byte custom-font sequence 0xEB 0xNN, where NN is
 *     the low byte of the codepoint (U+E01A -> 0xEB 0x1A).
 */
Common::String fmtUtf8ToSjis(const char *utf8);

} // End of namespace Ultima1
} // End of namespace Ultima

#endif
