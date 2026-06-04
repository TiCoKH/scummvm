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

#include "goldbox/poolrad/console.h"
#include "goldbox/poolrad/poolrad.h"

#include "common/str.h"
#include "common/file.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/data/daxblockcontainer.h"
#include "goldbox/gfx/dax_tile.h"
#include "goldbox/gfx/first_person_renderer.h"
#include "goldbox/gfx/pic.h"
#include "image/bmp.h"

namespace {

static const uint32 kColorBlack = 0;
static const uint32 kColorGrid = 1;
static const uint32 kColorRoom = 8;
static const uint32 kColorWall = 7;
static const uint32 kColorCursor = 15;
static const uint32 kDoorColors[4] = {0, 15, 14, 12};

static void blitPic(Graphics::Screen *screen, const Goldbox::Gfx::Pic *pic,
		int x, int y) {
	if (!screen || !pic)
		return;
	pic->draw(screen, x, y);
}

static void renderGeoMap(Graphics::Screen *screen, Goldbox::Surface &surface,
		const Goldbox::Data::DaxBlockGeo &geo, uint16 cursorX,
		uint16 cursorY, uint8 cursorDir, uint8 mapId) {
	const int cellSize = 10;
	const int wallThickness = 2;
	const int mapLeft = 8;
	const int mapTop = 8;
	const int mapSizePx = Goldbox::Data::DaxBlockGeo::GRID_SIZE * cellSize;
	const int infoCharX = 24;

	screen->fillRect(Common::Rect(0, 0, screen->w, screen->h), kColorBlack);

	for (int row = 0; row < Goldbox::Data::DaxBlockGeo::GRID_SIZE; ++row) {
		for (int col = 0; col < Goldbox::Data::DaxBlockGeo::GRID_SIZE; ++col) {
			const int x = mapLeft + col * cellSize;
			const int y = mapTop + row * cellSize;
			const Goldbox::Data::DaxBlockGeo::MapCell cell = geo.cellAt(row, col);

			uint32 fill = (cell.event != 0) ? 2 : kColorRoom;
			screen->fillRect(Common::Rect(x, y, x + cellSize, y + cellSize), fill);

			if (cell.wallType[Goldbox::Data::DaxBlockGeo::NORTH] != 0)
				screen->fillRect(Common::Rect(x, y, x + cellSize, y + wallThickness),
					kColorWall);
			if (cell.wallType[Goldbox::Data::DaxBlockGeo::EAST] != 0)
				screen->fillRect(Common::Rect(x + cellSize - wallThickness, y,
					x + cellSize, y + cellSize), kColorWall);
			if (cell.wallType[Goldbox::Data::DaxBlockGeo::SOUTH] != 0)
				screen->fillRect(Common::Rect(x, y + cellSize - wallThickness,
					x + cellSize, y + cellSize), kColorWall);
			if (cell.wallType[Goldbox::Data::DaxBlockGeo::WEST] != 0)
				screen->fillRect(Common::Rect(x, y, x + wallThickness, y + cellSize),
					kColorWall);

			for (int dir = 0; dir < 4; ++dir) {
				const uint8 doorState = cell.doorState[dir];
				if (doorState == 0)
					continue;

				switch (dir) {
				case Goldbox::Data::DaxBlockGeo::NORTH:
					screen->fillRect(Common::Rect(x + 2, y, x + cellSize - 2,
						y + wallThickness), kDoorColors[doorState]);
					break;
				case Goldbox::Data::DaxBlockGeo::EAST:
					screen->fillRect(Common::Rect(x + cellSize - wallThickness,
						y + 2, x + cellSize, y + cellSize - 2),
						kDoorColors[doorState]);
					break;
				case Goldbox::Data::DaxBlockGeo::SOUTH:
					screen->fillRect(Common::Rect(x + 2,
						y + cellSize - wallThickness, x + cellSize - 2,
						y + cellSize), kDoorColors[doorState]);
					break;
				case Goldbox::Data::DaxBlockGeo::WEST:
					screen->fillRect(Common::Rect(x, y + 2, x + wallThickness,
						y + cellSize - 2), kDoorColors[doorState]);
					break;
				}
			}
		}
	}

	for (int i = 0; i <= Goldbox::Data::DaxBlockGeo::GRID_SIZE; ++i) {
		const int x = mapLeft + i * cellSize;
		const int y = mapTop + i * cellSize;
		screen->drawLine(x, mapTop, x, mapTop + mapSizePx, kColorGrid);
		screen->drawLine(mapLeft, y, mapLeft + mapSizePx, y, kColorGrid);
	}

	if (cursorX < Goldbox::Data::DaxBlockGeo::GRID_SIZE
			&& cursorY < Goldbox::Data::DaxBlockGeo::GRID_SIZE) {
		const int cx = mapLeft + cursorX * cellSize + cellSize / 2;
		const int cy = mapTop + cursorY * cellSize + cellSize / 2;
		screen->drawLine(cx - 3, cy, cx + 3, cy, kColorCursor);
		screen->drawLine(cx, cy - 3, cx, cy + 3, kColorCursor);

		switch ((cursorDir / 2) & 0x03) {
		case 0:
			screen->drawLine(cx, cy, cx, cy - 5, kColorCursor);
			break;
		case 1:
			screen->drawLine(cx, cy, cx + 5, cy, kColorCursor);
			break;
		case 2:
			screen->drawLine(cx, cy, cx, cy + 5, kColorCursor);
			break;
		default:
			screen->drawLine(cx, cy, cx - 5, cy, kColorCursor);
			break;
		}
	}

	surface.writeStringC(0, 0, 15,
		Common::String::format("GEO map %u", (unsigned)mapId));
	surface.writeStringC(infoCharX, 1, 15,
		Common::String::format("pos %u,%u dir %u",
			(unsigned)cursorX, (unsigned)cursorY, (unsigned)cursorDir));
	if (cursorX < Goldbox::Data::DaxBlockGeo::GRID_SIZE
			&& cursorY < Goldbox::Data::DaxBlockGeo::GRID_SIZE) {
		const Goldbox::Data::DaxBlockGeo::MapCell cell =
			geo.cellAt(cursorY, cursorX);
		surface.writeStringC(infoCharX, 3, 10,
			Common::String::format("N:%u E:%u S:%u W:%u",
				(unsigned)cell.wallType[0], (unsigned)cell.wallType[1],
				(unsigned)cell.wallType[2], (unsigned)cell.wallType[3]));
		surface.writeStringC(infoCharX, 4, 10,
			Common::String::format("doors %u %u %u %u",
				(unsigned)cell.doorState[0], (unsigned)cell.doorState[1],
				(unsigned)cell.doorState[2], (unsigned)cell.doorState[3]));
		surface.writeStringC(infoCharX, 5, 10,
			Common::String::format("event %u", (unsigned)cell.event));
	}
	surface.writeStringC(infoCharX, 7, 14,
		"door colors: open/lock/wiz");
	const int legendX = infoCharX * 8;
	screen->fillRect(Common::Rect(legendX, 72, legendX + 12, 80), kDoorColors[1]);
	screen->fillRect(Common::Rect(legendX + 18, 72, legendX + 30, 80),
		kDoorColors[2]);
	screen->fillRect(Common::Rect(legendX + 36, 72, legendX + 48, 80),
		kDoorColors[3]);
}

static void renderWalldefAtlas(Graphics::Screen *screen,
		Goldbox::Surface &surface,
		const Goldbox::Gfx::WallSurfaceSet &surfaceSet, uint8 wallType) {
	screen->fillRect(Common::Rect(0, 0, screen->w, screen->h), kColorBlack);
	surface.writeStringC(0, 0, 15,
		Common::String::format("Wall type %u", (unsigned)wallType));

	struct RegionPlacement {
		Goldbox::Data::WalldefRegionId id;
		const char *label;
		int x;
		int y;
	};

	static const RegionPlacement kRegions[] = {
		{Goldbox::Data::WalldefRegionId::FAR_FORWARD, "far f", 8, 16},
		{Goldbox::Data::WalldefRegionId::FAR_LEFT, "far l", 32, 16},
		{Goldbox::Data::WalldefRegionId::FAR_RIGHT, "far r", 48, 16},
		{Goldbox::Data::WalldefRegionId::FAR_FILLER, "fill", 72, 16},
		{Goldbox::Data::WalldefRegionId::MED_FORWARD, "med f", 8, 64},
		{Goldbox::Data::WalldefRegionId::MED_LEFT, "med l", 40, 64},
		{Goldbox::Data::WalldefRegionId::MED_RIGHT, "med r", 64, 64},
		{Goldbox::Data::WalldefRegionId::CLOSE_FORWARD, "near f", 8, 136},
		{Goldbox::Data::WalldefRegionId::CLOSE_LEFT, "near l", 72, 136},
		{Goldbox::Data::WalldefRegionId::CLOSE_RIGHT, "near r", 96, 136}
	};

	for (uint i = 0; i < ARRAYSIZE(kRegions); ++i) {
		const Goldbox::Gfx::Pic *pic = surfaceSet.region(kRegions[i].id);
		surface.writeStringC(kRegions[i].x / 8, (kRegions[i].y / 8) - 1, 10,
			kRegions[i].label);
		blitPic(screen, pic, kRegions[i].x, kRegions[i].y);
		if (pic)
			screen->frameRect(Common::Rect(kRegions[i].x, kRegions[i].y,
				kRegions[i].x + pic->w, kRegions[i].y + pic->h), kColorGrid);
	}
}

static void renderFpView(Graphics::Screen *screen, Goldbox::Surface &surface,
		const Goldbox::Data::DaxBlockGeo &geo,
		const Goldbox::Gfx::WalldefSlotCache &wallSlots,
		uint16 x, uint16 y, uint8 dir, uint8 mapId) {
	screen->fillRect(Common::Rect(0, 0, screen->w, screen->h), kColorBlack);
	Goldbox::Gfx::FirstPersonRenderer::draw3dWorld(screen, dir, x, y, geo,
		wallSlots);
	surface.writeStringC(0, 0, 15,
		Common::String::format("FP map %u pos %u,%u dir %u",
			(unsigned)mapId, (unsigned)x, (unsigned)y, (unsigned)dir));
	surface.writeStringC(0, 15, 10,
		"Use geo command for top-down verification");
}

static bool parseUintArg(const char *arg, uint &value) {
	if (!arg)
		return false;
	char *endPtr = nullptr;
	long parsed = strtol(arg, &endPtr, 0);
	if (!endPtr || *endPtr != '\0' || parsed < 0)
		return false;
	value = static_cast<uint>(parsed);
	return true;
}

} // namespace

