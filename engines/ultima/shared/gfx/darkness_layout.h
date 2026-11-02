/* ScummVM - Graphic Adventure Engine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef ULTIMA_SHARED_GFX_DARKNESS_LAYOUT_H
#define ULTIMA_SHARED_GFX_DARKNESS_LAYOUT_H

#include "ultima/shared/gfx/text_box.h"

namespace Ultima {
namespace Shared {
namespace Gfx {

/**
 * DARKNESS principal layout box indices and their GFX_initTextBox parameters.
 *
 * Box  x    y    w    h    maxCol maxRow  fg  border  bg
 *  2   16  432  608   20    77     0      15    15    14   (info line)
 *  3   58  401  256   20    30     0      15    15    10   (location/name)
 *  7  370  303  263  117    31     6      15    15     0   (message)
 * 10  370   47  263   52    31     2      15    15     0   (name/resources)
 * 11  370  111  263  182    31    10      15    15     0   (attributes)
 * 14   24   64  216  326    26    19      15    11    14   (main-screen)
 */

/**
 * SCR_setupInfoLineBox: initialize and draw box 2 (information line).
 * GFX_initTextBox(2, 16, 432, 608, 20, 77, 0, 15, 15, 14) + applyFont + drawBox.
 */
void setupInfoLineBox(TextBox &box, TownsScreen &screen, SjisTextRasterizer &rasterizer);

/**
 * SCR_mainPlayField: initialize and draw box 3 (location/name below viewport).
 * GFX_initTextBox(3, 58, 401, 256, 20, 30, 0, 15, 15, 10) + drawBox.
 */
void setupLocationBox(TextBox &box, TownsScreen &screen, SjisTextRasterizer &rasterizer);

/**
 * SCR_mainStatusColumn: initialize and draw boxes 10, 11, 7
 * (name/resources, attributes/equipment, message).
 */
void setupStatusColumn(TextBox &nameBox, TextBox &attrBox, TextBox &msgBox,
		TownsScreen &screen, SjisTextRasterizer &rasterizer);

/**
 * SCR_setupMainScreenBox: initialize box 14 (main-screen / roster dialog).
 * GFX_initTextBox(14, 24, 64, 216, 326, 26, 19, 15, 11, 14). No drawBox call.
 */
void setupMainScreenBox(TextBox &box, TownsScreen &screen, SjisTextRasterizer &rasterizer);

} // namespace Gfx
} // namespace Shared
} // namespace Ultima

#endif
