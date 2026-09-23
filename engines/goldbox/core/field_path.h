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

#ifndef GOLDBOX_CORE_FIELD_PATH_H
#define GOLDBOX_CORE_FIELD_PATH_H

#include "common/scummsys.h"
#include "goldbox/core/direction.h"

namespace Goldbox {

/**
 * Bresenham line-walk state used by combat LOS and spell path tracing.
 * Mirrors gbFieldPath from the original engine.
 *
 * Used by:
 *   COMBAT_lineOfSightCheck  -> CombatContext::lineOfSightCheck
 *   SPELL_TraceSpellPath     -> spell system path tracing
 */
struct FieldPath {
    // Input
    int16 startCol, startRow;
    int16 endCol,   endRow;

    // Derived line geometry
    int16 deltaCol, deltaRow;
    int8  stepCol,  stepRow;

    // Traversal state
    int16 col,      row;
    int16 moveCost;

    // Bresenham state
    int16 error;
    int16 errorStep;
    int16 minorErrorStep;

    // Direction of the last step taken (written by stepBresenham)
    Direction stepDirection;
};

/**
 * Initialise a FieldPath for Bresenham line-walking.
 * Mirrors COMBAT_initBresenham.
 */
void initBresenham(FieldPath &path);

/**
 * Advance one step along a FieldPath.
 * Returns true while still moving toward the end, false when the end
 * tile is reached (no step taken).
 * Mirrors COMBAT_stepBresenham.
 */
bool stepBresenham(FieldPath &path);

} // namespace Goldbox

#endif // GOLDBOX_CORE_FIELD_PATH_H
