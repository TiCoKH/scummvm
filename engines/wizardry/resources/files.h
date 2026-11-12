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

#ifndef WIZARDRY_RESOURCES_FILES_H
#define WIZARDRY_RESOURCES_FILES_H

#include "common/array.h"
#include "common/scummsys.h"

namespace Wizardry {

constexpr uint BLOCK_SIZE = 512;

/**
 * Reads a whole file of the game into memory. The resource classes get
 * their data only through this function, so they can be run outside the
 * engine.
 * @returns false if there is no such file
 */
extern bool readFile(const char *name, Common::Array<byte> &data);

} // End of namespace Wizardry

#endif
