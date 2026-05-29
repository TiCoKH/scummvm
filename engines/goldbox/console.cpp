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

#include "goldbox/events.h"
#include "goldbox/console.h"
#include "goldbox/engine.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/data/daxblockcontainer.h"
#include "goldbox/gfx/dax_tile.h"
#include "goldbox/gfx/walldef_surface_builder.h"
#include "goldbox/gfx/pic.h"
#include "common/file.h"
#include "graphics/palette.h"
#include "image/bmp.h"

namespace Goldbox {

Console::Console() : GUI::Debugger() {
	registerCmd("view",   WRAP_METHOD(Console, cmdView));
	registerCmd("showTile", WRAP_METHOD(Console, cmdShowTile));
	registerCmd("dumpTileCache", WRAP_METHOD(Console, cmdDumpTileCache));
	registerCmd("dumpWalldef", WRAP_METHOD(Console, cmdDumpWalldef));
	registerCmd("dumpWalldefRegion", WRAP_METHOD(Console, cmdDumpWalldefRegion));
	registerCmd("dumpWalldefAll", WRAP_METHOD(Console, cmdDumpWalldefAll));
}

Console::~Console() {
}

bool Console::cmdView(int argc, const char **argv) {
	if (argc == 2) {
		g_events->replaceView(argv[1]);
		return false;
	} else {
		debugPrintf("view <name>\n");
		return true;
	}
}

bool Console::cmdShowTile(int argc, const char **argv) {
	if (argc != 3) {
		debugPrintf("Usage: showTile <tiletype> <id>\n");
		debugPrintf("Tile types: 8x8d, bacpac, dungcom, randcom, sqrpaci\n");
		debugPrintf("Example: showTile 8x8d 201\n");
		return true;
	}

	Common::String tileType = argv[1];
	tileType.toLowercase();
	uint8 blockId = atoi(argv[2]);

	// Get the appropriate container based on tile type
	Data::DaxBlockContainer *container = nullptr;
	if (tileType == "8x8d") {
		container = &g_engine->getDax8x8d();
	} else if (tileType == "bacpac") {
		container = &g_engine->getDaxBacpac();
	} else if (tileType == "dungcom") {
		container = &g_engine->getDaxDungcom();
	} else if (tileType == "randcom") {
		container = &g_engine->getDaxRandcom();
	} else if (tileType == "sqrpaci") {
		container = &g_engine->getDaxSqrpaci();
	} else {
		debugPrintf("Unknown tile type: %s\n", tileType.c_str());
		debugPrintf("Valid types: 8x8d, bacpac, dungcom, randcom, sqrpaci\n");
		return true;
	}

	// Try to get the block
	Data::DaxBlock *block = container->getBlockById(blockId);
	if (!block) {
		debugPrintf("Block ID %d not found in tileset '%s'\n", blockId, tileType.c_str());
		debugPrintf("Available blocks in this tileset: ");
		Common::Array<uint8> ids = container->getBlockIds();
		for (uint i = 0; i < ids.size(); ++i) {
			debugPrintf("%d", ids[i]);
			if (i < ids.size() - 1) {
				debugPrintf(", ");
			}
		}
		debugPrintf("\n");
		return true;
	}

	// Cast to DaxBlock8x8D (all tile types use this)
	Data::DaxBlock8x8D *tileBlock = dynamic_cast<Data::DaxBlock8x8D*>(block);
	if (!tileBlock) {
		debugPrintf("Block %d is not a valid tile block\n", blockId);
		return true;
	}

	// Display tile information
	debugPrintf("Tile Info - Type: %s, ID: %d\n", tileType.c_str(), blockId);
	debugPrintf("  Dimensions: %dx%d\n", tileBlock->width, tileBlock->height);
	debugPrintf("  Item Count: %d\n", tileBlock->item_count);
	debugPrintf("  Data Size: %d bytes\n", tileBlock->_data.size());

	// Check if we can render
	if (!g_engine) {
		debugPrintf("Error: Engine not available for rendering\n");
		return true;
	}

	Graphics::Screen *screen = g_engine->getScreen();
	if (!screen) {
		debugPrintf("Error: Screen not available for rendering\n");
		debugPrintf("Tile data is valid but cannot display graphics.\n");
		return true;
	}

	debugPrintf("\nTile Information:\n");
	debugPrintf("================\n");
	debugPrintf("Type: %s\n", tileType.c_str());
	debugPrintf("Block ID: %d\n", blockId);
	debugPrintf("Dimensions: %dx%d pixels\n", tileBlock->width, tileBlock->height);
	debugPrintf("Item Count: %d tiles\n", tileBlock->item_count);
	debugPrintf("Data Size: %d bytes\n", tileBlock->_data.size());
	debugPrintf("Bytes per Tile: %d\n", (tileBlock->_data.size() / tileBlock->item_count));

	// Show hex dump of first tile
	debugPrintf("\nFirst tile data (hex):\n");
	int bytesPerTile = tileBlock->_data.size() / tileBlock->item_count;
	const uint8 *data = tileBlock->_data.data();

	for (int i = 0; i < MIN(bytesPerTile, 64); ++i) {
		if (i % 16 == 0) {
			debugPrintf("\n  %04X: ", i);
		}
		debugPrintf("%02X ", data[i]);
	}
	debugPrintf("\n");

	debugPrintf("\nTile block is loaded and ready for rendering.\n");
	debugPrintf("Use 'font' or 'symbols' commands in poolrad console for visual rendering.\n");

	return true;
}

namespace {

static const byte kEgaPalette[16 * 3] = {
	0x00, 0x00, 0x00,  0x00, 0x00, 0xAA,  0x00, 0xAA, 0x00,  0x00, 0xAA, 0xAA,
	0xAA, 0x00, 0x00,  0xAA, 0x00, 0xAA,  0xAA, 0x55, 0x00,  0xAA, 0xAA, 0xAA,
	0x55, 0x55, 0x55,  0x55, 0x55, 0xFF,  0x55, 0xFF, 0x55,  0x55, 0xFF, 0xFF,
	0xFF, 0x55, 0x55,  0xFF, 0x55, 0xFF,  0xFF, 0xFF, 0x55,  0xFF, 0xFF, 0xFF
};

bool dumpSurfaceBMP(const Common::String &filename,
		const Graphics::ManagedSurface &surf) {
	Common::DumpFile f;
	if (!f.open(Common::Path(filename))) {
		return false;
	}
	bool ok = Image::writeBMP(f, surf.rawSurface(), kEgaPalette, 16);
	f.close();
	return ok;
}

} // anonymous namespace

bool Console::cmdDumpTileCache(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Usage: dumpTileCache <slot 0-4>\n");
		debugPrintf("Dumps all 8x8 tiles in the slot as a grid BMP.\n");
		debugPrintf("Slot ranges: 0=1-45, 1=46-115, 2=116-185, 3=186-255, 4=256-286\n");
		return true;
	}

