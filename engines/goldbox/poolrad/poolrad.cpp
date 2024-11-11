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
#include "goldbox/core/file.h"
#include "goldbox/gfx/surface.h"
#include "goldbox/gfx/dax_font.h"
#include "goldbox/gfx/dax_tile.h"
#include "goldbox/data/daxblock.h"
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
	Surface::setupPalette();

	Goldbox::File daxFile8x8d;
	if (!daxFile8x8d.open("8x8d1.dax")) {
		error("Failed to open 8x8d1.dax");
	}

	Goldbox::Data::DaxBlock *pc_font = daxFile8x8d.getBlockById(201);
	auto daxFont = new Goldbox::Gfx::DaxFont(dynamic_cast<Goldbox::Data::DaxBlock8x8D*>(pc_font));
	_font = daxFont;

	Goldbox::Data::DaxBlock *symbols = daxFile8x8d.getBlockById(202);
	auto daxScreenTiles = new Goldbox::Gfx::DaxTile(dynamic_cast<Goldbox::Data::DaxBlock8x8D*>(symbols));
	_symbols = daxScreenTiles;

	daxFile8x8d.close();

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
