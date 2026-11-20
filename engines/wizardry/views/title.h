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

#ifndef WIZARDRY_VIEWS_TITLE_H
#define WIZARDRY_VIEWS_TITLE_H

#include "wizardry/resources/tile_image.h"
#include "wizardry/views/tile_view.h"

namespace Wizardry {

/**
 * The title screen.
 *
 * For IBM-PC Wizardry I to III it is the original's (KANJIREA procedures
 * 13 to 15): the logo, a welcome, and below it one to four creatures at
 * random. They stay about six and a half seconds, then slide down out of
 * sight and others come. A key cuts the wait short; after the slide,
 * S)TART GAME goes on and any other key brings new creatures. M, which
 * made a scenario disk in the original, opens the gallery instead, as a
 * hidden feature for debugging; Escape there comes back to the title.
 *
 * The other releases build their title in code that has not been read
 * yet; for them only the logo is shown, and any key goes on.
 */
class Title : public TileView {
private:
	enum State {
		kStatic,     ///< only the logo
		kShowing,    ///< creatures on show
		kSliding     ///< creatures sliding away, a row each frame
	};

	State _state = kStatic;
	TileImage _logo;
	Common::Array<TileImage> _pictures;
	int _slide = 0;
	uint16 _key = 0;      ///< key pressed while the creatures were on show

	bool isOriginal() const;
	/** The key of a choice of the title: the first letter of its line */
	uint16 titleKey(uint id) const;
	void newPictures();
	void startSlide();
	void keyAfterSlide(uint16 key);

protected:
	void drawTiles(TextWindow &window) override;
	void timeout() override;

public:
	Title() : TileView("Title", false) {}

	/**
	 * Goes on to the game, or to the gallery for releases whose game
	 * screens do not exist yet
	 */
	void leave();

	bool msgFocus(const FocusMessage &msg) override;
	bool msgKeypress(const KeypressMessage &msg) override;
	bool msgMouseDown(const MouseDownMessage &msg) override;
};

} // End of namespace Wizardry

#endif
