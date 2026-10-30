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
#include "ultima/shared/early/ultima_early.h"
#include "ultima/shared/early/game.h"
#include "ultima/shared/gfx/font.h"
#include "ultima/shared/gfx/screen.h"
#include "ultima/shared/gfx/text_cursor.h"
#include "ultima/shared/gfx/tiff_loader.h"
#include "common/file.h"
#include "common/textconsole.h"
#include "common/translation.h"
#include "gui/message.h"

namespace Ultima {
namespace Ultima1 {
namespace U1Gfx {

BEGIN_MESSAGE_MAP(ViewTownsTitle, Shared::Gfx::VisualItem)
	ON_MESSAGE(ShowMsg)
	ON_MESSAGE(KeypressMsg)
	ON_MESSAGE(FrameMsg)
END_MESSAGE_MAP()

ViewTownsTitle::ViewTownsTitle(Shared::TreeItem *parent) :
		Shared::Gfx::VisualItem("TownsTitle", Rect(0, 0, 640, 480), parent),
		_mode(TITLEMODE_BASE), _expiryTime(0), _counter(0) {
	const char *const names[] = {
		"HEADTTL.TIF", "UT1T_1.TIF", "UT1T_2.TIF",
		"UT1T_3.TIF", "UT1T_4.TIF", "UT1T_5.TIF"
	};
	Graphics::Surface *const images[] = {
		&_top_header, &_ut1_title, &_ut1_dragon,
		&_ut1_knight1, &_ut1_knight2, &_ut1_knight3
	};
	for (uint i = 0; i < ARRAYSIZE(names); ++i) {
		Common::String path = Common::String::format("GRAPH/%s", names[i]);
		if (!Common::File::exists(Common::Path(path)))
			path = names[i];
		if (!Shared::Gfx::TIFFLoader::loadTIFFImage(path, images[i]))
			error("Could not load required FM Towns title image '%s'", names[i]);
		const bool validSize = i == 0 ? images[i]->w == 640 && images[i]->h == 24 :
			images[i]->w <= 608 && images[i]->h <= 360;
		if (!images[i]->format.isCLUT8() || !validSize)
			error("Invalid FM Towns title image '%s'", names[i]);
	}
}

ViewTownsTitle::~ViewTownsTitle() {
	_top_header.free();
	_ut1_title.free();
	_ut1_dragon.free();
	_ut1_knight1.free();
	_ut1_knight2.free();
	_ut1_knight3.free();
}

void ViewTownsTitle::drawImage(const Graphics::Surface &image, int y) {
	g_vm->_screen->blitFrom(image, Common::Point((640 - image.w) / 2, y));
}

void ViewTownsTitle::drawText(const Common::String &text, int y) {
	Shared::Point pt((640 - int(getGame()->getFont()->stringWidth(text))) / 2, y);
	getGame()->getFont()->writeString(*g_vm->_screen, text, pt, 15, 0);
}

void ViewTownsTitle::draw() {
	Graphics::ManagedSurface &screen = *g_vm->_screen;
	screen.clear();
	drawImage(_top_header, 0);
	screen.frameRect(Common::Rect(16, 40, 624, 432), 15);
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
	}
	drawText(_mode == TITLEMODE_MAIN_MENU ?
		"FM Towns support in development" : "Press any key for the menu", 448);
}

void ViewTownsTitle::drawCopyrightView() {
	drawImage(_ut1_title, 96);
	drawText("Ultima I - The First Age of Darkness", 288);
}

void ViewTownsTitle::drawPresentsView() {
	drawImage(_ut1_dragon, 96);
	drawText("Ultima Trilogy - FM Towns", 288);
}

void ViewTownsTitle::drawCastleView() {
	const Graphics::Surface *const images[] = {&_ut1_knight1, &_ut1_knight2, &_ut1_knight3};
	drawImage(*images[_counter], 80);
}

void ViewTownsTitle::drawTrademarksView() {
	drawImage(_ut1_title, 80);
	drawText("Ultima I - FM Towns", 288);
	drawText("Original game graphics", 312);
}

void ViewTownsTitle::drawMainMenu() {
	drawImage(_ut1_title, 64);
	drawText("a) Generate new character", 256);
	drawText("b) Continue previous game", 280);
	drawText("r) Replay title sequence", 304);
	drawText("q) Quit", 328);
	drawText("Character generation and save loading are not implemented.", 376);
}

void ViewTownsTitle::setMode(TitleMode mode) {
	_mode = mode;
	_counter = 0;
	_expiryTime = getGame()->getMillis() + 3000;
	getGame()->_textCursor->setVisible(false);
	setDirty();
}

bool ViewTownsTitle::FrameMsg(CFrameMsg *msg) {
	if (_mode == TITLEMODE_MAIN_MENU ||
			int32(getGame()->getMillis() - _expiryTime) < 0)
		return true;
	switch (_mode) {
	case TITLEMODE_BASE:
		setMode(TITLEMODE_PRESENTS);
		break;
	case TITLEMODE_PRESENTS:
		setMode(TITLEMODE_DRAGON);
		break;
	case TITLEMODE_DRAGON:
		if (++_counter == 3) {
			setMode(TITLEMODE_TRADEMARKS);
		} else {
			_expiryTime = getGame()->getMillis() + 3000;
			setDirty();
		}
		break;
	case TITLEMODE_TRADEMARKS:
		setMode(TITLEMODE_MAIN_MENU);
		break;
	default:
		break;
	}
	return true;
}

bool ViewTownsTitle::ShowMsg(CShowMsg *msg) {
	Shared::Gfx::VisualItem::ShowMsg(msg);
	setMode(TITLEMODE_BASE);
	return true;
}

bool ViewTownsTitle::KeypressMsg(CKeypressMsg *msg) {
	if (_mode != TITLEMODE_MAIN_MENU) {
		setMode(TITLEMODE_MAIN_MENU);
		return true;
	}
	switch (msg->_keyState.keycode) {
	case Common::KEYCODE_a: {
		GUI::MessageDialog dialog(_("FM Towns character generation is not implemented yet."));
		dialog.runModal();
		setDirty();
		break;
	}
	case Common::KEYCODE_b: {
		GUI::MessageDialog dialog(_("FM Towns save loading is not implemented yet."));
		dialog.runModal();
		setDirty();
		break;
	}
	case Common::KEYCODE_r:
		setMode(TITLEMODE_BASE);
		break;
	case Common::KEYCODE_ESCAPE:
	case Common::KEYCODE_q:
		g_vm->quitGame();
		break;
	default:
		break;
	}
	return true;
}

} // End of namespace U1Gfx
} // End of namespace Ultima1
} // End of namespace Ultima
