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

#ifndef GOLDBOX_POOLRAD_POOLRAD_H
#define GOLDBOX_POOLRAD_POOLRAD_H

#include "goldbox/engine.h"
#include "goldbox/poolrad/views/views.h"
#include "goldbox/poolrad/data/character.h"
//#include "goldbox/poolrad/files/game_archive.h"
//#include "goldbox/poolrad/data/saved.h"
//#include "goldbox/poolrad/gfx/pics.h"

namespace Goldbox {
namespace Poolrad {

const int MAX_CHARACTERS = 8;
const int MAX_PC_IN_PARTY = 6;
const int MAX_NPC_IN_PARTY = 2;

class PoolradEngine : public Goldbox::Engine {
private:
	Poolrad::Views::Views *_views = nullptr;
	uint16 _mapX = 0, _mapY = 0;

protected:
	void setup() override;
	GUI::Debugger *getConsole() override;

public:

	//GameArchive *_gameArchive = nullptr;
	//Data::Saved _saved;
	//Gfx::PicsDecoder _pics;

public:
	static Data::Character _party[MAX_CHARACTERS];

	PoolradEngine(OSystem *syst, const GoldboxGameDescription *gameDesc);
	~PoolradEngine() override;
	void initializePath(const Common::FSNode &gamePath) override;
};

extern PoolradEngine *g_engine;

} // namespace Poolrad
} // namespace Goldbox

#endif
