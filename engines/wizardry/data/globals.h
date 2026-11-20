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

#ifndef WIZARDRY_DATA_GLOBALS_H
#define WIZARDRY_DATA_GLOBALS_H

#include "common/file.h"
#include "wizardry/data/scenario.h"
#include "wizardry/game/items.h"
#include "wizardry/game/maze.h"
#include "wizardry/game/monsters.h"
#include "wizardry/game/treasure.h"
#include "wizardry/game/party.h"
#include "wizardry/resources/scenario_file.h"
#include "wizardry/gfx/tile_renderer.h"
#include "wizardry/gfx/tile_screen.h"
#include "wizardry/gfx/wizardry_font.h"
#include "wizardry/resources/messages.h"
#include "wizardry/resources/picture_file.h"

namespace Wizardry {

struct Globals {
	WizardryFont *_normalFont = nullptr;
	WizardryFont *_gfxFont = nullptr;

	// The following members are used for releases that draw through the tile screen (all but the Apple II one):
	TileScreen _tiles;
	TileRenderer _renderer;
	PictureFile _pictures;
	MessageFile _messages;
	Common::String _titleLines[5];             ///< the title's lines from the ASCII text
	bool _kanaText = false;   ///< kana or kanji: the title shows its kana lines
	WideFont *_wideFont = nullptr;
	ScenarioFile _scenario;
	Party _party;
	ItemList _items;
	MonsterList _monsters;
	RewardList _rewards;
	RoomFights _roomFights;
	MazeLevel _mazeLevel;                     ///< the level the party is on
	Common::Array<MazeLevel> _mazeLevels;     ///< all levels, as they are after what happened on them
	MazeState _mazeState;
	int _messageDelay = 20;   ///< how long a line of a fight stays, in the original's units of 20

	TSCNTOC _SCNTOC;
	int _LLBASE04 = -1;
	int _SCNTOCBL = 0;
	int _TIMEDLAY = 2000;
	bool _CACHEWRI = false;
	int _CACHEBL = 0;
	int _MAZEX = 0;
	int _MAZEY = 0;
	int _MAZELEV = 0;
	int _PARTYCNT = 0;
	int _DIRECTIO = 0;
	int _ACMOD2 = 0;

	Globals();
	~Globals();

	void setup();

	/**
	 * Sets the colours of the screen, with the CGA palette chosen in
	 * the game options for the IBM-PC releases
	 */
	void applyPalette();

	/** The text in the language the game options ask for */
	void loadMessages();

	/**
	 * Copies the tiles that changed to the screen
	 */
	void present();
};

extern Globals *g_globals;
#define _G(FIELD) (g_globals->_##FIELD)

} // namespace Wizardry

#endif