	int slot = atoi(argv[1]);
	if (slot < 0 || slot >= Gfx::Tile8x8Cache::kSlotCount) {
		debugPrintf("Invalid slot %d (must be 0-4)\n", slot);
		return true;
	}

	const Gfx::Tile8x8Cache &cache = g_engine->getTileCache();
	const Gfx::DaxTile *tiles = cache.getSlot(slot);
	if (!tiles) {
		debugPrintf("Slot %d is not loaded.\n", slot);
		return true;
	}

	uint32 count = tiles->getTileCount();
	if (count == 0) {
		debugPrintf("Slot %d has 0 tiles.\n", slot);
		return true;
	}

	// Layout: 16 tiles per row
	const int tilesPerRow = 16;
	int rows = (count + tilesPerRow - 1) / tilesPerRow;
	int gridW = tilesPerRow * 8;
	int gridH = rows * 8;

	Graphics::ManagedSurface grid(gridW, gridH);
	grid.fillRect(Common::Rect(gridW, gridH), 0);

	uint16 firstId = Gfx::Tile8x8Cache::firstGlobalTileIdForSlot(slot);

	for (uint32 i = 0; i < count; ++i) {
		const Graphics::ManagedSurface *tile = tiles->getTileSurface(i);
		if (!tile)
			continue;
		int col = i % tilesPerRow;
		int row = i / tilesPerRow;
		grid.copyRectToSurface(*tile, col * 8, row * 8,
				Common::Rect(0, 0, 8, 8));
	}

	Common::String filename = Common::String::format("tilecache_slot%d.bmp", slot);
	if (dumpSurfaceBMP(filename, grid)) {
		debugPrintf("Dumped %u tiles (IDs %u-%u) to %s (%dx%d)\n",
				count, firstId, firstId + count - 1,
				filename.c_str(), gridW, gridH);
	} else {
		debugPrintf("Failed to write %s\n", filename.c_str());
	}
	return true;
}

