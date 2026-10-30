/* ScummVM - Graphic Adventure Engine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef ULTIMA_SHARED_VIEWS_INTRO_TOWNS_GRAPHICS_H
#define ULTIMA_SHARED_VIEWS_INTRO_TOWNS_GRAPHICS_H

#include "ultima/shared/gfx/towns_screen.h"

namespace Ultima {
namespace Shared {
namespace Views {

struct IntroSlideRaster {
	const byte *pixels = nullptr;
	uint32 dataBytes = 0;
	int width = 0;
	int height = 0;
	uint8 bitsPerPixel = 0;
};

/**
 * Graphics-only bridge for the recovered intro wrappers.
 * Display modes follow the recovered GFX_setDisplayPage mapping.
 * Palette and raster buffers remain caller-owned for this object's lifetime.
 */
class IntroTownsGraphics {
public:
	IntroTownsGraphics(Gfx::TownsScreen &screen, const byte *defaultPaletteRGB48);

	void setSlideRaster(int index, const IntroSlideRaster &raster);
	/** Mode 0, write page 1, transfer at (31,16), write page 0, restore entries 1..15. */
	bool drawSlide(uint8 slideIndex);
	bool setDisplayPage(int originalMode);
	void present();

private:
	Gfx::TownsScreen &_screen;
	const byte *_defaultPaletteRGB48;
	IntroSlideRaster _slides[4];
};

} // End of namespace Views
} // End of namespace Shared
} // End of namespace Ultima

#endif
