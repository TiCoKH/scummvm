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

#include "ultima/ultima1/u1gfx/fmt/view_demo.h"
#include "ultima/ultima1/u1gfx/drawing_support.h"
#include "ultima/ultima1/core/resources.h"
#include "ultima/ultima1/game.h"
#include "ultima/shared/core/file.h"
#include "ultima/shared/gfx/text_cursor.h"
#include "ultima/shared/early/font_resources.h"
#include "ultima/shared/early/ultima_early.h"
#include "ultima/shared/gfx/tiff.h"

namespace Ultima {
namespace Ultima1 {
namespace U1Gfx {

BEGIN_MESSAGE_MAP(ViewDemo, Shared::Gfx::VisualItem)
	ON_MESSAGE(ShowMsg)
	ON_MESSAGE(KeypressMsg)
	ON_MESSAGE(FrameMsg)
END_MESSAGE_MAP()

ViewDemo::ViewDemo(Shared::TreeItem *parent) : Shared::Gfx::VisualItem("Intro", Rect(0, 0, 640, 480), parent) {
	setMode(DEMOMODE_HEADER);

	Shared::File f;
	// Load the headttl.tif
	f.open("graph/headttl.tif");
	Image::TIFFDecoder headttl;
	if (!headttl.loadStream(f))
		error("Couldn't load headttl.tif");
	f.close();
	const Graphics::Surface *src = headttl.getSurface();
	_header.create(src->w, src->h);
	_header.blitFrom(*src);

	f.open("graph/ut1_1.tif");
	Image::TIFFDecoder ut11;
	if (!ut11.loadStream(f))
		error("Couldn't load ut1_1.tif");
	f.close();
	src = ut11.getSurface();
	_slides[0].create(src->w, src->h);
	_slides[0].blitFrom(*src);

	f.open("graph/ut1_3.tif");
	Image::TIFFDecoder ut13;
	if (!ut13.loadStream(f))
		error("Couldn't load ut1_3.tif");
	f.close();
	src = ut13.getSurface();
	_slides[1].create(src->w, src->h);
	_slides[1].blitFrom(*src);

	f.open("graph/ut1_2.tif");
	Image::TIFFDecoder ut12;
	if (!ut12.loadStream(f))
		error("Couldn't load ut1_2.tif");
	f.close();
	src = ut12.getSurface();
	_slides[2].create(src->w, src->h);
	_slides[2].blitFrom(*src);

	f.open("graph/ut1_4.tif");
	Image::TIFFDecoder ut14;
	if (!ut14.loadStream(f))
		error("Couldn't load ut1_4.tif");
	f.close();
	src = ut14.getSurface();
	_slides[3].create(src->w, src->h);
	_slides[3].blitFrom(*src);

}

void ViewDemo::draw() {
	VisualItem::draw();

	switch (_mode) {
	case DEMOMODE_HEADER:
		drawDemoView();
		break;

	case DEMOMODE_PRESENTS:
		drawPresentsView();
		break;

	case DEMOMODE_CASTLE:
		drawCastleView();
		break;

	case DEMOMODE_TRADEMARKS:
		drawTrademarksView();
		break;

	case DEMOMODE_MAIN_MENU:
		drawMainMenu();
		break;

	default:
		break;
	}
}

void ViewDemo::drawDemoView() {
	Ultima1Game *game = static_cast<Ultima1Game *>(getGame());
	Shared::Gfx::VisualSurface s = getSurface();
	s.clear();

	s.drawBorderS(Point(0, 24), 640, 400, 15);

	// Write text
	s.writeString(game->_res->TITLE_MESSAGES[0], TextPoint(16, 8), game->_whiteColor);
	s.writeString(game->_res->TITLE_MESSAGES[1], TextPoint(8, 11), game->_whiteColor);
	s.writeString(game->_res->TITLE_MESSAGES[2], TextPoint(0, 21), game->_whiteColor);
}

void ViewDemo::drawPresentsView() {
	Ultima1Game *game = static_cast<Ultima1Game *>(getGame());
	Shared::Gfx::VisualSurface s = getSurface();

	switch (_counter) {
	case 0:
		s.clear();
		s.blitFrom(_logo, Point(20, 21));
		s.writeString(game->_res->TITLE_MESSAGES[3], TextPoint(14, 13));
		break;
	case 1:
		s.writeString(game->_res->TITLE_MESSAGES[4], TextPoint(5, 12));
		s.writeString(game->_res->TITLE_MESSAGES[5], TextPoint(5, 13));
		s.writeString(game->_res->TITLE_MESSAGES[6], TextPoint(5, 14));
		break;
	case 2:
		s.fillRect(Rect(0, 12 * 8, 320, 15 * 8), game->_bgColor);
		s.writeString(game->_res->TITLE_MESSAGES[7], TextPoint(6, 12));
		s.writeString(game->_res->TITLE_MESSAGES[8], TextPoint(6, 13));
		break;
	default:
		break;
	}
}

void ViewDemo::drawCastleView() {
	Shared::Gfx::VisualSurface s = getSurface();
	if (_counter == 0)
		s.blitFrom(_castle);

	drawCastleFlag(s, 123);
	drawCastleFlag(s, 196);
}

void ViewDemo::drawCastleFlag(Shared::Gfx::VisualSurface &s, int xp) {
	s.blitFrom(_flags[getGame()->getRandomNumber(0, 2)], Common::Point(xp, 55));
}

void ViewDemo::drawTrademarksView() {
	Ultima1Game *game = static_cast<Ultima1Game *>(getGame());
	Shared::Gfx::VisualSurface s = getSurface();
	if (_counter == 0)
		s.clear();

	if (_counter < 32) {
		s.blitFrom(_logo, Common::Rect(0, 0, _logo.w, _counter + 1), Point(20, 21));
		s.blitFrom(_logo, Common::Rect(0, _logo.h - _counter - 1, _logo.w, _logo.h),
			Common::Point(20, 21 + _logo.h - _counter - 1));
	} else {
		s.writeString(game->_res->TITLE_MESSAGES[9], TextPoint(1, 17));
		s.writeString(game->_res->TITLE_MESSAGES[10], TextPoint(2, 18));
		s.writeString(game->_res->TITLE_MESSAGES[11], TextPoint(11, 19));
		s.writeString(game->_res->TITLE_MESSAGES[12], TextPoint(6, 23));
	}
}

void ViewDemo::drawMainMenu() {
	Ultima1Game *game = static_cast<Ultima1Game *>(getGame());
	Shared::Gfx::VisualSurface s = getSurface();
	DrawingSupport ds(s);
	s.clear();
	ds.drawFrame();

	s.writeString(game->_res->MAIN_MENU_TEXT[0], TextPoint(12, 6));
	s.writeString(game->_res->MAIN_MENU_TEXT[1], TextPoint(14, 9));
	s.writeString(game->_res->MAIN_MENU_TEXT[2], TextPoint(14, 10));
	s.writeString(game->_res->MAIN_MENU_TEXT[3], TextPoint(13, 11));
	s.writeString(game->_res->MAIN_MENU_TEXT[4], TextPoint(8, 14));
	s.writeString(game->_res->MAIN_MENU_TEXT[5], TextPoint(8, 15));
	s.writeString(game->_res->MAIN_MENU_TEXT[6], TextPoint(13, 18));
}

void ViewDemo::setTitlePalette() {
	const byte PALETTE[] = { 0, 1, 2, 3, 4, 5, 6, 7, 56, 57, 58, 59, 60, 61, 62, 63 };
	getGame()->setEGAPalette(PALETTE);
}

void ViewDemo::setCastlePalette() {
	const byte PALETTE[] = { 0, 24, 7, 63, 63, 34, 58, 14, 20, 7, 61, 59, 1, 57, 7, 63 };
	getGame()->setEGAPalette(PALETTE);
}

bool ViewDemo::FrameMsg(CFrameMsg *msg) {
	uint32 time = getGame()->getMillis();
	if (time < _expiryTime)
		return true;
	setDirty();

	switch (_mode) {
	case DEMOMODE_COPYRIGHT:
		setMode(DEMOMODE_PRESENTS);
		break;
	case DEMOMODE_PRESENTS:
		_expiryTime = time + 3000;
		if (++_counter == 3)
			setMode(DEMOMODE_CASTLE);
		break;

	case DEMOMODE_CASTLE:
		_expiryTime = time + 200;
		if (++_counter == 100)
			setMode(DEMOMODE_PRESENTS);
		break;

	case DEMOMODE_TRADEMARKS:
		_expiryTime = time + 20;
		++_counter;
		if (_counter == 32) {
			_expiryTime = time + 4000;
		} else if (_counter == 33) {
			setMode(DEMOMODE_MAIN_MENU);
		}
		break;

	default:
		break;
	}

	return true;
}

void ViewDemo::setMode(DemoMode mode) {
	_expiryTime = getGame()->getMillis();
	_counter = 0;
	_mode = mode;
	setDirty();
	setTitlePalette();

	switch (mode) {
	case DEMOMODE_COPYRIGHT:
		_expiryTime += 4000;
		break;
	case DEMOMODE_PRESENTS:
		_expiryTime += 3000;
		break;
	case DEMOMODE_CASTLE:
		setCastlePalette();
		break;
	case DEMOMODE_MAIN_MENU: {
		Shared::Gfx::TextCursor *textCursor = getGame()->_textCursor;
		textCursor->setPosition(TextPoint(25, 18));
		textCursor->setVisible(true);
		break;
	}
	default:
		break;
	}
}

bool ViewDemo::ShowMsg(CShowMsg *msg) {
	Shared::Gfx::VisualItem::ShowMsg(msg);

	if (_mode == DEMOMODE_MAIN_MENU) {
		// Returning to main menu from another screen
		setMode(DEMOMODE_MAIN_MENU);
	}

	return true;
}

bool ViewDemo::KeypressMsg(CKeypressMsg *msg) {
	uint32 time = getGame()->getMillis();

	if (_mode == DEMOMODE_MAIN_MENU) {
		if (msg->_keyState.keycode == Common::KEYCODE_a || msg->_keyState.keycode == Common::KEYCODE_b) {
			// Hide the cursor
			Shared::Gfx::TextCursor *textCursor = getGame()->_textCursor;
			textCursor->setVisible(false);

			if (msg->_keyState.keycode == Common::KEYCODE_a) {
				setView("CharGen");
			} else {
				if (!g_vm->loadGameDialog())
					textCursor->setVisible(true);
			}
		}

	} else if (_mode != DEMOMODE_TRADEMARKS) {
		// Switch to the trademarks view
		_mode = DEMOMODE_TRADEMARKS;
		_expiryTime = time;
		_counter = -1;
	}

	return true;
}

} // End of namespace U1Gfx
} // End of namespace Shared
} // End of namespace Ultima
