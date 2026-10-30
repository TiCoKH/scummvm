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

#ifndef ULTIMA_ULTIMA1_GFX_FMT_VIEW_TOWNS_TITLE_H
#define ULTIMA_ULTIMA1_GFX_FMT_VIEW_TOWNS_TITLE_H

#include "ultima/shared/gfx/visual_item.h"
#include "graphics/surface.h"

namespace Ultima {
namespace Ultima1 {
namespace U1Gfx {

using Shared::CShowMsg;
using Shared::CKeypressMsg;
using Shared::CFrameMsg;

/**
 * FM Towns title/menu presentation using original indexed assets.
 * Menu text and sequencing are provisional; gameplay views are not implemented.
 */
class ViewTownsTitle : public Shared::Gfx::VisualItem {
	DECLARE_MESSAGE_MAP;
	bool ShowMsg(CShowMsg *msg);
	bool KeypressMsg(CKeypressMsg *msg);
	bool FrameMsg(CFrameMsg *msg);
private:
	Graphics::Surface _top_header, _ut1_title, _ut1_dragon, _ut1_knight1, _ut1_knight2, _ut1_knight3;
	enum TitleMode { TITLEMODE_BASE, TITLEMODE_DRAGON, TITLEMODE_TRADEMARKS, TITLEMODE_PRESENTS, TITLEMODE_MAIN_MENU };
	TitleMode _mode;
	uint32 _expiryTime;
	int _counter;
private:
	/**
	 * Shows the initial copyright screen
	 */
	void drawCopyrightView();

	/**
	 * Draws the presents view
	 */
	void drawPresentsView();

	/**
	 * Draws the castle view
	 */
	void drawCastleView();

	void drawImage(const Graphics::Surface &image, int y);
	void drawText(const Common::String &text, int y);

	/**
	 * Draws the trademarks view
	 */
	void drawTrademarksView();

	/**
	 * Draws the main menu
	 */
	void drawMainMenu();

	/**
	 * Sets the current mode (display) within the title
	 */
	void setMode(TitleMode mode);
public:
	CLASSDEF;

	/**
	 * Constructor
	 */
	ViewTownsTitle(Shared::TreeItem *parent = nullptr);
	~ViewTownsTitle() override;

	/**
	 * Draw the game screen
	 */
	void draw() override;
};

} // End of namespace U1Gfx
} // End of namespace Ultima1
} // End of namespace Ultima

#endif
