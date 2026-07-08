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

#ifndef GOLDBOX_POOLRAD_DATA_POOLRAD_TILE_PROPS_H
#define GOLDBOX_POOLRAD_DATA_POOLRAD_TILE_PROPS_H

#include "goldbox/combat/tile_property_provider.h"

namespace Goldbox {
namespace Poolrad {

/**
 * Pool of Radiance tile property provider.
 *
 * Contains the 65-entry tile property table specific to Pool of Radiance.
 * Other Gold Box games will provide their own implementations.
 */
class PoolradTilePropertyProvider : public Combat::TilePropertyProvider {
public:
    static const int kTableCount = 65;

    int getTilePropCount() const override { return kTableCount; }
    const Combat::TileProp *getTileProp(int index) const override;

    /** Singleton accessor for convenience. */
    static const PoolradTilePropertyProvider &instance();

private:
    static const Combat::TileProp kTable[kTableCount];
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_DATA_POOLRAD_TILE_PROPS_H
