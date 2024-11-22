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

#ifndef GOLDBOX_POOLRAD_VIEWS_ADD_CHARACTER_VIEW_H
#define GOLDBOX_POOLRAD_VIEWS_ADD_CHARACTER_VIEW_H

#include "graphics/managed_surface.h"
#include "goldbox/poolrad/views/view.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

class AddCharacterView : public View {
private:
	Common::Array<Common::String> _rosterList;
	Dialogs::VerticalMenu *_rosterMenu;

public:
	AddCharacterView();
	virtual ~AddCharacterView();

	bool msgKeypress(const KeypressMessage &msg) override;
	bool msgFocus(const FocusMessage &msg) override;
	bool msgUnfocus(const UnfocusMessage &msg) override;
	void draw() override;
	void timeout() override;
	void loadRosterList();
    void processRoster();
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
