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
 */

#ifndef GOLDBOX_GFX_DAX_RENDERER_H
#define GOLDBOX_GFX_DAX_RENDERER_H

#include "common/scummsys.h"
#include "goldbox/gfx/pic.h"

namespace Goldbox {
namespace Gfx {

/**
 * DaxRenderer abstracts access to DAX containers and decoding into Pic.
 * The default implementation uses VmInterface to fetch blocks.
 */
class DaxRenderer {
public:
    DaxRenderer() {}

    /**
     * Decode and return a head Pic for the given sprite ID.
     * Caller owns the returned Pic.
     */
    virtual Pic *readHead(uint8 spriteId);

    /**
     * Decode and return a body Pic for the given sprite ID.
     * Caller owns the returned Pic.
     */
    virtual Pic *readBody(uint8 spriteId);
};

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_DAX_RENDERER_H
