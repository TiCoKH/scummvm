/* ScummVM - Graphic Adventure Engine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "ultima/shared/gfx/darkness_layout.h"

namespace Ultima {
namespace Shared {
namespace Gfx {

static bool initAndDraw(TextBox &box, TownsScreen &screen, SjisTextRasterizer &rasterizer,
		int32 x, int32 y, int32 w, int32 h,
		int32 maxCol, int32 maxRow,
		uint16 fontColor, uint16 borderColor, uint16 bgColor,
		bool drawFrame_) {
	TextBoxConfig cfg;
	cfg.x = x;
	cfg.y = y;
	cfg.width = w;
	cfg.height = h;
	cfg.requestedColumns = maxCol;
	cfg.requestedRows = maxRow;
	cfg.fontColor = fontColor;
	cfg.borderColor = borderColor;
	cfg.backgroundColor = bgColor;
	cfg.textBackgroundMode = -1;

	// Re-use existing TextBox object: reconstruct in-place via initialize.
	if (!box.initialize(cfg))
		return false;
	if (drawFrame_) {
		screen.setWritePage(0);
		box.applyFont();
		box.drawFrame();
	}
	return true;
}

// SCR_setupInfoLineBox
// GFX_initTextBox(2, 16, 432, 608, 20, 77, 0, 15, 15, 14) + applyFont + drawBox
void setupInfoLineBox(TextBox &box, TownsScreen &screen, SjisTextRasterizer &rasterizer) {
	initAndDraw(box, screen, rasterizer, 16, 432, 608, 20, 77, 0, 15, 15, 14, true);
}

// SCR_mainPlayField
// GFX_initTextBox(3, 58, 401, 256, 20, 30, 0, 15, 15, 10) + drawBox
void setupLocationBox(TextBox &box, TownsScreen &screen, SjisTextRasterizer &rasterizer) {
	initAndDraw(box, screen, rasterizer, 58, 401, 256, 20, 30, 0, 15, 15, 10, true);
}

// SCR_mainStatusColumn
// Box 10: GFX_initTextBox(10, 370, 47, 263, 52, 31, 2, 15, 15, 0) + applyFont + drawBox
// Box 11: GFX_initTextBox(11, 370, 111, 263, 182, 31, 10, 15, 15, 0) + applyFont + drawBox
// Box  7: GFX_initTextBox(7, 370, 303, 263, 117, 31, 6, 15, 15, 0) + applyFont + drawBox
void setupStatusColumn(TextBox &nameBox, TextBox &attrBox, TextBox &msgBox,
		TownsScreen &screen, SjisTextRasterizer &rasterizer) {
	initAndDraw(nameBox, screen, rasterizer, 370,  47, 263,  52, 31,  2, 15, 15, 0, true);
	initAndDraw(attrBox, screen, rasterizer, 370, 111, 263, 182, 31, 10, 15, 15, 0, true);
	initAndDraw(msgBox,  screen, rasterizer, 370, 303, 263, 117, 31,  6, 15, 15, 0, true);
}

// SCR_setupMainScreenBox
// GFX_initTextBox(14, 24, 64, 216, 326, 26, 19, 15, 11, 14) — no drawBox
void setupMainScreenBox(TextBox &box, TownsScreen &screen, SjisTextRasterizer &rasterizer) {
	initAndDraw(box, screen, rasterizer, 24, 64, 216, 326, 26, 19, 15, 11, 14, false);
}

} // namespace Gfx
} // namespace Shared
} // namespace Ultima
