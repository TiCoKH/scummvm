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
#include "goldbox/gfx/dax_tile.h"

namespace Goldbox {

Console::Console() : GUI::Debugger() {
	registerCmd("view",   WRAP_METHOD(Console, cmdView));
	registerCmd("showTile", WRAP_METHOD(Console, cmdShowTile));
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

} // End of namespace Goldbox