bool Console::cmdDumpWalldef(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Usage: dumpWalldef <wallType 1-15>\n");
		debugPrintf("Dumps all 10 region surfaces for the given wall type.\n");
		debugPrintf("Regions: 0=FAR_FWD, 1=FAR_L, 2=FAR_R, 3=MED_FWD,\n");
		debugPrintf("  4=MED_L, 5=MED_R, 6=CLOSE_FWD, 7=CLOSE_L, 8=CLOSE_R, 9=FAR_FILL\n");
		return true;
	}

	int wallType = atoi(argv[1]);
	if (wallType < 1 || wallType > 15) {
		debugPrintf("Invalid wallType %d (must be 1-15)\n", wallType);
		return true;
	}

	const Gfx::WalldefSlotCache &wsc = g_engine->getWalldefSlotCache();
	const Gfx::WallSurfaceSet *set = wsc.surfaceSetForWallType((uint8)wallType);
	if (!set) {
		debugPrintf("WallType %d not loaded (slot %d, slice %d).\n",
				wallType, (wallType - 1) / 5 + 1, (wallType - 1) % 5);
		return true;
	}

	static const char *kRegionNames[] = {
		"far_fwd", "far_left", "far_right",
		"med_fwd", "med_left", "med_right",
		"close_fwd", "close_left", "close_right",
		"far_filler"
	};

	int dumped = 0;
	for (int r = 0; r < Data::DaxBlockWalldef::VIEW_COUNT; ++r) {
		Data::WalldefRegionId rid = static_cast<Data::WalldefRegionId>(r);
		const Gfx::Pic *pic = set->region(rid);
		if (!pic || pic->w == 0 || pic->h == 0)
			continue;

		Common::String filename = Common::String::format(
				"walldef_wt%d_%d_%s.bmp", wallType, r, kRegionNames[r]);
		if (dumpSurfaceBMP(filename, *pic)) {
			debugPrintf("  [%d] %s: %dx%d -> %s\n", r, kRegionNames[r],
					pic->w, pic->h, filename.c_str());
			++dumped;
		} else {
			debugPrintf("  [%d] %s: FAILED to write\n", r, kRegionNames[r]);
		}
	}
	debugPrintf("Dumped %d region surfaces for wallType %d.\n", dumped, wallType);
	return true;
}

bool Console::cmdDumpWalldefRegion(int argc, const char **argv) {
	if (argc != 3) {
		debugPrintf("Usage: dumpWalldefRegion <wallType 1-15> <regionId 0-9>\n");
		debugPrintf("Dumps a single walldef region surface and prints tile indices.\n");
		return true;
	}

	int wallType = atoi(argv[1]);
	int regionIdx = atoi(argv[2]);
	if (wallType < 1 || wallType > 15) {
		debugPrintf("Invalid wallType %d\n", wallType);
		return true;
	}
	if (regionIdx < 0 || regionIdx >= Data::DaxBlockWalldef::VIEW_COUNT) {
		debugPrintf("Invalid regionId %d (must be 0-9)\n", regionIdx);
		return true;
	}

	const Gfx::WalldefSlotCache &wsc = g_engine->getWalldefSlotCache();
	const Gfx::WallSurfaceSet *set = wsc.surfaceSetForWallType((uint8)wallType);
	if (!set) {
		debugPrintf("WallType %d not loaded.\n", wallType);
		return true;
	}

	Data::WalldefRegionId rid = static_cast<Data::WalldefRegionId>(regionIdx);
	const Gfx::Pic *pic = set->region(rid);
	if (!pic || pic->w == 0 || pic->h == 0) {
		debugPrintf("Region %d is empty for wallType %d.\n", regionIdx, wallType);
		return true;
	}

	const int slotIdx = (wallType - 1) / 5;
	const int sliceIdx = (wallType - 1) % 5;
	debugPrintf("WallType %d -> cache slot %d, slice %d\n",
			wallType, slotIdx + 1, sliceIdx);
	debugPrintf("Region %d: %dx%d pixels (%d cols x %d rows of 8x8 tiles)\n",
			regionIdx, pic->w, pic->h, pic->w / 8, pic->h / 8);

	// Dump the surface
	Common::String filename = Common::String::format(
			"walldef_wt%d_r%d.bmp", wallType, regionIdx);
	if (dumpSurfaceBMP(filename, *pic)) {
		debugPrintf("Saved to %s\n", filename.c_str());
	} else {
		debugPrintf("Failed to write %s\n", filename.c_str());
	}

	// Print the patched tile indices from the walldef chunk data
	// Access the walldef block to read raw indices
	Data::DaxBlock *rawWalldefBlock =
			g_engine->getDaxManager().getWalldef().getBlockById(
					g_engine->getWalldefSlotCache().walldefBlockIdForSlot(slotIdx + 1));
	Data::DaxBlockWalldef *walldef = rawWalldefBlock
			? dynamic_cast<Data::DaxBlockWalldef *>(rawWalldefBlock) : nullptr;

	int cols = pic->w / 8;
	int rows = pic->h / 8;

	if (walldef) {
		int chunkIdx = g_engine->getWalldefSlotCache().chunkIndexForSlot(slotIdx + 1);
		if (chunkIdx >= 0 && chunkIdx < walldef->chunkCount()) {
			Data::DaxBlockWalldef::Slice slice =
					walldef->chunk(chunkIdx).slice(sliceIdx);
			debugPrintf("Patched tile indices (globalTileId after offset):\n");
			for (int row = 0; row < rows; ++row) {
				debugPrintf("  row %2d:", row);
				for (int col = 0; col < cols; ++col) {
					uint8 tid = slice.tileIndex(rid, row, col);
					debugPrintf(" %3d", tid);
				}
				debugPrintf("\n");
			}

			// Also show which tile cache slot each index resolves to
			const Gfx::Tile8x8Cache &tc = g_engine->getTileCache();
			debugPrintf("Tile cache slot resolution:\n");
			for (int row = 0; row < rows; ++row) {
				debugPrintf("  row %2d:", row);
				for (int col = 0; col < cols; ++col) {
					uint8 tid = slice.tileIndex(rid, row, col);
					if (tid == 0) {
						debugPrintf("  --");
					} else {
						int tcSlot = tc.slotForGlobalTileId(tid);
						int localIdx = tc.localTileIndex(tid);
						const Graphics::ManagedSurface *t = tc.tileSurface(tid);
						debugPrintf(" %d:%02d%s", tcSlot, localIdx,
								t ? "" : "!");
					}
				}
				debugPrintf("\n");
			}
		}
	} else {
		debugPrintf("(Cannot read raw walldef data - block not available)\n");
		// Fallback: just print pixel samples
		debugPrintf("Cell top-left pixel values:\n");
		for (int row = 0; row < rows; ++row) {
			debugPrintf("  row %2d:", row);
			for (int col = 0; col < cols; ++col) {
				uint8 px = pic->getPixel(col * 8, row * 8);
				debugPrintf(" %2d", px);
			}
			debugPrintf("\n");
		}
	}

	return true;
}

