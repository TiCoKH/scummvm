/* ScummVM - Graphic Adventure Engine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "ultima/shared/views/intro_towns_graphics.h"
#include "common/textconsole.h"

namespace Ultima {
namespace Shared {
namespace Views {

IntroTownsGraphics::IntroTownsGraphics(Gfx::TownsScreen &screen,
		const byte *defaultPaletteRGB48) :
		_screen(screen), _defaultPaletteRGB48(defaultPaletteRGB48) {
	if (!defaultPaletteRGB48)
		error("IntroTownsGraphics: Requires a default palette");
}

void IntroTownsGraphics::setSlideRaster(int index, const IntroSlideRaster &raster) {
	if (index < 0 || index >= int(ARRAYSIZE(_slides))) {
		warning("IntroTownsGraphics::setSlideRaster(): Invalid slide %d", index);
		return;
	}
	_slides[index] = raster;
}

bool IntroTownsGraphics::setDisplayPage(int originalMode) {
	return _screen.setLegacyDisplayPage(originalMode);
}

bool IntroTownsGraphics::drawSlide(uint8 slideIndex) {
	if (slideIndex >= ARRAYSIZE(_slides) || !_slides[slideIndex].pixels ||
			_screen.getLayerBpp(0) != 1) {
		warning("IntroTownsGraphics::drawSlide(): Invalid slide %u or non-indexed text page", slideIndex);
		return false;
	}
	const IntroSlideRaster &slide = _slides[slideIndex];
	setDisplayPage(0);
	_screen.setWritePage(1);
	const bool ok = _screen.putPixelBlock(slide.pixels, slide.dataBytes, 31, 16,
			slide.width, slide.height, slide.bitsPerPixel);
	_screen.setWritePage(0);
	_screen.applyDefaultPalette4bit(_defaultPaletteRGB48);
	return ok;
}

void IntroTownsGraphics::present() {
	_screen.update();
	g_system->updateScreen();
}

} // End of namespace Views
} // End of namespace Shared
} // End of namespace Ultima
