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

#ifndef ULTIMA_ULTIMA1_GFX_FMT_VIEW_TOWNS_DEMO_H
#define ULTIMA_ULTIMA1_GFX_FMT_VIEW_TOWNS_DEMO_H

#include "ultima/shared/gfx/visual_container.h"
#include "graphics/managed_surface.h"

namespace Ultima {
namespace Ultima1 {
namespace U1Gfx {

using Shared::CShowMsg;
using Shared::CKeypressMsg;
using Shared::CFrameMsg;

/**
 * This class implements the header of FM Towns version of game,
 */
class ViewTownsDemo : public Shared::Gfx::VisualItem {
	DECLARE_MESSAGE_MAP;
	bool ShowMsg(CShowMsg *msg);
	bool KeypressMsg(CKeypressMsg *msg);
	bool FrameMsg(CFrameMsg *msg);
private:
	Graphics::ManagedSurface _header, _logo, _castle;
	Graphics::ManagedSurface _flags[3];
	Graphics::ManagedSurface _slides[4];
	enum DemoMode { DEMOMODE_HEADER, DEMOMODE_COPYRIGHT, DEMOMODE_PRESENTS, DEMOMODE_CASTLE, DEMOMODE_TRADEMARKS, DEMOMODE_MAIN_MENU };
	DemoMode _mode;
	uint32 _expiryTime;
	int _counter;
private:
	/**
	 * Shows the FM Towns game intro
	 */
	void drawDemoView();

	/**
	 * Draws the presents view
	 */
	void drawPresentsView();

	/**
	 * Draws the castle view
	 */
	void drawCastleView();

	/**
	 * Animates the castle flags
	 */
	void drawCastleFlag(Shared::Gfx::VisualSurface &s, int xp);

	/**
	 * Draws the trademarks view
	 */
	void drawTrademarksView();

	/**
	 * Draws the main menu
	 */
	void drawMainMenu();

	/**
	 * Sets up the palette for the castle view
	 */
	void setCastlePalette();

	/**
	 * Sets up the palette for the title views
	 */
	void setTitlePalette();

	/**
	 * Sets the current mode (display) within the title
	 */
	void setMode(DemoMode mode);
public:
	CLASSDEF;

	/**
	 * Constructor
	 */
	ViewTownsDemo(Shared::TreeItem *parent = nullptr);

	/**
	 * Draw the game screen
	 */
	void draw() override;
};

} // End of namespace U1Gfx
} // End of namespace Shared
} // End of namespace Ultima

#endif
