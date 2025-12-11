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

#include "common/system.h"
#include "graphics/paletteman.h"
#include "goldbox/vm_interface.h"
#include "goldbox/poolrad/views/title_view.h"
#include "goldbox/poolrad/poolrad.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

bool TitleView::msgFocus(const FocusMessage &msg) {
	Goldbox::Data::DaxBlockContainer &titleContainer = VmInterface::getDaxManager().getTitle();

	Goldbox::Data::DaxBlock *daxBlock = titleContainer.getBlockById(1);
	if (!daxBlock) {
		error("Failed to load title block 1 from title container");
	}
	_pic1 = Goldbox::Gfx::Pic::read(dynamic_cast<Goldbox::Data::DaxBlockPic*>(daxBlock));

	daxBlock = titleContainer.getBlockById(2);
	if (!daxBlock) {
		error("Failed to load title block 2 from title container");
	}
	_pic2 = Goldbox::Gfx::Pic::read(dynamic_cast<Goldbox::Data::DaxBlockPic*>(daxBlock));

	return true;
}

bool TitleView::msgKeypress(const KeypressMessage &msg) {
	_state = 4;
	redraw();
	return true;
}

void TitleView::draw() {
	Surface s = getSurface();
	switch(_state)
	{
		case 0:
			s.clear();
			s.setToText();
			s.writeStringC("Loading...Please Wait", 10, 0, 24);
			delaySeconds(1);
			break;
		case 1:
			s.blitFrom(*_pic1);
			delaySeconds(5);
			break;
		case 2:
			s.blitFrom(*_pic2);
			delaySeconds(5);
			break;
		case 3:
			delete _pic1;
			delete _pic2;
			replaceView("Credits");
			break;
		case 4:
			delete _pic1;
			delete _pic2;
			replaceView("Mainmenu"); //TODO Option to skip Copy Protection
			break;

	}
}

void TitleView::timeout() {
	_state++;
	redraw();
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
