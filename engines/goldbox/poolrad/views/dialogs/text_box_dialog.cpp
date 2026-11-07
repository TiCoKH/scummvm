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

#include "goldbox/poolrad/views/dialogs/text_box_dialog.h"
#include "goldbox/engine.h"
#include "goldbox/events.h"
#include "goldbox/gfx/game_text.h"
#include "goldbox/gfx/surface.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

TextBoxDialog::TextBoxDialog(const Common::String &name)
		: Dialog(name),
		  _gameText(g_engine ? &g_engine->getGameText() : nullptr),
		  _frameCounter(0), _framesPerWord(0) {
	setBounds(Window(0, 0, 39, 24));
}

void TextBoxDialog::activate() {
	Dialog::activate();
	_pendingDrawStep = false;
}

void TextBoxDialog::clearText() {
	_frameCounter = 0;
	_framesPerWord = 0;
	_pendingDrawStep = false;
	_timedPause = false;
	_eraseArea = true;
	if (_gameText)
		_gameText->clearMessageArea();
	redraw();
}

void TextBoxDialog::setText(const Common::String &text, bool clearBox) {
	_frameCounter = 0;
	_pendingDrawStep = false;
	_timedPause = false;
	const uint speed = VmInterface::getTextDelay();
	_framesPerWord = speed;
	if (_gameText)
		_gameText->setText(text, clearBox);
	redraw();
}

void TextBoxDialog::showMessage(::Goldbox::Data::PlayerCharacter *speaker,
		const Common::String &text, uint8 row, bool timed) {
	_frameCounter = 0;
	_pendingDrawStep = false;
	_timedPause = timed;
	_pauseCounter = 0;
	_framesPerWord = VmInterface::getTextDelay();
	if (_gameText)
		_gameText->showMessage(speaker, text, row,
				GameText::INGAME_MESSAGE, timed);
	redraw();
}

void TextBoxDialog::draw() {
	if (!_isVisible || !_gameText)
		return;

	// An inactive GameText draws nothing, so a clear must erase explicitly.
	if (_eraseArea) {
		_eraseArea = false;
		Surface eraseSurface = getSurface();
		eraseSurface.clearBox(1, 17, 38, 22, 0);
	}

	if (_pendingDrawStep) {
		_pendingDrawStep = false;
		_gameText->advance();
	}

	if (_framesPerWord == 0) {
		while (_gameText->advance())
			;
	}

	Surface surface = getSurface();
	_gameText->draw(surface);
	if (_gameText->isWaitingForKey()) {
		surface.clearBox(0, 24, 39, 24, 0);
		// DisplayMessageWindow page break: LightMagenta prompt on row 24.
		surface.writeStringC(0, 24, 13, "PRESS ANY KEY TO CONTINUE");
	}
}

bool TextBoxDialog::tick() {
	// PauseOnly (gameDelay * 225 ms), then ClearMessageWindow.
	if (_timedPause && _gameText && !_gameText->isBusy()) {
		if (++_pauseCounter >= VmInterface::getTextDelay() * FRAME_RATE / 2)
			clearText();
		return true;
	}

	if (!_gameText || _gameText->isWaitingForKey()
			|| !_gameText->isBusy())
		return false;

	if (_framesPerWord == 0) {
		redraw();
		return true;
	}

	++_frameCounter;
	if (_frameCounter >= _framesPerWord) {
		_frameCounter = 0;
		_pendingDrawStep = true;
		redraw();
	}
	return true;
}

bool TextBoxDialog::msgKeypress(const KeypressMessage &msg) {
	// The original pause ignores the keyboard.
	if (_timedPause)
		return true;

	if (!_gameText || !_gameText->isWaitingForKey())
		return false;

	(void)msg;
	_gameText->nextPage();
	_pendingDrawStep = false;
	Surface surface = getSurface();
	surface.clearBox(0, 24, 39, 24, 0);
	redraw();
	return true;
}

bool TextBoxDialog::isBusy() const {
	return _gameText && _gameText->isBusy();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
