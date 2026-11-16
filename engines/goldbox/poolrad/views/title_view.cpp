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
#include "goldbox/gfx/backend/gfx_backend.h"
#include "goldbox/poolrad/views/title_view.h"
#include "goldbox/poolrad/views/credits_view.h"
#include "goldbox/poolrad/poolrad.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

bool TitleView::msgFocus(const FocusMessage &msg) {
	// Some ports keep their title screens outside the game data; without
	// both pictures the title goes on to the credits.
	Goldbox::Gfx::GfxBackend &gfx = g_engine->getGfx();
	Goldbox::Data::DaxFileManager &dax = VmInterface::getDaxManager();
	_pic1 = gfx.loadTitlePicture(dax, 1);
	_pic2 = _pic1 ? gfx.loadTitlePicture(dax, 2) : nullptr;
	if (!_pic2) {
		delete _pic1;
		_pic1 = nullptr;
	}
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
			s.writeStringC(0, 24, 10, VmInterface::getString("prompts.loading"));
			delaySeconds(1);
			break;
		case 1:
			_pic1->draw(&s, 0, 0);
			delaySeconds(5);
			break;
		case 2:
			_pic2->draw(&s, 0, 0);
			delaySeconds(5);
			break;
		case 3:
			delete _pic1;
			delete _pic2;
			g_engine->stopV10Music();
			replaceView("Credits");
			break;
		case 4:
			delete _pic1;
			delete _pic2;
			g_engine->stopV10Music();
			// A key skips the title and credits, not the code wheel.
			replaceView(viewAfterCredits());
			break;
		default:
			debug(6, "Unknown state in TitleView");

	}
}

void TitleView::timeout() {
	// Without title pictures go on to the credits. The Amiga has its credits
	// in the intro, which follows the code wheel.
	if (_state == 0 && !_pic1) {
		if (g_engine->getPlatform() == Common::kPlatformAmiga) {
			replaceView(viewAfterCredits());
			return;
		}
		_state = 3;
	}
	else if (++_state == 1) {
		// v1.0 VIEW_Title: stop, then the title tune on Tandy, before the
		// first picture.
		g_engine->stopV10Music();
		g_engine->playV10TandyMusic(kSfxTitleMusicV10);
	}
	redraw();
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
