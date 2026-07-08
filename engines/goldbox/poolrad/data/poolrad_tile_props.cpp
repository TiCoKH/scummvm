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

// Data from m68k binary at 0x284858, struct = {passable, padding, blockType, gfxID}
// Table is indexed by raw tile value (1-based from tilemap = index+1 here)
// Entry [0] corresponds to raw tile 1 in the tilemap buffer.
const TileProp PoolradTilePropertyProvider::kTable[] = {
/*       pass  pad  blk   gfx                                        */
/* [00] */ { -1, 0x00, 0x02, 0x00 }, // impassable, hard block
/* [01] */ { -1, 0x00, 0x02, 0x01 }, // impassable, hard block
/* [02] */ { -1, 0x00, 0x02, 0x02 }, // impassable, hard block
/* [03] */ { -1, 0x00, 0x02, 0x03 }, // impassable, hard block
/* [04] */ {  1, 0x00, 0x00, 0x04 }, // walkable
/* [05] */ { -1, 0x00, 0x02, 0x05 }, // impassable, hard block
/* [06] */ { -1, 0x00, 0x02, 0x06 }, // impassable, hard block
/* [07] */ { -1, 0x00, 0x02, 0x07 }, // impassable, hard block
/* [08] */ {  1, 0x00, 0x00, 0x08 }, // walkable
/* [09] */ { -1, 0x00, 0x02, 0x09 }, // impassable, hard block
/* [10] */ {  1, 0x00, 0x00, 0x0A }, // walkable
/* [11] */ { -1, 0x00, 0x02, 0x0B }, // impassable, hard block
/* [12] */ {  1, 0x00, 0x00, 0x0C }, // walkable
/* [13] */ { -1, 0x00, 0x02, 0x0D }, // impassable, hard block
/* [14] */ {  1, 0x00, 0x00, 0x0E }, // walkable
/* [15] */ { -1, 0x00, 0x02, 0x0F }, // impassable, hard block
/* [16] */ {  1, 0x00, 0x00, 0x10 }, // walkable
/* [17] */ { -1, 0x00, 0x02, 0x11 }, // impassable, hard block
/* [18] */ { -1, 0x00, 0x02, 0x12 }, // impassable, hard block
/* [19] */ { -1, 0x00, 0x02, 0x13 }, // impassable, hard block
/* [20] */ { -1, 0x00, 0x02, 0x14 }, // impassable, hard block
/* [21] */ { -1, 0x00, 0x02, 0x15 }, // impassable, hard block
/* [22] */ {  1, 0x00, 0x00, 0x16 }, // walkable
/* [23] */ {  1, 0x00, 0x00, 0x17 }, // walkable  (TILE_OPEN_PLAIN)
/* [24] */ { -1, 0x00, 0x02, 0x18 }, // impassable, hard block
/* [25] */ {  1, 0x00, 0x00, 0x22 }, // walkable
/* [26] */ {  1, 0x00, 0x00, 0x23 }, // walkable  (TILE_LIGHT_VEGETATION)
/* [27] */ {  1, 0x00, 0x00, 0x24 }, // walkable
/* [28] */ {  1, 0x00, 0x00, 0x25 }, // walkable
/* [29] */ {  1, 0x00, 0x00, 0x26 }, // walkable
/* [30] */ {  1, 0x00, 0x00, 0x27 }, // walkable
/* [31] */ { -1, 0x00, 0x02, 0x00 }, // impassable, hard block
/* --- second tileset bank (indices 32..64) --- */
/* [32] */ { -1, 0x00, 0x02, 0x01 }, // impassable, hard block
/* [33] */ { -1, 0x00, 0x02, 0x02 }, // impassable, hard block
/* [34] */ { -1, 0x00, 0x02, 0x03 }, // impassable, hard block
/* [35] */ {  1, 0x00, 0x00, 0x04 }, // walkable
/* [36] */ {  1, 0x00, 0x00, 0x05 }, // walkable
/* [37] */ {  1, 0x00, 0x00, 0x06 }, // walkable
/* [38] */ {  1, 0x00, 0x00, 0x07 }, // walkable
/* [39] */ { -1, 0x00, 0x00, 0x08 }, // impassable, no hard block
/* [40] */ { -1, 0x00, 0x00, 0x09 }, // impassable, no hard block
/* [41] */ {  1, 0x00, 0x00, 0x0A }, // walkable
/* [42] */ {  1, 0x00, 0x00, 0x0B }, // walkable
/* [43] */ {  1, 0x00, 0x00, 0x0C }, // walkable
/* [44] */ {  1, 0x00, 0x00, 0x0D }, // walkable
/* [45] */ {  1, 0x00, 0x00, 0x0E }, // walkable
/* [46] */ {  1, 0x00, 0x00, 0x0F }, // walkable
/* [47] */ {  1, 0x00, 0x00, 0x10 }, // walkable
/* [48] */ {  1, 0x00, 0x00, 0x11 }, // walkable
/* [49] */ { -1, 0x00, 0x00, 0x12 }, // impassable, no hard block
/* [50] */ { -1, 0x00, 0x00, 0x13 }, // impassable, no hard block
/* [51] */ {  1, 0x00, 0x00, 0x14 }, // walkable
/* [52] */ {  1, 0x00, 0x00, 0x15 }, // walkable
/* [53] */ {  1, 0x00, 0x00, 0x16 }, // walkable
/* [54] */ { -1, 0x00, 0x00, 0x17 }, // impassable, no hard block
/* [55] */ { -1, 0x00, 0x00, 0x18 }, // impassable, no hard block
/* [56] */ {  1, 0x00, 0x00, 0x19 }, // walkable
/* [57] */ {  1, 0x00, 0x00, 0x1A }, // walkable
/* [58] */ {  1, 0x00, 0x00, 0x1B }, // walkable
/* [59] */ { -1, 0x00, 0x00, 0x1C }, // impassable, no hard block
/* [60] */ { -1, 0x00, 0x02, 0x1D }, // impassable, hard block (TILE_SMALL_ROCKS)
/* [61] */ { -1, 0x00, 0x02, 0x1E }, // impassable, hard block (TILE_LARGE_BOULDERS)
/* [62] */ { -1, 0x00, 0x02, 0x1F }, // impassable, hard block
/* [63] */ {  1, 0x00, 0x00, 0x20 }, // walkable
/* [64] */ { -1, 0x00, 0x02, 0x21 }, // impassable, hard block
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
