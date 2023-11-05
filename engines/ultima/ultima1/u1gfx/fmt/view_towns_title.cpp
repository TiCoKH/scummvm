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

#include "ultima/ultima1/u1gfx/fmt/view_towns_title.h"
#include "ultima/ultima1/u1gfx/drawing_support.h"
#include "ultima/ultima1/core/resources.h"
#include "ultima/ultima1/game.h"
#include "ultima/shared/core/file.h"
#include "ultima/shared/gfx/text_cursor.h"
#include "ultima/shared/gfx/tiff_loader.h"
#include "ultima/shared/early/font_resources.h"
#include "ultima/shared/early/ultima_early.h"


namespace Ultima {
namespace Ultima1 {
namespace U1Gfx {

BEGIN_MESSAGE_MAP(ViewTownsTitle, Shared::Gfx::VisualItem)
	ON_MESSAGE(ShowMsg)
	ON_MESSAGE(KeypressMsg)
	ON_MESSAGE(FrameMsg)
END_MESSAGE_MAP()

ViewTownsTitle::ViewTownsTitle(Shared::TreeItem *parent) : Shared::Gfx::VisualItem("TownsTitle", Rect(0, 0, 640, 480), parent) {
	setMode(TITLEMODE_BASE);

	Ultima::Shared::Gfx::TIFFLoader::loadTIFFImage("GRAPH/HEADTTL.TIF", &_top_header);
	Ultima::Shared::Gfx::TIFFLoader::loadTIFFImage("GRAPH/UT1T_1.TIF", &_ut1_title);
	Ultima::Shared::Gfx::TIFFLoader::loadTIFFImage("GRAPH/UT1T_2.TIF", &_ut1_dragon);
	Ultima::Shared::Gfx::TIFFLoader::loadTIFFImage("GRAPH/UT1T_3.TIF", &_ut1_knight1);
	Ultima::Shared::Gfx::TIFFLoader::loadTIFFImage("GRAPH/UT1T_4.TIF", &_ut1_knight2);
	Ultima::Shared::Gfx::TIFFLoader::loadTIFFImage("GRAPH/UT1T_5.TIF", &_ut1_knight3);

}

void ViewTownsTitle::draw() {
	VisualItem::draw();

	switch (_mode) {
	case TITLEMODE_BASE:
		drawCopyrightView();
		break;

	case TITLEMODE_PRESENTS:
		drawPresentsView();
		break;

	case TITLEMODE_DRAGON:
		drawCastleView();
		break;

	case TITLEMODE_TRADEMARKS:
		drawTrademarksView();
		break;

	case TITLEMODE_MAIN_MENU:
		drawMainMenu();
		break;

	default:
		break;
	}
}

void ViewTownsTitle::drawCopyrightView() {
	Ultima1Game *game = static_cast<Ultima1Game *>(getGame());
	Shared::Gfx::VisualSurface s = getSurface();
	s.clear();

	// Draw horizontal title lines
	for (int idx = 0; idx < 3; ++idx) {
		s.hLine(112, idx + 58, 200, 1);
		s.hLine(112, idx + 74, 200, 1);
	}

	// Write text
	s.writeString(game->_res->TITLE_MESSAGES[0], TextPoint(16, 8), game->_whiteColor);
	s.writeString(game->_res->TITLE_MESSAGES[1], TextPoint(8, 11), game->_whiteColor);
	s.writeString(game->_res->TITLE_MESSAGES[2], TextPoint(0, 21), game->_whiteColor);
}

void ViewTownsTitle::drawPresentsView() {
	Ultima1Game *game = static_cast<Ultima1Game *>(getGame());
	Shared::Gfx::VisualSurface s = getSurface();

	switch (_counter) {
	case 0:
		s.clear();
		s.blitFrom(_ut1_title, Point(20, 21));
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

void ViewTownsTitle::drawCastleView() {
	Shared::Gfx::VisualSurface s = getSurface();
	if (_counter == 0)
		s.blitFrom(_ut1_knight2);

	drawCastleFlag(s, 123);
	drawCastleFlag(s, 196);
}

void ViewTownsTitle::drawCastleFlag(Shared::Gfx::VisualSurface &s, int xp) {
	s.blitFrom(_ut1_knight1, Common::Point(xp, 55));
}

void ViewTownsTitle::drawTrademarksView() {
	Ultima1Game *game = static_cast<Ultima1Game *>(getGame());
	Shared::Gfx::VisualSurface s = getSurface();
	if (_counter == 0)
		s.clear();

	if (_counter < 32) {
		s.blitFrom(_ut1_title, Common::Rect(0, 0, _ut1_title.w, _counter + 1), Point(20, 21));
		s.blitFrom(_ut1_title, Common::Rect(0, _ut1_title.h - _counter - 1, _ut1_title.w, _ut1_title.h),
			Common::Point(20, 21 + _ut1_title.h - _counter - 1));
	} else {
		s.writeString(game->_res->TITLE_MESSAGES[9], TextPoint(1, 17));
		s.writeString(game->_res->TITLE_MESSAGES[10], TextPoint(2, 18));
		s.writeString(game->_res->TITLE_MESSAGES[11], TextPoint(11, 19));
		s.writeString(game->_res->TITLE_MESSAGES[12], TextPoint(6, 23));
	}
}

void ViewTownsTitle::drawMainMenu() {
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

void ViewTownsTitle::setTitlePalette() {
	const byte PALETTE[] = { 0, 1, 2, 3, 4, 5, 6, 7, 56, 57, 58, 59, 60, 61, 62, 63 };
	getGame()->setEGAPalette(PALETTE);
}

void ViewTownsTitle::setCastlePalette() {
	const byte PALETTE[] = { 0, 24, 7, 63, 63, 34, 58, 14, 20, 7, 61, 59, 1, 57, 7, 63 };
	getGame()->setEGAPalette(PALETTE);
}

bool ViewTownsTitle::FrameMsg(CFrameMsg *msg) {
	uint32 time = getGame()->getMillis();
	if (time < _expiryTime)
		return true;
	setDirty();

	switch (_mode) {
	case TITLEMODE_BASE:
		setMode(TITLEMODE_BASE);
		break;
	case TITLEMODE_DRAGON:
		_expiryTime = time + 3000;
		if (++_counter == 3)
			setMode(TITLEMODE_BASE);
		break;

	case TITLEMODE_TRADEMARKS:
		_expiryTime = time + 200;
		if (++_counter == 100)
			setMode(TITLEMODE_BASE);
		break;

	case TITLEMODE_MAIN_MENU:
		_expiryTime = time + 20;
		++_counter;
		if (_counter == 32) {
			_expiryTime = time + 4000;
		} else if (_counter == 33) {
			setMode(TITLEMODE_MAIN_MENU);
		}
		break;

	default:
		break;
	}

	return true;
}

void ViewTownsTitle::setMode(TitleMode mode) {
	_expiryTime = getGame()->getMillis();
	_counter = 0;
	_mode = mode;
	setDirty();
	setTitlePalette();

	switch (mode) {
	case TITLEMODE_TRADEMARKS:
		_expiryTime += 4000;
		break;
	case TITLEMODE_PRESENTS:
		_expiryTime += 3000;
		break;
	case TITLEMODE_DRAGON:
		setCastlePalette();
		break;
	case TITLEMODE_MAIN_MENU: {
		Shared::Gfx::TextCursor *textCursor = getGame()->_textCursor;
		textCursor->setPosition(TextPoint(25, 18));
		textCursor->setVisible(true);
		break;
	}
	default:
		break;
	}
}

bool ViewTownsTitle::ShowMsg(CShowMsg *msg) {
	Shared::Gfx::VisualItem::ShowMsg(msg);

	if (_mode == TITLEMODE_MAIN_MENU) {
		// Returning to main menu from another screen
		setMode(TITLEMODE_MAIN_MENU);
	}

	return true;
}

bool ViewTownsTitle::KeypressMsg(CKeypressMsg *msg) {
	uint32 time = getGame()->getMillis();

	if (_mode == TITLEMODE_MAIN_MENU) {
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

	} else if (_mode != TITLEMODE_TRADEMARKS) {
		// Switch to the trademarks view
		_mode = TITLEMODE_TRADEMARKS;
		_expiryTime = time;
		_counter = -1;
	}

	return true;
}

} // End of namespace U1Gfx
} // End of namespace Shared
} // End of namespace Ultima
