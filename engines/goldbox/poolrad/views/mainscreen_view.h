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

#ifndef GOLDBOX_POOLRAD_VIEWS_MAINSCREEN_VIEW_H
#define GOLDBOX_POOLRAD_VIEWS_MAINSCREEN_VIEW_H

#include "goldbox/poolrad/views/view.h"
#include "goldbox/core/global.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

/**
 * Base class for the main screen layout shared by most game states.
 * Draws the base window layout (3 fixed windows + 1 optional mini window).
 * Derived views add state-specific overlays (portraits, party list, etc.).
 */
class MainScreenView : public View {
protected:
	bool _showMiniWindow = false;

	/**
	 * Draws the base main screen windows.
	 * Mirrors Amiga SCREEN_DrawMainWindows(param) behavior:
	 * - Always draws: (1,1,38,22), (1,17,38,22), (1,1,15,15)
	 * - Conditionally draws: (3,3,13,13) when _showMiniWindow is true
	 */
	void drawBaseWindows();

public:
	MainScreenView(const Common::String &name) : View(name) {}
	virtual ~MainScreenView() {}

	/**
	 * Sets whether the small 13x13 window should be shown.
	 * Called by the engine router based on game state.
	 */
	void setShowMiniWindow(bool show) { _showMiniWindow = show; }

	/**
	 * Returns whether the mini window is shown.
	 */
	bool getShowMiniWindow() const { return _showMiniWindow; }

	/**
	 * Hook called when entering this view with a new game state.
	 * Derived classes override to perform one-time setup (load resources, etc.).
	 */
	virtual void onEnter(GameState state) {}

	/**
	 * Hook called every frame to perform lightweight updates.
	 * Derived classes override for idempotent operations (map color refresh, etc.).
	 */
	virtual void onUpdate() {}
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_MAINSCREEN_VIEW_H