bool Console::cmdDumpWalldefAll(int argc, const char **argv) {
	(void)argc;
	(void)argv;

	debugPrintf("Dumping all loaded walldef surfaces + tile cache slots...\n");

	const Gfx::Tile8x8Cache &tc = g_engine->getTileCache();
	const Gfx::WalldefSlotCache &wsc = g_engine->getWalldefSlotCache();

	// Dump tile cache slots 0-4
	for (int slot = 0; slot < Gfx::Tile8x8Cache::kSlotCount; ++slot) {
		const Gfx::DaxTile *tiles = tc.getSlot(slot);
		if (!tiles || tiles->getTileCount() == 0) {
			debugPrintf("  TileCache slot %d: not loaded\n", slot);
			continue;
		}

		uint32 count = tiles->getTileCount();
		const int tilesPerRow = 16;
		int rows = (count + tilesPerRow - 1) / tilesPerRow;
		int gridW = tilesPerRow * 8;
		int gridH = rows * 8;

		Graphics::ManagedSurface grid(gridW, gridH);
		grid.fillRect(Common::Rect(gridW, gridH), 0);

		for (uint32 i = 0; i < count; ++i) {
			const Graphics::ManagedSurface *tile = tiles->getTileSurface(i);
			if (!tile)
				continue;
			grid.copyRectToSurface(*tile, (i % tilesPerRow) * 8,
					(i / tilesPerRow) * 8, Common::Rect(0, 0, 8, 8));
		}

		Common::String filename = Common::String::format(
				"walldump_tilecache_slot%d.bmp", slot);
		dumpSurfaceBMP(filename, grid);
		debugPrintf("  TileCache slot %d: %u tiles -> %s\n",
				slot, count, filename.c_str());
	}

	// Dump all walldef regions for wall types 1-15
	static const char *kRegionNames[] = {
		"far_fwd", "far_left", "far_right",
		"med_fwd", "med_left", "med_right",
		"close_fwd", "close_left", "close_right",
		"far_filler"
	};

	int totalDumped = 0;
	for (int wt = 1; wt <= 15; ++wt) {
		const Gfx::WallSurfaceSet *set = wsc.surfaceSetForWallType((uint8)wt);
		if (!set) {
			debugPrintf("  WallType %2d: not loaded\n", wt);
			continue;
		}

		for (int r = 0; r < Data::DaxBlockWalldef::VIEW_COUNT; ++r) {
			Data::WalldefRegionId rid = static_cast<Data::WalldefRegionId>(r);
			const Gfx::Pic *pic = set->region(rid);
			if (!pic || pic->w == 0 || pic->h == 0)
				continue;

			Common::String filename = Common::String::format(
					"walldump_wt%02d_r%d_%s.bmp", wt, r, kRegionNames[r]);
			dumpSurfaceBMP(filename, *pic);
			++totalDumped;
		}
	}

	debugPrintf("Done. Dumped %d walldef region surfaces.\n", totalDumped);
	return true;
}

} // End of namespace Goldbox
