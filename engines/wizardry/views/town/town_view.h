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

#ifndef WIZARDRY_VIEWS_TOWN_TOWN_VIEW_H
#define WIZARDRY_VIEWS_TOWN_TOWN_VIEW_H

#include "wizardry/ui/town_screens.h"
#include "wizardry/views/cell_view.h"

namespace Wizardry {
namespace Town {

/**
 * Base class for the places in town. It turns keys and mouse clicks into
 * menu choices and shows short notices; what is on screen is drawn by
 * TownScreens.
 */
class TownView : public CellView {
private:
	Common::String _notice;

protected:
	Menu _menu;

	/**
	 * Draws what lies behind the place: the castle heading and the party
	 */
	virtual void drawBackground(TownScreens &screens);

	/**
	 * Draws the place over the background
	 */
	virtual void drawTown(TownScreens &screens) = 0;

	/**
	 * Codes of keys that are not characters, as the original gets them
	 * from its keyboard driver
	 */
	enum {
		kKeyBackspace = 8,
		kKeyLeft = 9,
		kKeyRight = 12,
		kKeyDown = 10,
		kKeyUp = 11,
		kKeyReturn = 13,
		kKeyEscape = 27
	};

	/**
	 * Called with the result of the menu, or with -1 for a key that
	 * chooses nothing. The character is in upper case; RETURN is 13.
	 */
	virtual void choose(int result, uint16 ascii) = 0;

	/**
	 * Shows a line in the middle of the screen for a moment. Keys are
	 * ignored meanwhile.
	 */
	void notice(const Common::String &text);
	virtual void noticeDone() {}

	enum {
		/** The pause of the original after it has said something: 40 timer ticks of the PC */
		kPauseFrames = FRAME_RATE * 40 / 18
	};

	void drawCells(TextWindow &window) override;
	void timeout() override;

public:
	TownView(const Common::String &name) : CellView(name, false) {}

	bool msgKeypress(const KeypressMessage &msg) override;

	/**
	 * Turns a key into the character the original would get: letters in
	 * upper case, and codes for RETURN, the cursor keys and so on.
	 * @returns 0 for a key that means nothing
	 */
	static uint16 keyToAscii(const KeypressMessage &msg);
	bool msgMouseDown(const MouseDownMessage &msg) override;
};

} // End of namespace Town
} // End of namespace Wizardry

#endif
