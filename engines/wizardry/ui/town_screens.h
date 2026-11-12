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

#ifndef WIZARDRY_UI_TOWN_SCREENS_H
#define WIZARDRY_UI_TOWN_SCREENS_H

#include "wizardry/game/party.h"
#include "wizardry/gfx/cell_screen.h"
#include "wizardry/resources/messages.h"
#include "wizardry/ui/menu.h"

namespace Wizardry {

/**
 * Drawing of the castle and its places for Wizardry I, with the window
 * positions and message numbers of the original. These functions only
 * draw; they know nothing of the engine, keys or views.
 */
class TownScreens {
public:
	enum {
		kNobody = 0xffffffff
	};

private:
	CellScreen &_screen;
	const MessageFile &_messages;
	const Party &_party;

	TextWindow openWindow(int x, int y, int width, int height);
	void printPartyLine(TextWindow &window, uint index);

public:
	TownScreens(CellScreen &screen, const MessageFile &messages, const Party &party) :
		_screen(screen), _messages(messages), _party(party) {}

	/**
	 * What every place in town shows: the main window, the "CASTLE"
	 * heading and the party
	 */
	void drawBackground();
	void drawParty();

	/**
	 * A small window in the middle with one line, for things said in passing
	 */
	void drawNotice(const Common::String &text);

	void drawCastleMenu(Menu &menu);

	void drawTavernMenu(Menu &menu);
	/**
	 * @param available  Roster numbers behind the letters A, B, ...;
	 *                   kNobody leaves a gap
	 */
	void drawTavernAdd(const Common::Array<uint> &available);
	void drawTavernRemove();
};

} // End of namespace Wizardry

#endif
