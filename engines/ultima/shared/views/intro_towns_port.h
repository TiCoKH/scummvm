/* ScummVM - Graphic Adventure Engine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef ULTIMA_SHARED_VIEWS_INTRO_TOWNS_PORT_H
#define ULTIMA_SHARED_VIEWS_INTRO_TOWNS_PORT_H

#include "ultima/shared/gfx/text_box.h"
#include "ultima/shared/views/intro_towns_graphics.h"
#include "ultima/shared/views/intro_view.h"

namespace Ultima {
namespace Shared {
namespace Views {

class IntroKeyboardBuffer {
public:
	virtual ~IntroKeyboardBuffer() {}
	virtual void clearPendingKeyPresses() = 0;
};

class IntroTownsPort : public IntroPort {
public:
	IntroTownsPort(IntroTownsGraphics &graphics, Gfx::TextBox &box0,
			IntroKeyboardBuffer &keyboard);

	bool drawSlide(uint8 slideIndex) override;
	void applyTextBoxFont(uint8 boxIndex) override;
	void setDisplayPage(int32 legacyMode) override;
	void prepareTextBox(uint8 boxIndex) override;
	void printTextBoxString(uint8 boxIndex, const char *sjisBytes) override;
	void advanceTextBoxLine(uint8 boxIndex) override;
	void resetBoxTo(uint8 boxIndex, int32 column, int32 row) override;
	void present() override;
	void clearPendingKeyboard() override;

private:
	Gfx::TextBox &_box0;
	IntroTownsGraphics &_graphics;
	IntroKeyboardBuffer &_keyboard;
};

} // namespace Views
} // namespace Shared
} // namespace Ultima

#endif
