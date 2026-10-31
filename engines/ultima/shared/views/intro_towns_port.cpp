/* ScummVM - Graphic Adventure Engine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "ultima/shared/views/intro_towns_port.h"

#include "common/textconsole.h"

namespace Ultima {
namespace Shared {
namespace Views {

IntroTownsPort::IntroTownsPort(IntroTownsGraphics &graphics, Gfx::TextBox &box0,
		IntroKeyboardBuffer &keyboard) :
		_box0(box0), _graphics(graphics), _keyboard(keyboard) {
	if (!box0.isInitialized())
		error("IntroTownsPort: Text box 0 must be initialized");
}

bool IntroTownsPort::drawSlide(uint8 slideIndex) {
	return _graphics.drawSlide(slideIndex);
}

void IntroTownsPort::applyTextBoxFont(uint8 boxIndex) {
	if (boxIndex != 0)
		error("IntroTownsPort: Unsupported text box %u", boxIndex);
	_box0.applyFont();
}

void IntroTownsPort::setDisplayPage(int32 legacyMode) {
	if (!_graphics.setDisplayPage(legacyMode))
		error("IntroTownsPort: Unsupported display mode %d", legacyMode);
}

void IntroTownsPort::prepareTextBox(uint8 boxIndex) {
	if (boxIndex != 0)
		error("IntroTownsPort: Unsupported text box %u", boxIndex);
	_box0.prepare();
}

void IntroTownsPort::printTextBoxString(uint8 boxIndex, const char *sjisBytes) {
	if (boxIndex != 0)
		error("IntroTownsPort: Unsupported text box %u", boxIndex);
	_box0.printSjis(sjisBytes);
}

void IntroTownsPort::advanceTextBoxLine(uint8 boxIndex) {
	if (boxIndex != 0)
		error("IntroTownsPort: Unsupported text box %u", boxIndex);
	_box0.advanceLine();
}

void IntroTownsPort::resetBoxTo(uint8 boxIndex, int32 column, int32 row) {
	if (boxIndex != 0)
		error("IntroTownsPort: Unsupported text box %u", boxIndex);
	_box0.setCursor(column, row);
}

void IntroTownsPort::present() {
	_graphics.present();
}

void IntroTownsPort::clearPendingKeyboard() {
	_keyboard.clearPendingKeyPresses();
}

} // namespace Views
} // namespace Shared
} // namespace Ultima
