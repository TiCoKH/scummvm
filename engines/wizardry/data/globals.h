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
#include "wizardry/game/party.h"
#include "wizardry/resources/scenario_file.h"
#include "wizardry/gfx/cell_renderer.h"
#include "wizardry/gfx/cell_screen.h"
#include "wizardry/gfx/wizardry_font.h"
#include "wizardry/resources/messages.h"
#include "wizardry/resources/picture_file.h"

namespace Wizardry {

struct Globals {
	WizardryFont *_normalFont = nullptr;
	WizardryFont *_gfxFont = nullptr;

	// Releases that draw through the cell screen (all but the Apple II one)
	CellScreen _cells;
	CellRenderer _renderer;
	PictureFile _pictures;
	MessageFile _messages;
	ScenarioFile _scenario;
	Party _party;

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
	 * Copies the cells that changed to the screen
	 */
	void present();
};

extern Globals *g_globals;
#define _G(FIELD) (g_globals->_##FIELD)

} // namespace Wizardry

#endif
