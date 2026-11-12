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

#ifndef WIZARDRY_RESOURCES_TITLE_IMAGE_H
#define WIZARDRY_RESOURCES_TITLE_IMAGE_H

#include "wizardry/profile.h"
#include "wizardry/resources/cell_image.h"

namespace Wizardry {

/**
 * Loads the title picture of a graphics set. It is not stored in cells but
 * as plain screen rows of 80 bytes; its size is a whole number of cells all
 * the same: 35 x 7 on the IBM-PC, 40 x 7 on the Japanese machines.
 *
 * The IBM-PC driver shows it at cell (2, 2). Where the Japanese drivers put
 * it has not been read from their code yet.
 *
 * @returns false if the set has no title
 */
extern bool loadTitleImage(const Profile &profile, const GfxSet &gfx, CellImage &image);

} // End of namespace Wizardry

#endif