namespace Goldbox {
namespace Poolrad {

Console::Console() : Goldbox::Console() {
	registerCmd("font", WRAP_METHOD(Console, cmdFont));
	registerCmd("geo", WRAP_METHOD(Console, cmdGeo));
	registerCmd("walldef", WRAP_METHOD(Console, cmdWalldef));
	registerCmd("fpview", WRAP_METHOD(Console, cmdFpview));
	registerCmd("walldefstate", WRAP_METHOD(Console, cmdWalldefstate));
	registerCmd("dumpPic", WRAP_METHOD(Console, cmdDumpPic));
}

bool Console::cmdFont(int argc, const char **argv) {
	auto *font = g_engine->_font;
	Graphics::Screen *screen = g_engine->getScreen();

	screen->clear();
	for (uint i = 0; i < 177; ++i) {
		font->drawChar(screen, i, (i % 16) * 16, (i / 16) * 16, 255);
	}

	screen->update();
	return false;
}

bool Console::cmdGeo(int argc, const char **argv) {
	Graphics::Screen *screen = g_engine->getScreen();
	if (!screen) {
		debugPrintf("No screen available\n");
		return true;
	}

	uint mapId = g_engine->getLegacySharedRuntimeState().byteMapId;
	uint x = 8;
	uint y = 8;
	uint dir = 0;
	uint16 activeX = 0;
	uint16 activeY = 0;
	uint8 activeDir = 0;
	if (g_engine->getActiveMapPosition(activeX, activeY, activeDir)) {
		x = activeX;
		y = activeY;
		dir = activeDir;
	} else {
		x = 8;
		y = 8;
		dir = 0;
	}

	if (argc >= 2 && !parseUintArg(argv[1], mapId)) {
		debugPrintf("Usage: geo [mapId] [x y dir]\n");
		return true;
	}
	if (argc >= 5) {
		if (!parseUintArg(argv[2], x) || !parseUintArg(argv[3], y)
				|| !parseUintArg(argv[4], dir)) {
			debugPrintf("Usage: geo [mapId] [x y dir]\n");
			return true;
		}
		dir &= 0x07;
	}

	Data::DaxBlockGeo *geo = g_engine->getGeoBlockById((uint8)mapId);
	if (!geo) {
		debugPrintf("GEO block %u not found\n", (unsigned)mapId);
		return true;
	}

	Surface s(*screen, Common::Rect(0, 0, screen->w, screen->h));
	renderGeoMap(screen, s, *geo, (uint16)x, (uint16)y, (uint8)dir,
		(uint8)mapId);
	screen->update();
	debugPrintf("Rendered GEO map %u at %u,%u dir %u\n",
		(unsigned)mapId, (unsigned)x, (unsigned)y, (unsigned)dir);
	return false;
}

bool Console::cmdWalldef(int argc, const char **argv) {
	Graphics::Screen *screen = g_engine->getScreen();
	if (!screen) {
		debugPrintf("No screen available\n");
		return true;
	}

	uint wallType = 1;
	if (argc >= 2 && !parseUintArg(argv[1], wallType)) {
		debugPrintf("Usage: walldef [wallType]\n");
		return true;
	}
	if (wallType == 0 || wallType > 15) {
		debugPrintf("wallType must be 1..15\n");
		return true;
	}

	const Gfx::WallSurfaceSet *surfaceSet =
		g_engine->getWalldefSlotCache().surfaceSetForWallType((uint8)wallType);
	if (!surfaceSet) {
		debugPrintf("No walldef surfaces loaded for wall type %u\n",
			(unsigned)wallType);
		return true;
	}

	Surface s(*screen, Common::Rect(0, 0, screen->w, screen->h));
	renderWalldefAtlas(screen, s, *surfaceSet, (uint8)wallType);
	screen->update();
	debugPrintf("Rendered walldef atlas for wall type %u\n",
		(unsigned)wallType);
	return false;
}

bool Console::cmdFpview(int argc, const char **argv) {
	Graphics::Screen *screen = g_engine->getScreen();
	if (!screen) {
		debugPrintf("No screen available\n");
		return true;
	}

	uint mapId = g_engine->getLegacySharedRuntimeState().byteMapId;
	uint x = 8;
	uint y = 8;
	uint dir = 0;
	uint16 activeX = 0;
	uint16 activeY = 0;
	uint8 activeDir = 0;
	if (g_engine->getActiveMapPosition(activeX, activeY, activeDir)) {
		x = activeX;
		y = activeY;
		dir = activeDir;
	} else {
		x = 8;
		y = 8;
		dir = 0;
	}

	if (argc >= 2 && !parseUintArg(argv[1], mapId)) {
		debugPrintf("Usage: fpview [mapId] [x y dir]\n");
		return true;
	}
	if (argc >= 5) {
		if (!parseUintArg(argv[2], x) || !parseUintArg(argv[3], y)
				|| !parseUintArg(argv[4], dir)) {
			debugPrintf("Usage: fpview [mapId] [x y dir]\n");
			return true;
		}
		if (dir <= 3)
			dir *= 2;
		else
			dir &= 0x07;
	}

	Data::DaxBlockGeo *geo = g_engine->getGeoBlockById((uint8)mapId);
	if (!geo) {
		debugPrintf("GEO block %u not found\n", (unsigned)mapId);
		return true;
	}

	Surface s(*screen, Common::Rect(0, 0, screen->w, screen->h));
	renderFpView(screen, s, *geo, g_engine->getWalldefSlotCache(),
		(uint16)x, (uint16)y, (uint8)dir, (uint8)mapId);
	screen->update();
	debugPrintf("Rendered first-person preview for map %u at %u,%u dir %u\n",
		(unsigned)mapId, (unsigned)x, (unsigned)y, (unsigned)dir);
	return false;
}

bool Console::cmdWalldefstate(int argc, const char **argv) {
	const Gfx::Tile8x8Cache &tileCache = g_engine->getTileCache();
	uint32 totalLoadedTiles = 0;
	uint loadedTileSlots = 0;

	debugPrintf("Tile cache slot state (slots 0 and 4 are fixed):\n");
	for (int slot = 0; slot < Gfx::Tile8x8Cache::kSlotCount; ++slot) {
		const Gfx::DaxTile *tiles = tileCache.getSlot(slot);
		const bool fixedSlot = (slot == 0 || slot == 4);
		const uint16 firstId =
			Gfx::Tile8x8Cache::firstGlobalTileIdForSlot(slot);
		const uint16 lastId =
			Gfx::Tile8x8Cache::lastGlobalTileIdForSlot(slot);

		if (!tiles) {
			debugPrintf("  tile slot %d%s: empty (global %u-%u)\n",
				slot, fixedSlot ? " [fixed]" : "",
				(unsigned)firstId, (unsigned)lastId);
			continue;
		}

		const uint32 tileCount = tiles->getTileCount();
		totalLoadedTiles += tileCount;
		++loadedTileSlots;
		debugPrintf("  tile slot %d%s: loaded (%u tiles, global %u-%u)\n",
			slot, fixedSlot ? " [fixed]" : "", (unsigned)tileCount,
			(unsigned)firstId, (unsigned)lastId);
	}

	debugPrintf("  tile totals: %u/%u slots loaded, %u tiles loaded\n",
		(unsigned)loadedTileSlots,
		(unsigned)Gfx::Tile8x8Cache::kSlotCount,
		(unsigned)totalLoadedTiles);

	debugPrintf("Runtime walldef slot state:\n");
	for (int slot = 1; slot <= 3; ++slot) {
		DebugWallSetState state;
		if (!g_engine->getDebugWallSetState(slot, state)) {
			debugPrintf("  slot %d: unavailable\n", slot);
			continue;
		}

		if (!state.loaded) {
			debugPrintf("  slot %d: empty\n", slot);
			continue;
		}

		const Gfx::DaxTile *slotTiles = tileCache.getSlot(slot);
		const uint32 slotTileCount = slotTiles ? slotTiles->getTileCount() : 0;

		debugPrintf("  slot %d: walldef=%u tile8x8=%u chunk=%u tiles=%u\n",
			slot,
			(unsigned)state.walldefBlockId,
			(unsigned)state.tileBlockId,
			(unsigned)state.chunkIndex,
			(unsigned)slotTileCount);
	}

	return true;
}


bool Console::cmdDumpPic(int argc, const char **argv) {
	if (argc < 2) {
		debugPrintf("Usage: dumpPic <blockId> [container]\n");
		debugPrintf("  container: PIC (default), HEAD, BODY, CPIC, TITLE\n");
		return true;
	}

	uint blockId = 0;
	if (!parseUintArg(argv[1], blockId)) {
		debugPrintf("Invalid block ID\n");
		return true;
	}

	Common::String containerName = "PIC";
	if (argc >= 3)
		containerName = argv[2];
	containerName.toUppercase();

	::Goldbox::Data::DaxBlockContainer *container = nullptr;
	if (containerName == "PIC")
		container = &g_engine->getDaxPic();
	else if (containerName == "HEAD")
		container = &g_engine->getDaxHead();
	else if (containerName == "BODY")
		container = &g_engine->getDaxBody();
	else if (containerName == "CPIC")
		container = &g_engine->getDaxCPic();
	else if (containerName == "TITLE")
		container = &g_engine->getDaxTitle();
	else {
		debugPrintf("Unknown container: %s\n", containerName.c_str());
		return true;
	}

	::Goldbox::Data::DaxBlock *rawBlock = container->getBlockById((uint8)blockId);
	if (!rawBlock) {
		debugPrintf("Block %u not found in %s\n", blockId, containerName.c_str());
		return true;
	}

	static const byte kEgaPalette[16 * 3] = {
		0x00,0x00,0x00, 0x00,0x00,0xAA, 0x00,0xAA,0x00, 0x00,0xAA,0xAA,
		0xAA,0x00,0x00, 0xAA,0x00,0xAA, 0xAA,0x55,0x00, 0xAA,0xAA,0xAA,
		0x55,0x55,0x55, 0x55,0x55,0xFF, 0x55,0xFF,0x55, 0x55,0xFF,0xFF,
		0xFF,0x55,0x55, 0xFF,0x55,0xFF, 0xFF,0xFF,0x55, 0xFF,0xFF,0xFF
	};

	if (containerName == "PIC") {
		// PIC container uses EGAPIC (DaxBlockSprit with XOR decode).
		::Goldbox::Data::DaxBlockSprit *spritBlock =
			dynamic_cast< ::Goldbox::Data::DaxBlockSprit *>(rawBlock);
		if (!spritBlock || spritBlock->frameCount() < 1) {
			debugPrintf("Block %u: failed to decode as EGAPIC\n", blockId);
			return true;
		}

		const ::Goldbox::Data::DaxBlockSprit::FrameInfo *f0 = spritBlock->frameInfo(0);
		debugPrintf("%s block %u: %dx%d frameCount=%d (EGAPIC)\n",
			containerName.c_str(), blockId,
			f0 ? (int)f0->width : 0, f0 ? (int)f0->height : 0,
			spritBlock->frameCount());

		for (int f = 0; f < spritBlock->frameCount(); ++f) {
			::Goldbox::Gfx::Pic *pic = ::Goldbox::Gfx::Pic::readEgaPicFrame(spritBlock, f);
			if (!pic) {
				debugPrintf("  frame %d: decode failed\n", f);
				continue;
			}

			Common::String filename = Common::String::format("%s_%u_frame%d.bmp",
				containerName.c_str(), blockId, f);
			Common::DumpFile outFile;
			if (outFile.open(Common::Path(filename))) {
				Image::writeBMP(outFile, *pic, kEgaPalette, 16);
				outFile.close();
				debugPrintf("  frame %d: saved %s (%dx%d)\n", f, filename.c_str(),
					pic->w, pic->h);
			} else {
				debugPrintf("  frame %d: failed to open %s\n", f, filename.c_str());
			}
			delete pic;
		}
	} else {
		// HEAD, BODY, CPIC, TITLE use old DaxBlockPic format.
		::Goldbox::Data::DaxBlockPic *picBlock =
			dynamic_cast< ::Goldbox::Data::DaxBlockPic *>(rawBlock);
		if (!picBlock) {
			debugPrintf("Block %u is not a PIC block\n", blockId);
			return true;
		}

		const int width = picBlock->width;
		const int height = picBlock->height;
		const int dataSize = (int)picBlock->_data.size();
		debugPrintf("%s block %u: %dx%d frameCount=%d dataSize=%d\n",
			containerName.c_str(), blockId, width, height,
			picBlock->frameCount, dataSize);

		if (width <= 0 || height <= 0) {
			debugPrintf("Invalid dimensions\n");
			return true;
		}

		const int frameSize = (width * height) / 2;
		const int frames = dataSize / frameSize;
		debugPrintf("frameSize=%d derivedFrames=%d\n", frameSize, frames);

		for (int f = 0; f < frames && f < 16; ++f) {
			::Goldbox::Gfx::Pic *pic = ::Goldbox::Gfx::Pic::readFrame(picBlock, f);
			if (!pic) {
				debugPrintf("  frame %d: decode failed\n", f);
				continue;
			}

			Common::String filename = Common::String::format("%s_%u_frame%d.bmp",
				containerName.c_str(), blockId, f);
			Common::DumpFile outFile;
			if (outFile.open(Common::Path(filename))) {
				Image::writeBMP(outFile, *pic, kEgaPalette, 16);
				outFile.close();
				debugPrintf("  frame %d: saved %s (%dx%d)\n", f, filename.c_str(),
					pic->w, pic->h);
			} else {
				debugPrintf("  frame %d: failed to open %s\n", f, filename.c_str());
			}
			delete pic;
		}
	}

	return true;
}

} // namespace Poolrad
} // namespace Goldbox
