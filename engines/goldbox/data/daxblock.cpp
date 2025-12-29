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

#include "common/util.h"
#include "common/memstream.h"
#include "goldbox/data/daxblock.h"

namespace {

const int kWalldefViewCols[] = { 1, 1, 1, 3, 2, 2, 7, 2, 2, 1 };
const int kWalldefViewRows[] = { 2, 4, 4, 4, 8, 8, 8, 11, 11, 2 };

} // namespace

namespace Goldbox {
namespace Data {

    DaxBlock* DaxBlock::createDaxBlock(ContentType contentType) {
        if (contentType == ContentType::TILE) {
            return new DaxBlock8x8D();
        } else if (contentType == ContentType::PIC || contentType == ContentType::CTILE) {
            // CTILE uses the same binary layout as PIC but allows callers to
            // opt into mask-aware handling.
            return new DaxBlockPic();
//       } else if (contentType == ContentType::SPRIT) {
//           return new DaxBlockPic();
        } else if (contentType == ContentType::WALLDEF) {
            return new DaxBlockWalldef();
        } else if (contentType == ContentType::ECL) {
            return new DaxBlockEcl();
        } else if (contentType == ContentType::GEO) {
            return new DaxBlockGeo();
        } else {
            return nullptr;
        }
    }

    GfxDataHeader DaxBlock::popGfxHeader() {
        if (_data.size() < sizeof(GfxDataHeader)) {
            // Handle error: not enough data to extract a header
            return GfxDataHeader();
        }

        Common::MemoryReadStream readStream(_data.data(), _data.size());

        GfxDataHeader header;

        // Extract bytes to populate header
        header.height = readStream.readUint16LE();
        header.cwidth = readStream.readUint16LE();
        header.frameNum = readStream.readUint16LE();
        header.w_unk = readStream.readUint16LE();
        header.elemNum = readStream.readByte();
        readStream.read(header.a_unk, sizeof(header.a_unk));

        // Remove the used bytes from the array
        _data.erase(_data.begin(), _data.begin() + 17);

        return header;
    }

    DaxBlock8x8D::DaxBlock8x8D() {
        height = 8;
        width = 8;
        item_count = 0;
    }

    void DaxBlock8x8D::adjust() {
        if (blockId != 201) {
            GfxDataHeader gfx_header = popGfxHeader();
            height = gfx_header.height;
            width = gfx_header.cwidth * 8;
            item_count = gfx_header.elemNum;
        } else {
            item_count = _data.size() / 8;
        }
    }

    DaxBlockPic::DaxBlockPic() {
        height = 0;
        width = 0;
    }

    void DaxBlockPic::adjust() {
        GfxDataHeader gfx_header = popGfxHeader();
        height = gfx_header.height;
        width = gfx_header.cwidth * 8;
    }

DaxBlockWalldef::DaxBlockWalldef() {}

void DaxBlockWalldef::adjust() {
    _chunks.clear();
    size_t count = _data.size() / CHUNK_SIZE;
    for (size_t i = 0; i < count; ++i) {
        _chunks.push_back(Chunk(_data.data() + i * CHUNK_SIZE));
    }
}

int DaxBlockWalldef::Slice::cols(WalldefRegionId id) const {
    int idx = static_cast<int>(id);
    if (idx < 0 || idx >= VIEW_COUNT)
        return 0;

    return kWalldefViewCols[idx];
}

int DaxBlockWalldef::Slice::rows(WalldefRegionId id) const {
    int idx = static_cast<int>(id);
    if (idx < 0 || idx >= VIEW_COUNT)
        return 0;

    return kWalldefViewRows[idx];
}

uint8 DaxBlockWalldef::Slice::tileIndex(WalldefRegionId id, int row, int col) const {
    Common::Span<const uint8> data = region(id);
    int c = cols(id);
    int r = rows(id);

    if (c <= 0 || r <= 0)
        return 0;

    if (row < 0 || row >= r || col < 0 || col >= c)
        return 0;

    int offset = row * c + col;
    if (offset >= data.size())
        return 0;

    return data[offset];
}

DaxBlockEcl::DaxBlockEcl() {}

void DaxBlockEcl::adjust() {
    // For now, expose the raw program bytes as a span.
    if (_data.size() > 0) {
        _program = Common::Span<const uint8>(_data.data(), _data.size());
    } else {
        _program = Common::Span<const uint8>();
    }
}

DaxBlockGeo::DaxBlockGeo() : _dataLength(0), _raw(), _northEastWalls(),
    _southWestWalls(), _events(), _doors() {}

void DaxBlockGeo::adjust() {
    // GEO format: 2-byte little-endian payload length followed by four
    // 256-byte sections: NE walls, SW walls, events, doors.
    _dataLength = 0;
    _raw = Common::Span<const uint8>();
    _northEastWalls = Common::Span<const uint8>();
    _southWestWalls = Common::Span<const uint8>();
    _events = Common::Span<const uint8>();
    _doors = Common::Span<const uint8>();

    if (_data.size() < 2)
        return;

    Common::MemoryReadStream readStream(_data.data(), _data.size());
    _dataLength = readStream.readUint16LE();

    const size_t payloadSize = (_data.size() > 2) ? _data.size() - 2 : 0;
    const size_t requiredSize = CELL_COUNT * 4; // 4 blocks of 256 bytes
    if (payloadSize < requiredSize)
        return;

    const uint8 *payload = _data.data() + 2;

    _northEastWalls = Common::Span<const uint8>(payload, CELL_COUNT);
    _southWestWalls = Common::Span<const uint8>(payload + CELL_COUNT,
            CELL_COUNT);
    _events = Common::Span<const uint8>(payload + CELL_COUNT * 2,
            CELL_COUNT);
    _doors = Common::Span<const uint8>(payload + CELL_COUNT * 3,
            CELL_COUNT);

    _raw = Common::Span<const uint8>(payload, payloadSize);
}

int DaxBlockGeo::cellIndex(int row, int col) const {
    if (row < 0 || row >= GRID_SIZE || col < 0 || col >= GRID_SIZE)
        return -1;

    return row * GRID_SIZE + col;
}

DaxBlockGeo::Walls DaxBlockGeo::wallsAt(int row, int col) const {
    Walls walls = { 0, 0, 0, 0 };

    int idx = cellIndex(row, col);
    if (idx < 0)
        return walls;

        if (_northEastWalls.size() < CELL_COUNT
            || _southWestWalls.size() < CELL_COUNT)
        return walls;

    uint8 ne = _northEastWalls[idx];
    uint8 sw = _southWestWalls[idx];

    walls.north = (ne >> 4) & 0x0f;
    walls.east = ne & 0x0f;
    walls.south = (sw >> 4) & 0x0f;
    walls.west = sw & 0x0f;

    return walls;
}

uint8 DaxBlockGeo::eventAt(int row, int col) const {
    int idx = cellIndex(row, col);
    if (idx < 0)
        return 0;

    if (_events.size() < CELL_COUNT)
        return 0;

    return _events[idx];
}

uint8 DaxBlockGeo::doorAt(int row, int col) const {
    int idx = cellIndex(row, col);
    if (idx < 0)
        return 0;

    if (_doors.size() < CELL_COUNT)
        return 0;

    return _doors[idx];
}

} // namespace Data
} // namespace Goldbox
