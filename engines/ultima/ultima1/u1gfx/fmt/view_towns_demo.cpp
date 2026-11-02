/* ScummVM - Graphic Adventure Engine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "ultima/ultima1/u1gfx/fmt/view_towns_demo.h"
#include "ultima/ultima1/u1gfx/fmt/fmtowns_resources.h"
#include "ultima/ultima1/core/resources.h"
#include "common/config-manager.h"
#include "common/language.h"
#include "common/str-enc.h"
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

const uint32 kMillisPerDemoDelayUnit = 100;

// UT1DEMO box 0: GFX_initTextBox(0, 29, 362, 592, 52, 71, 2, 15, 15, 10)
// pixelX = box.x + 4 + col*8 => first glyph x = 29+4 = 33
// baseline = box.y + 17 + row*16 => first glyph baseline = 362+17 = 379
const Shared::Gfx::TextBoxConfig kBox0Config = [] {
	Shared::Gfx::TextBoxConfig c;
	c.x = 29;
	c.y = 362;
	c.width = 592;
	c.height = 52;
	c.requestedColumns = 71;
	c.requestedRows = 2;
	c.fontColor = 15;
	c.borderColor = 15;
	c.backgroundColor = 10;
	c.textBackgroundMode = -1;
	return c;
}();

} // End of anonymous namespace

class DemoSjisRasterizer : public Shared::Gfx::SjisTextRasterizer {
public:
#ifdef USE_SJIS
	explicit DemoSjisRasterizer(Graphics::FontSJIS *font) : _font(font) {}
#else
	explicit DemoSjisRasterizer(void *) {}
#endif

	void drawSjisBytes(Shared::Gfx::TownsScreen &screen, int16 x, int16 baselineY,
			const byte *sjis, uint16 byteLength) override {
		Graphics::ManagedSurface &surf = screen.getWriteSurface();
#ifdef USE_SJIS
		if (_font) {
			const byte *p = sjis;
			const byte *end = sjis + byteLength;
			int cx = x;
			while (p < end) {
				uint16 ch;
				if ((*p >= 0x81 && *p <= 0x9F) || (*p >= 0xE0 && *p <= 0xEF)) {
					if (p + 1 >= end) break;
					// FontSJIS expects lead byte in low byte, trail byte in high byte
					ch = p[0] | (p[1] << 8);
					p += 2;
				} else {
					ch = *p++;
				}
				const int glyphTop = baselineY - (int)_font->getFontHeight() + 1;
				_font->drawChar(surf.getBasePtr(cx, glyphTop), ch,
						surf.pitch, surf.format.bytesPerPixel, 15, 0, -1, -1);
				cx += _font->getCharWidth(ch);
			}
			return;
		}
#endif
		// Fallback: treat bytes as ASCII via the game font
		Shared::Point pos(x, baselineY - 7);
		Common::String str(reinterpret_cast<const char *>(sjis), byteLength);
		g_vm->_game->getFont()->writeString(surf, str, pos, 15, 0);
	}

private:
#ifdef USE_SJIS
	Graphics::FontSJIS *_font;
#endif
};

BEGIN_MESSAGE_MAP(ViewTownsDemo, Shared::Gfx::VisualItem)
	ON_MESSAGE(ShowMsg)
	ON_MESSAGE(KeypressMsg)
	ON_MESSAGE(FrameMsg)
END_MESSAGE_MAP()

ViewTownsDemo::ViewTownsDemo(Shared::TreeItem *parent) :
		Shared::Gfx::VisualItem("TownsIntro", Rect(0, 0, 640, 480), parent),
		_intro(nullptr), _menuVisible(false) {
#ifdef USE_SJIS
	_sjisFont = Graphics::FontSJIS::createFont(Common::kPlatformFMTowns);
	if (!_sjisFont)
		warning("FM Towns SJIS font not available (FMT_FNT.ROM or SJIS.FNT missing)");
	_rasterizer = new DemoSjisRasterizer(_sjisFont);
#else
	_rasterizer = new DemoSjisRasterizer(nullptr);
#endif

	g_vm->_townsScreen->setWritePage(0);
	_box0 = new Shared::Gfx::TextBox(*g_vm->_townsScreen, *_rasterizer);
	if (!_box0->initialize(kBox0Config))
		error("ViewTownsDemo: Failed to initialize text box 0");

	_graphics = new Shared::Views::IntroTownsGraphics(*g_vm->_townsScreen,
			Shared::Game::getFMTownsPalette());
	_introPort = new Shared::Views::IntroTownsPort(*_graphics, *_box0, *this);

	if (!loadImage("HEADTTL.TIF", _header) || _header.w != 640 ||
			_header.h != 24 || !_header.format.isCLUT8())
		error("Could not load FM Towns intro header");

	const char *const names[] = {
		"UT1_1.TIF", "UT1_2.TIF", "UT1_3.TIF", "UT1_4.TIF"
	};
	for (uint i = 0; i < ARRAYSIZE(names); ++i) {
		if (!loadImage(names[i], _slides[i]) || _slides[i].format.bytesPerPixel != 2)
			error("Could not load FM Towns intro slide '%s'", names[i]);
		Shared::Views::IntroSlideRaster raster;
		raster.pixels = static_cast<const byte *>(_slides[i].getPixels());
		raster.dataBytes = _slides[i].pitch * _slides[i].h;
		raster.width = _slides[i].w;
		raster.height = _slides[i].h;
		raster.bitsPerPixel = 16;
		_graphics->setSlideRaster(i, raster);
	}
}

ViewTownsDemo::~ViewTownsDemo() {
	delete _intro;
	delete _introPort;
	delete _graphics;
	delete _box0;
	delete _rasterizer;
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

static Common::String toSjis(const char *utf8) {
	if (!utf8 || !*utf8)
		return Common::String();
	return Common::convertFromU32String(
		Common::convertToU32String(utf8, Common::kUtf8),
		Common::kWindows932);
}

void ViewTownsDemo::setDemoStrings(const GameResources &resources) {
	const bool isJapanese =
		Common::parseLanguage(ConfMan.get("language")) == Common::JA_JPN;

	for (uint i = 0; i < 13; ++i) {
		const char *ja = isJapanese ? _fmtRes.INTRO_SLIDE_JA[i].c_str() : nullptr;
		_demoText[i] = (ja && *ja) ? toSjis(ja) : resources.TITLE_MESSAGES[i];
	}
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
	_headerTitle = FMTownsResources::str(
			_fmtRes.INTRO_HEADER_EN, _fmtRes.INTRO_HEADER_JA);
	g_vm->_townsScreen->setWritePage(0);
	g_vm->_townsScreen->drawMainScreenFrame(0, 24, 640, 400, 0);
	debug("startIntro: calling drawHeader title='%s'", _headerTitle.c_str());
	g_vm->_townsScreen->drawHeader(_header, _headerTitle, _sjisFont);
	_intro->start(getGame()->getMillis());
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
	g_vm->_townsScreen->getWriteSurface().copyRectToSurface(slide, x, y,
			Common::Rect(slide.w, slide.h));
	g_vm->_townsScreen->invalidateRect(x, y, slide.w, slide.h);

	g_vm->_townsScreen->setWritePage(0);
	g_vm->_townsScreen->drawMainScreenFrame(0, 24, 640, 400, 0);
	debug("drawSlide %u: calling drawHeader", slideIndex);
	g_vm->_townsScreen->drawHeader(_header, _headerTitle, _sjisFont);
	_box0->prepare();
	return true;
}

void ViewTownsDemo::drawDemoString(const Common::String &text) {
	g_vm->_townsScreen->setWritePage(0);
	_box0->printSjis(text.c_str());
}

void ViewTownsDemo::drawMainMenu() {
	g_vm->_townsScreen->setWritePage(0);
	_box0->prepare();
	_box0->setCursor(0, 0);
	_box0->printSjis(_demoText[13].c_str());
	_box0->advanceLine();
	_box0->printSjis(_demoText[14].c_str());
	_box0->advanceLine();
	_box0->printSjis(_demoText[15].c_str());
	_box0->advanceLine();
	_box0->printSjis(_demoText[16].c_str());
	_box0->advanceLine();
	_box0->printSjis(_demoText[17].substr(1).c_str());
	_box0->advanceLine();
	_box0->printSjis(_demoText[18].substr(1).c_str());
	_box0->advanceLine();
	_box0->printSjis(_demoText[19].substr(1).c_str());
	_box0->advanceLine();
	_box0->printSjis(_("r) Replay demo").encode().c_str());
	_box0->advanceLine();
	_box0->printSjis(_("q) Quit").encode().c_str());
	_introPort->present();
}

void ViewTownsDemo::clearPendingKeyPresses() {
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
