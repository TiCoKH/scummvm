/* ScummVM - Graphic Adventure Engine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef ULTIMA_ULTIMA1_GFX_FMT_VIEW_TOWNS_DEMO_H
#define ULTIMA_ULTIMA1_GFX_FMT_VIEW_TOWNS_DEMO_H

#include "ultima/shared/gfx/visual_item.h"
#include "ultima/shared/views/intro_view.h"
#include "common/str.h"
#include "graphics/managed_surface.h"
#include "graphics/surface.h"

#ifdef USE_SJIS
namespace Graphics { class FontSJIS; }
#endif

namespace Ultima {
namespace Ultima1 {

class GameResources;

namespace U1Gfx {

using Shared::CShowMsg;
using Shared::CKeypressMsg;
using Shared::CFrameMsg;

class TownsIntroPort;

class ViewTownsDemo : public Shared::Gfx::VisualItem {
	DECLARE_MESSAGE_MAP;
	bool ShowMsg(CShowMsg *msg);
	bool KeypressMsg(CKeypressMsg *msg);
	bool FrameMsg(CFrameMsg *msg);

	friend class TownsIntroPort;

	Graphics::Surface _header;
	Graphics::Surface _slides[4];
	Graphics::ManagedSurface _textSurface;
	Common::String _demoText[Shared::Views::IntroAssets::kTextCount];
	Shared::Views::IntroAssets _introAssets;
	TownsIntroPort *_introPort;
	Shared::Views::IntroView *_intro;
	bool _menuVisible;
	int _textX;
	int _textY;
#ifdef USE_SJIS
	Graphics::FontSJIS *_sjisFont;
#endif

	bool loadImage(const Common::String &name, Graphics::Surface &image);
	bool drawSlide(uint8 slideIndex);
	void composePage0();
	void drawDemoString(const Common::String &text, int x, int y);
	void drawMainMenu();
	void setDemoStrings(const GameResources &resources);
	void finishIntro();
	void setMenuText(const Common::String &text);

public:
	CLASSDEF;

	explicit ViewTownsDemo(Shared::TreeItem *parent = nullptr);
	~ViewTownsDemo() override;

	void draw() override;
	void startIntro(const GameResources &resources);
};

} // End of namespace U1Gfx
} // End of namespace Ultima1
} // End of namespace Ultima

#endif
