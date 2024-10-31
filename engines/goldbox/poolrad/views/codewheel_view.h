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

#ifndef GOLDBOX_POOLRAD_VIEWS_CODEWHEEL_VIEW_H
#define GOLDBOX_POOLRAD_VIEWS_CODEWHEEL_VIEW_H

#include "graphics/managed_surface.h"
#include "goldbox/poolrad/views/view.h"
#include "goldbox/poolrad/views/dialogs/horizontal_input_txt.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

struct Passcode {
    const char* answer;
    uint8 outer[2];
    uint8 inner[2];
    const char* path;
};

class CodewheelView : public View {
private:

	Passcode passcodes[13] = {
		{"BEWARE", {0x1D, 0x1E}, {0x6C, 0x70}, "- - - - -"},
		{"ZOMBIE", {0x5B, 0x5C}, {0x4D, 0xFF}, "-..-..-.."},
		{"NOTNOW", {0x2C, 0x2D}, {0x6A, 0xFF}, "........."},
		{"COPPER", {0x09, 0x0A}, {0x43, 0xFF}, "- - - - -"},
		{"DRAGON", {0x01, 0x02}, {0x68, 0xFF}, "-..-..-.."},
		{"EFREET", {0x11, 0x12}, {0x6D, 0xFF}, "........."},
		{"FRIEND", {0x17, 0x18}, {0x3F, 0xFF}, "- - - - -"},
		{"JUNGLE", {0x19, 0x1A}, {0x4E, 0xFF}, "-..-..-.."},
		{"KNIGHT", {0x22, 0x23}, {0x6C, 0xFF}, "........."},
		{"SAVIOR", {0x2E, 0x2F}, {0x42, 0xFF}, "- - - - -"},
		{"TEMPLE", {0x36, 0x37}, {0x4B, 0xFF}, "-..-..-.."},
		{"VULCAN", {0x34, 0x35}, {0x6E, 0xFF}, "........."},
		{"WYVERN", {0x2C, 0x2D}, {0x47, 0xFF}, "- - - - -"}
	};

	Dialogs::HorizontalInputTxt *_inputPrompt;
	int _retry = 0;
	int _code_index = 0;

public:
	CodewheelView();
	virtual ~CodewheelView();

	bool msgKeypress(const KeypressMessage &msg) override;
	bool msgFocus(const FocusMessage &msg) override;
	bool msgUnfocus(const UnfocusMessage &msg) override;
	void draw() override;
	void timeout() override;
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
