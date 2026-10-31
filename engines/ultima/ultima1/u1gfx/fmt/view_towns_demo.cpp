/* ScummVM - Graphic Adventure Engine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "ultima/ultima1/u1gfx/fmt/view_towns_demo.h"
#include "ultima/ultima1/core/resources.h"
#include "ultima/ultima1/game.h"
#include "ultima/shared/early/ultima_early.h"
#include "ultima/shared/early/game.h"
#include "ultima/shared/gfx/font.h"
#include "ultima/shared/gfx/screen.h"
#include "ultima/shared/gfx/tiff_loader.h"
#include "ultima/shared/gfx/towns_screen.h"
#ifdef USE_SJIS
#include "graphics/sjis.h"
#endif
#include "common/array.h"
#include "common/events.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/translation.h"
#include "gui/message.h"

namespace Ultima {
namespace Ultima1 {
namespace U1Gfx {

namespace {

const uint32 kMillisPerDemoDelayUnit = 10;

} // End of anonymous namespace

class TownsIntroPort : public Shared::Views::IntroPort {
	ViewTownsDemo &_view;

public:
	explicit TownsIntroPort(ViewTownsDemo &view) : _view(view) {}

	bool drawSlide(uint8 slideIndex) override {
		return _view.drawSlide(slideIndex);
	}

	void applyTextBoxFont(uint8 boxIndex) override {
		if (boxIndex != 0)
			error("Unsupported FM Towns intro text box %u", boxIndex);
	}

	void setDisplayPage(int32 legacyMode) override {
		if (!g_vm->_townsScreen->setLegacyDisplayPage(legacyMode))
			error("Unsupported FM Towns intro display mode %d", legacyMode);
	}

	void prepareTextBox(uint8 boxIndex) override {
		if (boxIndex != 0)
			error("Unsupported FM Towns intro text box %u", boxIndex);
		_view._textSurface.clear(0);
	}

	void printTextBoxString(uint8 boxIndex, const char *bytes) override {
		if (boxIndex != 0)
			error("Unsupported FM Towns intro text box %u", boxIndex);
		_view.drawDemoString(Common::String(bytes), _view._textX, _view._textY);
	}

	void advanceTextBoxLine(uint8 boxIndex) override {
		if (boxIndex != 0)
			error("Unsupported FM Towns intro text box %u", boxIndex);
		_view._textX = 32;
		_view._textY += 8;
	}

	void resetBoxTo(uint8 boxIndex, int32 arg1, int32 arg2) override {
		if (boxIndex != 0 || arg1 != 0 || arg2 != 0)
			error("Unsupported FM Towns intro text box reset");
		_view._textSurface.clear(0);
		_view._textX = 32;
		_view._textY = 48;
	}

	void present() override {
		_view.composePage0();
		g_vm->_townsScreen->update();
		g_system->updateScreen();
	}

	void clearPendingKeyboard() override {
		Common::Array<Common::Event> pending;
		Common::Event event;
		Common::EventManager *eventManager = g_system->getEventManager();
		while (eventManager->pollEvent(event)) {
			if (event.type != Common::EVENT_KEYDOWN &&
					event.type != Common::EVENT_KEYUP)
				pending.push_back(event);
		}
		for (uint i = 0; i < pending.size(); ++i)
			eventManager->pushEvent(pending[i]);
	}
};

BEGIN_MESSAGE_MAP(ViewTownsDemo, Shared::Gfx::VisualItem)
	ON_MESSAGE(ShowMsg)
	ON_MESSAGE(KeypressMsg)
	ON_MESSAGE(FrameMsg)
END_MESSAGE_MAP()

ViewTownsDemo::ViewTownsDemo(Shared::TreeItem *parent) :
		Shared::Gfx::VisualItem("TownsIntro", Rect(0, 0, 640, 480), parent),
		_textSurface(640, 240), _introPort(new TownsIntroPort(*this)),
		_intro(nullptr), _menuVisible(false), _textX(32), _textY(48) {
#ifdef USE_SJIS
	_sjisFont = Graphics::FontSJIS::createFont(Common::kPlatformFMTowns);
	if (!_sjisFont)
		warning("FM Towns SJIS font not available (FMT_FNT.ROM or SJIS.FNT missing)");
#endif
	if (!loadImage("HEADTTL.TIF", _header) || _header.w != 640 ||
			_header.h != 24 || !_header.format.isCLUT8())
		error("Could not load FM Towns intro header");

	const char *const names[] = {
		"UT1_1.TIF", "UT1_2.TIF", "UT1_3.TIF", "UT1_4.TIF"
	};
	for (uint i = 0; i < ARRAYSIZE(names); ++i) {
		if (!loadImage(names[i], _slides[i]) || _slides[i].format.bytesPerPixel != 2)
			error("Could not load FM Towns intro slide '%s'", names[i]);
	}
}

ViewTownsDemo::~ViewTownsDemo() {
	delete _intro;
	delete _introPort;
#ifdef USE_SJIS
	delete _sjisFont;
#endif
	_header.free();
	for (Graphics::Surface &slide : _slides)
		slide.free();
}

bool ViewTownsDemo::loadImage(const Common::String &name, Graphics::Surface &image) {
	return Shared::Gfx::TIFFLoader::loadTIFFImage(name, &image);
}

void ViewTownsDemo::setDemoStrings(const GameResources &resources) {
	for (uint i = 0; i < 13; ++i)
		_demoText[i] = resources.TITLE_MESSAGES[i];
	for (uint i = 0; i < 7; ++i)
		_demoText[13 + i] = resources.MAIN_MENU_TEXT[i];

	// The script advances these three source pointers by one byte. Prefix the
	// demo fallback strings so that operation retains the complete readable text.
	for (uint i = 17; i <= 19; ++i)
		_demoText[i] = Common::String::format(" %s", _demoText[i].c_str());
	_demoText[20] = resources.PRESS_SPACE_TO_CONTINUE;

	for (uint i = 0; i < Shared::Views::IntroAssets::kTextCount; ++i)
		_introAssets.text[i] = _demoText[i].c_str();
}

void ViewTownsDemo::startIntro(const GameResources &resources) {
	setDemoStrings(resources);
	delete _intro;
	_intro = new Shared::Views::IntroView(*_introPort, _introAssets,
			kMillisPerDemoDelayUnit);
	_menuVisible = false;
	_intro->start(getGame()->getMillis());
	if (_intro->state() == Shared::Views::IntroView::kFailed)
		error("Could not start FM Towns intro sequence");
}

bool ViewTownsDemo::drawSlide(uint8 slideIndex) {
	if (slideIndex >= ARRAYSIZE(_slides)) {
		warning("ViewTownsDemo::drawSlide(): Invalid slide %u", slideIndex);
		return false;
	}

	const Graphics::Surface &slide = _slides[slideIndex];
	const int x = 31;
	const int y = 16;
	if (x + slide.w > g_vm->_townsScreen->getLayerWidth(1) ||
			y + slide.h > g_vm->_townsScreen->getLayerHeight(1)) {
		warning("ViewTownsDemo::drawSlide(): Slide does not fit page 1");
		return false;
	}

	g_vm->_townsScreen->clearLayer(1);
	g_vm->_townsScreen->setWritePage(1);
	// Decoded surfaces contain native-endian pixels; putPixelBlock takes raw LE data.
	g_vm->_townsScreen->getWriteSurface().copyRectToSurface(slide, x, y,
			Common::Rect(slide.w, slide.h));
	g_vm->_townsScreen->invalidateRect(x, y, slide.w, slide.h);
	g_vm->_townsScreen->setWritePage(0);

	_textSurface.clear(0);
	return true;
}

void ViewTownsDemo::drawDemoString(const Common::String &text, int x, int y) {
#ifdef USE_SJIS
	if (_sjisFont) {
		const byte *p = reinterpret_cast<const byte *>(text.c_str());
		int cx = x;
		while (*p) {
			uint16 ch;
			if ((*p >= 0x81 && *p <= 0x9F) || (*p >= 0xE0 && *p <= 0xEF)) {
				if (!p[1]) break;
				ch = (p[0] << 8) | p[1];
				p += 2;
			} else {
				ch = *p++;
			}
			_sjisFont->drawChar(_textSurface.rawSurface(), ch, cx, y, 15, 0);
			cx += _sjisFont->getCharWidth(ch);
		}
		return;
	}
#endif
	Shared::Point pos(x, y);
	getGame()->getFont()->writeString(_textSurface, text, pos, 15, 0);
}

void ViewTownsDemo::composePage0() {
	g_vm->_townsScreen->setWritePage(0);
	Graphics::ManagedSurface &page = g_vm->_townsScreen->getWriteSurface();
	uint8 *dest = static_cast<uint8 *>(page.getPixels());

	for (int y = 0; y < 480; ++y) {
		const uint8 *src = static_cast<const uint8 *>(_textSurface.getBasePtr(0, y / 2));
		memcpy(dest + y * page.pitch, src, 640);
	}

	page.copyRectToSurface(_header, 0, 0, Common::Rect(_header.w, _header.h));
	g_vm->_townsScreen->invalidateRect(0, 0, 640, 480);
}

void ViewTownsDemo::drawMainMenu() {
	_textSurface.clear(0);
	drawDemoString(_demoText[13], 248, 28);
	drawDemoString(_demoText[14], 264, 52);
	drawDemoString(_demoText[15], 264, 60);
	drawDemoString(_demoText[16], 264, 68);
	drawDemoString(_demoText[17].substr(1), 160, 96);
	drawDemoString(_demoText[18].substr(1), 160, 112);
	drawDemoString(_demoText[19].substr(1), 160, 128);
	drawDemoString(_("r) Replay demo"), 160, 152);
	drawDemoString(_("q) Quit"), 160, 168);
	_introPort->present();
}

void ViewTownsDemo::finishIntro() {
	const Shared::Views::IntroView::State state = _intro->state();
	if (state == Shared::Views::IntroView::kFailed)
		error("FM Towns intro could not draw an image or text");
	if (state == Shared::Views::IntroView::kCompleted ||
			state == Shared::Views::IntroView::kInterrupted) {
		_menuVisible = true;
		drawMainMenu();
	}
}

void ViewTownsDemo::setMenuText(const Common::String &text) {
	GUI::MessageDialog dialog(text);
	dialog.runModal();
	drawMainMenu();
}

void ViewTownsDemo::draw() {
	if (_menuVisible)
		drawMainMenu();
}

bool ViewTownsDemo::ShowMsg(CShowMsg *msg) {
	Shared::Gfx::VisualItem::ShowMsg(msg);
	if (_intro && _intro->state() == Shared::Views::IntroView::kCompleted) {
		_menuVisible = true;
		drawMainMenu();
	}
	return true;
}

bool ViewTownsDemo::FrameMsg(CFrameMsg *msg) {
	if (!_intro || _intro->state() != Shared::Views::IntroView::kRunning)
		return true;
	_intro->update(getGame()->getMillis());
	finishIntro();
	return true;
}

bool ViewTownsDemo::KeypressMsg(CKeypressMsg *msg) {
	if (_intro && _intro->state() == Shared::Views::IntroView::kRunning) {
		Common::Event event;
		event.type = Common::EVENT_KEYDOWN;
		if (_intro->handleEvent(event))
			finishIntro();
		return true;
	}

	if (!_menuVisible)
		return true;

	switch (msg->_keyState.keycode) {
	case Common::KEYCODE_a:
		setMenuText(_("FM Towns character generation is not implemented yet."));
		break;
	case Common::KEYCODE_b:
		setMenuText(_("FM Towns save loading is not implemented yet."));
		break;
	case Common::KEYCODE_r:
		startIntro(*static_cast<Ultima1Game *>(getGame())->_res);
		break;
	case Common::KEYCODE_ESCAPE:
	case Common::KEYCODE_q:
		g_vm->quitGame();
		break;
	default:
		break;
	}
	return true;
}

} // End of namespace U1Gfx
} // End of namespace Ultima1
} // End of namespace Ultima
