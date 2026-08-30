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

#include "goldbox/runtime/runtime_geo.h"
#include "common/textconsole.h"
#include <string.h>

namespace Goldbox {

RuntimeGeoBlock::RuntimeGeoBlock() : _loaded(false), _blockId(0xFF), _mapId(0) {
    memset(_buf, 0, sizeof(_buf));
}

void RuntimeGeoBlock::loadFromGeoBlock(const Data::DaxBlockGeo &geo) {
    clear();

    const Common::Span<const uint8> raw = geo.raw();
    if (raw.size() < BUFFER_SIZE)
        return;

    memcpy(_buf, raw.data(), BUFFER_SIZE);
    _blockId = geo.blockId;
    _loaded = true;
}

void RuntimeGeoBlock::loadFromBuffer(const uint8 *buf, uint32 size) {
    clear();

    if (!buf || size < BUFFER_SIZE)
        return;

    memcpy(_buf, buf, BUFFER_SIZE);
    _loaded = true;
}

void RuntimeGeoBlock::clear() {
    memset(_buf, 0, sizeof(_buf));
    _loaded = false;
    _blockId = 0xFF;
}

bool RuntimeGeoBlock::isInBounds(MapPos pos) const {
    return (pos.x >= 0 && pos.x < GRID_SIZE && pos.y >= 0 && pos.y < GRID_SIZE);
}

void RuntimeGeoBlock::wrapCoords(MapPos &pos) const {
    if (pos.x > 15) pos.x = 0;
    if (pos.x < 0)  pos.x = 15;
    if (pos.y > 15) pos.y = 0;
    if (pos.y < 0)  pos.y = 15;
}

uint8 RuntimeGeoBlock::getMapNibble(MapPos pos, uint8 wireDir) const {
    if (!_loaded)
        return 0;

    if (!isInBounds(pos) && (_mapId == 0 || _mapId == 10))
        return 0;

    wrapCoords(pos);
    const int idx = cellIndex(pos);

    switch (wireDir) {
    case 0: // North
        return (_buf[idx] >> 4) & 0x0F;
    case 2: // East
        return _buf[idx] & 0x0F;
    case 4: // South
        return (_buf[PLANE_SIZE + idx] >> 4) & 0x0F;
    case 6: // West
        return _buf[PLANE_SIZE + idx] & 0x0F;
    default:
        return 0x0F;
    }
}

uint8 RuntimeGeoBlock::getGeoData(MapPos pos) const {
    if (!_loaded)
        return 0;

    if (!isInBounds(pos) && (_mapId == 0 || _mapId == 10))
        return 0;

    wrapCoords(pos);
    return _buf[PLANE_SIZE * 2 + cellIndex(pos)];
}

uint8 RuntimeGeoBlock::getWallFlag(MapPos pos, uint8 wireDir) const {
    if (!_loaded)
        return 0;

    if (!isInBounds(pos) && (_mapId == 0 || _mapId == 10))
        return 0;

    wrapCoords(pos);

    // If the wall nibble is 0 (no wall), return 1 (passable) per original.
    const uint8 nibble = getMapNibble(pos, wireDir);
    if (nibble == 0)
        return 1;

    const int idx = cellIndex(pos);
    const uint8 doorByte = _buf[PLANE_SIZE * 3 + idx];

    switch (wireDir) {
    case 0: // North: bits [1:0]
        return doorByte & 0x03;
    case 2: // East: bits [3:2]
        return (doorByte >> 2) & 0x03;
    case 4: // South: bits [5:4]
        return (doorByte >> 4) & 0x03;
    case 6: // West: bits [7:6]
        return (doorByte >> 6) & 0x03;
    default:
        return 1;
    }
}

void RuntimeGeoBlock::clearFlag(MapPos pos, uint8 wireDir) {
    if (!_loaded)
        return;

    const int idx = cellIndex(pos);
    uint8 &doorByte = _buf[PLANE_SIZE * 3 + idx];

    switch (wireDir) {
    case 0: // North: clear bits [1:0]
        doorByte &= 0xFC;
        break;
    case 2: // East: clear bits [3:2]
        doorByte &= 0xF3;
        break;
    case 4: // South: clear bits [5:4]
        doorByte &= 0xCF;
        break;
    case 6: // West: clear bits [7:6]
        doorByte &= 0x3F;
        break;
    default:
        break;
    }
}

void RuntimeGeoBlock::setTileDirectionState(MapPos pos, uint8 wireDir) {
    if (!_loaded)
        return;

    if (!isInBounds(pos))
        return;

    const int idx = cellIndex(pos);
    uint8 &doorByte = _buf[PLANE_SIZE * 3 + idx];

    switch (wireDir) {
    case 0: // North: set bits [1:0] to 01
        doorByte = (doorByte & 0xFC) | 0x01;
        break;
    case 2: // East: set bits [3:2] to 01
        doorByte = (doorByte & 0xF3) | 0x04;
        break;
    case 4: // South: set bits [5:4] to 01
        doorByte = (doorByte & 0xCF) | 0x10;
        break;
    case 6: // West: set bits [7:6] to 01
        doorByte = (doorByte & 0x3F) | 0x40;
        break;
    default:
        break;
    }
}

void RuntimeGeoBlock::writePlane(int plane, int index, uint8 value) {
    if (plane < 0 || plane > 3 || index < 0 || index >= PLANE_SIZE)
        return;
    _buf[plane * PLANE_SIZE + index] = value;
}

uint8 RuntimeGeoBlock::readPlane(int plane, int index) const {
    if (plane < 0 || plane > 3 || index < 0 || index >= PLANE_SIZE)
        return 0;
    return _buf[plane * PLANE_SIZE + index];
}

} // namespace Goldbox
