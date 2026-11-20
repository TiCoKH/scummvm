/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "wizardry/gfx/wide_font.h"

#ifdef USE_SJIS
#include "common/platform.h"
#include "graphics/sjis.h"

namespace Wizardry {

namespace {

class SjisWideFont : public WideFont {
private:
	Graphics::FontSJIS *_font;

public:
	explicit SjisWideFont(Graphics::FontSJIS *font) : _font(font) {}
	~SjisWideFont() override { delete _font; }

	bool drawChar(uint16 code, byte *dst, uint pitch, uint width, uint height) const override {
		enum {
			kSize = 16
		};
		byte glyph[kSize * kSize];
		memset(glyph, 0, sizeof(glyph));

		// FontSJIS takes the lead byte low and the trail byte high
		const uint16 ch = (code >> 8) | ((code & 0xff) << 8);
		if (!_font->getCharWidth(ch))
			return false;
		_font->drawChar(glyph, ch, kSize, 1, 1, 0, kSize, kSize);

		// Into the tile; a tile of half the height takes each pair of rows
		// together, as the 640x200 screens have half the rows
		const uint step = (height < kSize) ? kSize / height : 1;
		for (uint y = 0; y < height; ++y) {
			for (uint x = 0; x < width; ++x) {
				byte ink = 0;
				for (uint k = 0; k < step && y * step + k < kSize; ++k)
					ink |= (x < kSize) ? glyph[(y * step + k) * kSize + x] : 0;
				dst[y * pitch + x] = ink ? 1 : 0;
			}
		}
		return true;
	}
};

} // End of anonymous namespace

WideFont *createPC98WideFont() {
	Graphics::FontSJIS *font = Graphics::FontSJIS::createFont(Common::kPlatformPC98);
	return font ? new SjisWideFont(font) : nullptr;
}

} // End of namespace Wizardry

#endif
