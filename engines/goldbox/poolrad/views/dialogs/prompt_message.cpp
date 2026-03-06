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

#include "goldbox/poolrad/views/dialogs/prompt_message.h"
#include "goldbox/events.h"
#include "goldbox/gfx/surface.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

PromptMessage::PromptMessage(const Common::String &name, const PromptMessageConfig &config)
	: Dialog(name),
	  _message(config.message),
	  _textColor(config.textColor),
	  _backgroundColor(config.backgroundColor) {
	// Constructor: prepare but don't display
	_shown = false;

	// Optional: Set bounds to row 24 only (text coordinates)
	// This restricts drawing to the message area
	setBounds(Window(0, 24, 39, 24));
}

uint PromptMessage::_calculateDisplayFrames() const {
	uint textDelay = VmInterface::getTextDelay();

	// Simple formula: delay directly controls display time
	// textDelay * FRAME_RATE / 2 gives:
	// textDelay 1: 10 frames (0.5 seconds)
	// textDelay 2: 20 frames (1.0 second)
	// textDelay 3: 30 frames (1.5 seconds)
	// textDelay 4: 40 frames (2.0 seconds)
	// textDelay 5: 50 frames (2.5 seconds)
	return textDelay * FRAME_RATE / 2;
}

void PromptMessage::activate() {
	Dialog::activate();
	_shown = true;
	redraw();

	// Start the auto-close timer
	uint displayFrames = _calculateDisplayFrames();
	delayFrames(displayFrames);

	debug("PromptMessage::activate() - Displaying for %u frames: \"%s\"",
		displayFrames, _message.c_str());
}

void PromptMessage::draw() {
	if (!_shown)
		return;

	Surface s = getSurface();
	s.clearBox(0, 24, 39, 24, _backgroundColor);
	s.writeStringC(0, 24, _textColor, _message);
}

void PromptMessage::timeout() {
	// Called when display timer expires
	debug("PromptMessage::timeout() - Auto-closing message");
	Surface s = getSurface();
	s.clearBox(0, 24, 39, 24, _backgroundColor);
	close();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
