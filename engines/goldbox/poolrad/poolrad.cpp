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
#include "goldbox/gfx/surface.h"
#include "goldbox/gfx/dax_font.h"
#include "goldbox/gfx/dax_tile.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/poolrad/poolrad.h"
//#include "goldbox/poolrad/gfx/cursors.h"

#include "goldbox/poolrad/console.h"

namespace Goldbox {
namespace Poolrad {

PoolradEngine *g_engine;

PoolradEngine::PoolradEngine(OSystem *syst, const GoldboxGameDescription *gameDesc) :
		Goldbox::Engine(syst, gameDesc) {
	g_engine = this;
	_dcache = g_engine->getDaxCache();
}

PoolradEngine::~PoolradEngine() {
	g_engine = nullptr;
	delete _views;
	delete _dcache;
}

void PoolradEngine::initializePath(const Common::FSNode &gamePath) {
	Engine::initializePath(gamePath);
	SearchMan.addDirectory("data", gamePath.getChild("rom").getChild("data"), 0, 3);
}

void PoolradEngine::setup() {
	// Initialise engine data for the game
//	Common::U32String errMsg;
//	if (!Common::load_engine_data("poolrad.dat", "poolrad", 1, 0, errMsg)) {
//		Common::String msg(errMsg);
//		error("%s", msg.c_str());
//	}

	Surface::setupPalette();

	_dcache->loadFile("8x8d1.dax");
	Goldbox::DaxBlock *pc_font = _dcache->getBlock(ContentType::_8X8D, 201);
	auto daxFont = new Goldbox::Gfx::DaxFont(dynamic_cast<DaxBlock8x8D*>(pc_font));
	_fonts.push_back(daxFont);
	Goldbox::DaxBlock *symbols = _dcache->getBlock(ContentType::_8X8D, 202);
	auto daxScreenTiles = new Goldbox::Gfx::DaxTile(dynamic_cast<DaxBlock8x8D*>(symbols));
	_fonts.push_back(daxScreenTiles);

	_dcache->loadFile("title.dax");

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
	addView("Roster");
}

GUI::Debugger *PoolradEngine::getConsole() {
	return new Console();
}

} // namespace Wasteland1
} // namespace Wasteland
