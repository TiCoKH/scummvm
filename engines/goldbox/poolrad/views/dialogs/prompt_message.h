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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_PROMPT_MESSAGE_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_PROMPT_MESSAGE_H

#include "goldbox/poolrad/views/dialogs/dialog.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * Configuration for PromptMessage dialog.
 * Default background color is black (0), text color is light green (10).
 * Display duration automatically uses engine's global textDelay setting.
 */
struct PromptMessageConfig {
	Common::String message;
	int textColor = 10;      // Light green (indices match original palette)
	int backgroundColor = 0; // Black
};

/**
 * Temporary overlay message that displays at row 24 for a duration based on
 * the engine's global textDelay setting, then automatically closes and triggers
 * parent redraw.
 *
 * Usage:
 *   PromptMessageConfig config;
 *   config.message = "Operation complete!";
 *   config.textColor = 10;
 *   auto msg = new PromptMessage("PromptMsg", config);
 *   attachDialog(msg);  // Parent view/dialog manages lifecycle
 *
 * The dialog automatically:
 *   1. Clears row 24
 *   2. Displays the message
 *   3. Sets a timer based on message length and engine's textDelay (1-5)
 *   4. Closes when timer expires
 *   5. Parent view redraws automatically
 *
 * IMPORTANT: Parent dialog/view must manage memory - call delete in destructor.
 */
class PromptMessage : public Dialog {
private:
	Common::String _message;
	int _textColor;
	int _backgroundColor;
	bool _shown = false;

	/**
	 * Calculate display duration in frames based on engine's textDelay setting.
	 * textDelay 1 (fast): 1 second
	 * textDelay 3 (medium): 2 seconds
	 * textDelay 5 (slow): 3 seconds
	 */
	uint _calculateDisplayFrames() const;

public:
	/**
	 * Constructor. Does not display immediately.
	 * Call activate() to show the message.
	 */
	PromptMessage(const Common::String &name, const PromptMessageConfig &config);

	/**
	 * Display the message and start the auto-close timer.
	 * Called by parent dialog/view via attachDialog().
	 */
	void activate() override;

	/**
	 * Draw the message on screen.
	 * Called automatically by the event system.
	 */
	void draw() override;

	/**
	 * Called when the display timeout expires.
	 * Closes the dialog, triggering parent redraw.
	 */
	void timeout() override;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_PROMPT_MESSAGE_H
