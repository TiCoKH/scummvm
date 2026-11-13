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

#ifndef WIZARDRY_GAME_RANDOM_H
#define WIZARDRY_GAME_RANDOM_H

#include "common/scummsys.h"

namespace Wizardry {

/**
 * Where the rules get their dice from, so that they can be run with the
 * engine's generator or with a fixed sequence
 */
class Dice {
public:
	virtual ~Dice() {}

	/**
	 * Returns a number from 0 to count - 1
	 */
	virtual uint roll(uint count) = 0;
};

} // End of namespace Wizardry

#endif
