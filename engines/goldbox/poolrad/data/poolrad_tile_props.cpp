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

#include "goldbox/poolrad/data/poolrad_tile_props.h"

namespace Goldbox {
namespace Poolrad {

using Combat::TileProp;

// struct = {passable, padding, tileSize, blockID}
// Table is indexed by raw tile value (1-based from tilemap = index+1 here)
// Entry [0] corresponds to raw tile 1 in the tilemap buffer.
const TileProp PoolradTilePropertyProvider::kTable[] = {
/* [00] */ { -1, 0x00, 0x02, 0x00 }, // impassable, full tile
/* [01] */ { -1, 0x00, 0x02, 0x01 }, // impassable, full tile
/* [02] */ { -1, 0x00, 0x02, 0x02 }, // impassable, full tile
/* [03] */ { -1, 0x00, 0x02, 0x03 }, // impassable, full tile
/* [04] */ {  1, 0x00, 0x00, 0x04 }, // walkable, half-size
/* [05] */ { -1, 0x00, 0x02, 0x05 }, // impassable, full tile
/* [06] */ { -1, 0x00, 0x02, 0x06 }, // impassable, full tile
/* [07] */ { -1, 0x00, 0x02, 0x07 }, // impassable, full tile
/* [08] */ {  1, 0x00, 0x00, 0x08 }, // walkable, half-size
/* [09] */ { -1, 0x00, 0x02, 0x09 }, // impassable, full tile
/* [10] */ {  1, 0x00, 0x00, 0x0A }, // walkable, half-size
/* [11] */ { -1, 0x00, 0x02, 0x0B }, // impassable, full tile
/* [12] */ {  1, 0x00, 0x00, 0x0C }, // walkable, half-size
/* [13] */ { -1, 0x00, 0x02, 0x0D }, // impassable, full tile
/* [14] */ {  1, 0x00, 0x00, 0x0E }, // walkable, half-size
/* [15] */ { -1, 0x00, 0x02, 0x0F }, // impassable, full tile
/* [16] */ {  1, 0x00, 0x00, 0x10 }, // walkable, half-size
/* [17] */ { -1, 0x00, 0x02, 0x11 }, // impassable, full tile
/* [18] */ { -1, 0x00, 0x02, 0x12 }, // impassable, full tile
/* [19] */ { -1, 0x00, 0x02, 0x13 }, // impassable, full tile
/* [20] */ { -1, 0x00, 0x02, 0x14 }, // impassable, full tile
/* [21] */ { -1, 0x00, 0x02, 0x15 }, // impassable, full tile
/* [22] */ {  1, 0x00, 0x00, 0x16 }, // walkable, half-size
/* [23] */ {  1, 0x00, 0x00, 0x17 }, // walkable, half-size (TILE_OPEN_PLAIN)
/* [24] */ { -1, 0x00, 0x02, 0x18 }, // impassable, full tile
/* [25] */ {  1, 0x00, 0x00, 0x22 }, // walkable, half-size
/* [26] */ {  1, 0x00, 0x00, 0x23 }, // walkable, half-size (TILE_LIGHT_VEGETATION)
/* [27] */ {  1, 0x00, 0x00, 0x24 }, // walkable, half-size
/* [28] */ {  1, 0x00, 0x00, 0x25 }, // walkable, half-size
/* [29] */ {  1, 0x00, 0x00, 0x26 }, // walkable, half-size
/* [30] */ {  1, 0x00, 0x00, 0x27 }, // walkable, half-size
/* [31] */ { -1, 0x00, 0x02, 0x00 }, // impassable, full tile
/* --- second tileset bank (indices 32..64) --- */
/* [32] */ { -1, 0x00, 0x02, 0x01 }, // impassable, full tile
/* [33] */ { -1, 0x00, 0x02, 0x02 }, // impassable, full tile
/* [34] */ { -1, 0x00, 0x02, 0x03 }, // impassable, full tile
/* [35] */ {  1, 0x00, 0x00, 0x04 }, // walkable, half-size
/* [36] */ {  1, 0x00, 0x00, 0x05 }, // walkable, half-size
/* [37] */ {  1, 0x00, 0x00, 0x06 }, // walkable, half-size
/* [38] */ {  1, 0x00, 0x00, 0x07 }, // walkable, half-size
/* [39] */ { -1, 0x00, 0x00, 0x08 }, // impassable, half-size
/* [40] */ { -1, 0x00, 0x00, 0x09 }, // impassable, half-size
/* [41] */ {  1, 0x00, 0x00, 0x0A }, // walkable, half-size
/* [42] */ {  1, 0x00, 0x00, 0x0B }, // walkable, half-size
/* [43] */ {  1, 0x00, 0x00, 0x0C }, // walkable, half-size
/* [44] */ {  1, 0x00, 0x00, 0x0D }, // walkable, half-size
/* [45] */ {  1, 0x00, 0x00, 0x0E }, // walkable, half-size
/* [46] */ {  1, 0x00, 0x00, 0x0F }, // walkable, half-size
/* [47] */ {  1, 0x00, 0x00, 0x10 }, // walkable, half-size
/* [48] */ {  1, 0x00, 0x00, 0x11 }, // walkable, half-size
/* [49] */ { -1, 0x00, 0x00, 0x12 }, // impassable, half-size
/* [50] */ { -1, 0x00, 0x00, 0x13 }, // impassable, half-size
/* [51] */ {  1, 0x00, 0x00, 0x14 }, // walkable, half-size
/* [52] */ {  1, 0x00, 0x00, 0x15 }, // walkable, half-size
/* [53] */ {  1, 0x00, 0x00, 0x16 }, // walkable, half-size
/* [54] */ { -1, 0x00, 0x00, 0x17 }, // impassable, half-size
/* [55] */ { -1, 0x00, 0x00, 0x18 }, // impassable, half-size
/* [56] */ {  1, 0x00, 0x00, 0x19 }, // walkable, half-size
/* [57] */ {  1, 0x00, 0x00, 0x1A }, // walkable, half-size
/* [58] */ {  1, 0x00, 0x00, 0x1B }, // walkable, half-size
/* [59] */ { -1, 0x00, 0x00, 0x1C }, // impassable, half-size
/* [60] */ { -1, 0x00, 0x02, 0x1D }, // impassable, full tile (TILE_SMALL_ROCKS)
/* [61] */ { -1, 0x00, 0x02, 0x1E }, // impassable, full tile (TILE_LARGE_BOULDERS)
/* [62] */ { -1, 0x00, 0x02, 0x1F }, // impassable, full tile
/* [63] */ {  1, 0x00, 0x00, 0x20 }, // walkable, half-size
/* [64] */ { -1, 0x00, 0x02, 0x21 }, // impassable, full tile
};

const TileProp *PoolradTilePropertyProvider::getTileProp(int index) const {
    if (index < 0 || index >= kTableCount)
        return nullptr;
    return &kTable[index];
}

const PoolradTilePropertyProvider &PoolradTilePropertyProvider::instance() {
    static PoolradTilePropertyProvider inst;
    return inst;
}

} // namespace Poolrad
} // namespace Goldbox
