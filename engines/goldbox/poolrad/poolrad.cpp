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

#include "common/config-manager.h"
#include "common/engine_data.h"
#include "common/fs.h"
#include "goldbox/core/file.h"
#include "goldbox/gfx/surface.h"
#include "goldbox/gfx/dax_font.h"
#include "goldbox/gfx/dax_tile.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/data/daxfilemanager.h"
#include "goldbox/data/strings_data.h"
#include "goldbox/poolrad/poolrad.h"
//#include "goldbox/poolrad/gfx/cursors.h"

#include "goldbox/poolrad/console.h"

namespace Goldbox {
namespace Poolrad {

PoolradEngine *g_engine;

//Data::Character PoolradEngine::_party[MAX_CHARACTERS];

PoolradEngine::PoolradEngine(OSystem *syst, const GoldboxGameDescription *gameDesc) :
		Goldbox::Engine(syst, gameDesc) {
	g_engine = this;
}

PoolradEngine::~PoolradEngine() {
	g_engine = nullptr;
	delete _views;
}

void PoolradEngine::initializePath(const Common::FSNode &gamePath) {
	Engine::initializePath(gamePath);
	//SearchMan.addDirectory("data", gamePath.getChild("rom").getChild("data"), 0, 3);
}

void PoolradEngine::setup() {
// Initialise engine data for the game
//	Common::U32String errMsg;
//	if (!Common::load_engine_data("poolrad.dat", "poolrad", 1, 0, errMsg)) {
//		Common::String msg(errMsg);
//		error("%s", msg.c_str());
//	}
	Common::File items;
	if (!items.open("ITEMS")) {
		warning("Cannot open ITEMS file");
		return;
	}
	Common::SeekableReadStream &in = items;
	Engine::gItemProps.load(in);
	Engine::gItemProps.debugStorage(); //TODO: Remove this line later
	items.close();

	Surface::setupPalette();

	// Load all 8x8d files into container
	Common::Array<Common::Path> dax8x8dFiles;
	dax8x8dFiles.push_back(Common::Path("8x8d1.dax"));
	dax8x8dFiles.push_back(Common::Path("8x8d2.dax"));
	dax8x8dFiles.push_back(Common::Path("8x8d3.dax"));
	dax8x8dFiles.push_back(Common::Path("8x8d4.dax"));
	dax8x8dFiles.push_back(Common::Path("8x8d5.dax"));
	dax8x8dFiles.push_back(Common::Path("8x8d6.dax"));
	dax8x8dFiles.push_back(Common::Path("8x8d7.dax"));
	dax8x8dFiles.push_back(Common::Path("8x8d8.dax"));

	Goldbox::Data::DaxBlockContainer &container8x8d = _daxManager.get8x8d();
	container8x8d.loadFromFiles(dax8x8dFiles);

	// Populate daxFont from container
	Goldbox::Data::DaxBlock *pc_font = container8x8d.getBlockById(201);
	if (!pc_font) {
		error("Failed to load font block 201 from 8x8d container");
	}
	auto daxFont = new Goldbox::Gfx::DaxFont(dynamic_cast<Goldbox::Data::DaxBlock8x8D*>(pc_font));
	_font = daxFont;

	// Populate daxScreenTiles from container
	Goldbox::Data::DaxBlock *symbols = container8x8d.getBlockById(202);
	if (!symbols) {
		error("Failed to load symbols block 202 from 8x8d container");
	}
	auto daxScreenTiles = new Goldbox::Gfx::DaxTile(dynamic_cast<Goldbox::Data::DaxBlock8x8D*>(symbols));
	_symbols = daxScreenTiles;

	Goldbox::File walldefFile;
	if (!walldefFile.open("walldef3.dax")) {
		error("Failed to open WALLDEF3.DAX");
	}

	Goldbox::Data::DaxBlock *block = walldefFile.getBlockById(0);
	auto walldef = dynamic_cast<Goldbox::Data::DaxBlockWalldef *>(block);

	// Example: Access the first chunk, first slice, closeForward region
	if (walldef->chunkCount() > 0) {
		const auto &chunk = walldef->chunk(0);
		auto slice = chunk.slice(0);
		auto closeForward = slice.region(Goldbox::Data::WalldefRegionId::CLOSE_FORWARD);
		// Now you can iterate over closeForward to get the tile indices
		//for (uint8 tile : closeForward) {
			// Use tile...
		//}
	}

	walldefFile.close();


	if (!_strings.load("global_strings.yml")){
		error("Failed to open global_strings.yml");
	}



/*
	// Load save data
	int saveSlot = ConfMan.getInt("save_slot");
	_saved.load();
	if (saveSlot != -1)
		(void)loadGameState(saveSlot);
*/
	//_pics.load("allpics1");

	// Setup game views
	_views = new Views::Views();
	addView("Title");
}

GUI::Debugger *PoolradEngine::getConsole() {
	return new Console();
}

} // namespace Poolrad
} // namespace Goldbox
