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

#include "goldbox/gfx/backend/pc98_gfx_backend.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "engines/util.h"
#include "graphics/palette.h"
#include "graphics/paletteman.h"
#include "goldbox/data/daxfilemanager.h"
#include "goldbox/gfx/backend/dos_gfx_backend.h"
#include "goldbox/gfx/dax_font.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/data/daxheadercontainer.h"
#include "goldbox/data/format/pc98_dax_format.h"
#include "common/file.h"

namespace Goldbox {
namespace Gfx {

void Pc98GfxBackend::initScreen() {
	initGraphics(640, 400);
}

void Pc98GfxBackend::setupPalette() {
	// The 8 digital colors, in EGA index order: Pc98DaxFormat converts the
	// art to EGA indices and the engine names colors by EGA index. Each dark
	// EGA color shows as its bright digital color; 8 (dark gray) is the
	// opaque black of masked art.
	static const uint8 kEgaRgb[16][3] = {
		{ 0, 0, 0 },       { 0, 0, 255 },     { 0, 255, 0 },   { 0, 255, 255 },
		{ 255, 0, 0 },     { 255, 0, 255 },   { 255, 255, 0 }, { 255, 255, 255 },
		{ 0, 0, 0 },       { 0, 0, 255 },     { 0, 255, 0 },   { 0, 255, 255 },
		{ 255, 0, 0 },     { 255, 0, 255 },   { 255, 255, 0 }, { 255, 255, 255 }
	};
	Graphics::Palette pal(16);
	for (uint i = 0; i < 16; ++i)
		pal.set(i, kEgaRgb[i][0], kEgaRgb[i][1], kEgaRgb[i][2]);
	g_system->getPaletteManager()->setPalette(pal);
}

const GfxBackend::ArtColors &Pc98GfxBackend::artColors() const {
	// The PC-98 port is built from the same Pascal tables as DOS; these are
	// unverified against its redrawn art.
	static const ArtColors kColors = {
		13, 8, 8, 13, 8,
		{ 0x91, 0xA2, 0xB3, 0xC4, 0xE6, 0xF7 }
	};
	return kColors;
}

static const uint8 kPc98FontBlock = 201;

Graphics::Font *Pc98GfxBackend::createFont(Data::DaxFileManager &dax) {
	// 8X8D block 201: the 177 DOS glyphs redrawn at 16x16, one bit per pixel.
	Data::DaxBlock8x8D *block = dynamic_cast<Data::DaxBlock8x8D *>(
		dax.get8x8d().getBlockById(kPc98FontBlock));
	if (!block)
		error("Failed to load font block %d from 8x8d container", kPc98FontBlock);
	return new DaxFont(block, 16);
}

/** The single block of a PC-98 DAX file, unpacked. */
static bool loadSingleBlock(const Data::DaxFileManager &dax,
		const Common::Path &path, Common::Array<uint8> &out) {
	Common::File file;
	if (!file.open(path))
		return false;
	Data::DaxHeaderContainer headers;
	dax.getFormat().parseHeaders(file, headers);
	if (headers.getHeaders().empty())
		return false;
	const Data::DaxHeader &header = headers.getHeaders()[0];
	Common::Array<uint8> packed(header.compSize);
	file.seek(headers.getFileDataOffset() + header.offset);
	if (file.read(packed.data(), packed.size()) != packed.size())
		return false;
	out.resize(header.rawSize);
	return dax.getFormat().unpack(packed, out);
}

Pic *Pc98GfxBackend::loadTitlePicture(Data::DaxFileManager &dax, int index) const {
	// VIEW_Title: TITLE<n>B/R/G.DAX hold the blue, red and green planes of a
	// 640x400 screen, 80 bytes per row.
	static const char kPlanes[3] = { 'B', 'R', 'G' };
	static const int kWidth = 640;
	static const int kHeight = 400;
	Common::Array<uint8> planes[3];
	for (int p = 0; p < 3; ++p) {
		const Common::Path path(Common::String::format("TITLE%d%c.DAX",
			index, kPlanes[p]));
		if (!loadSingleBlock(dax, path, planes[p])
				|| planes[p].size() < kWidth / 8 * kHeight) {
			warning("PC-98 title plane %s could not be loaded",
				path.toString().c_str());
			return nullptr;
		}
	}

	Pic *pic = new Pic(kWidth, kHeight);
	for (int y = 0; y < kHeight; ++y) {
		for (int x = 0; x < kWidth; ++x) {
			const int offset = y * (kWidth / 8) + x / 8;
			const int bit = 7 - (x & 7);
			const uint8 color = ((planes[0][offset] >> bit) & 1)
				| (((planes[1][offset] >> bit) & 1) << 1)
				| (((planes[2][offset] >> bit) & 1) << 2);
			pic->setPixel(x, y, Data::Pc98DaxFormat::digitalToEga(color));
		}
	}
	return pic;
}

uint8 Pc98GfxBackend::mapChar(uint8 chr) const {
	// Same glyph order as the DOS font.
	return DosGfxBackend::fontIndex(chr);
}

} // namespace Gfx
} // namespace Goldbox
