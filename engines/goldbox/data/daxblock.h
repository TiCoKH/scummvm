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

#ifndef GOLDBOX_DATA_DAXBLOCK_H
#define GOLDBOX_DATA_DAXBLOCK_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/span.h"

namespace Goldbox {
namespace Data {

    struct DaxHeader {
        uint8 id;
        uint32 offset;
        uint16 rawSize; // decodeSize
        uint16 compSize; // dataLength

        // Default constructor
        DaxHeader() = default;

        // Copy constructor
        DaxHeader(const DaxHeader &other) = default;

        // Copy assignment operator
        DaxHeader& operator=(const DaxHeader &other) = default;
    };

    struct GfxDataHeader {
        uint16 height;
        uint16 cwidth;
        uint16 frameNum; //???
        uint16 w_unk;
        uint8 elemNum;
        uint8 a_unk[8];

        // Default constructor
        GfxDataHeader() = default;

        // Copy constructor
        GfxDataHeader(const GfxDataHeader &other) = default;

        // Copy assignment operator
        GfxDataHeader& operator=(const GfxDataHeader &other) = default;
    };

    enum class ContentType {
        TILE, BACK, BIGPIC, BODY, CBODY, CHEAD, COMSPR, ECL, GEO, HEAD, MONCHA, PIC, SPRIT, TITLE, WALLDEF, UNKNOWN
    };

    enum class WalldefRegionId {
        FAR_FORWARD,
        FAR_LEFT,
        FAR_RIGHT,
        MED_FORWARD,
        MED_LEFT,
        MED_RIGHT,
        CLOSE_FORWARD,
        CLOSE_LEFT,
        CLOSE_RIGHT,
        FAR_FILLER,
        WALDEF_REGION_COUNT
    };

    struct WalldefRegion {
        int offset;
        int size;
        uint8 x_dim, y_dim;
    };

    static const WalldefRegion kWalldefRegions[] = {
        { 0,   2, 1, 2 },   // FAR_FORWARD
        { 2,   4, 1, 4 },   // FAR_LEFT
        { 6,   4, 1, 4 },   // FAR_RIGHT
        { 10, 12, 3, 4 },   // MED_FORWARD
        { 22, 16, 2, 8 },   // MED_LEFT
        { 38, 16, 2, 8 },   // MED_RIGHT
        { 54, 56, 7, 8 },   // CLOSE_FORWARD
        { 110, 22, 2, 11 },  // CLOSE_LEFT
        { 132, 22, 2, 11 },  // CLOSE_RIGHT
        { 154, 2, 1, 2 }    // FAR_FILLER
    };


class DaxBlock {
public:
    virtual ~DaxBlock() = default; // Ensure it has a virtual destructor
    virtual void adjust() = 0;

    // Common properties
    uint8 blockId;
    Common::Array<uint8> _data;

    static DaxBlock* createDaxBlock(ContentType contentType);

protected:
    GfxDataHeader popGfxHeader();

};


class DaxBlockPic : public DaxBlock {
public:
    int height;
    int width;

    DaxBlockPic();

private:
    void adjust() override;
};


class DaxBlock8x8D : public DaxBlockPic {
public:
    int item_count;

    DaxBlock8x8D();

private:
    void adjust() override;
};

class DaxBlockWalldef : public DaxBlock {
public:
    static const int SLICE_COUNT = 5;
    static const int SLICE_SIZE = 156;
    static const int CHUNK_SIZE = SLICE_COUNT * SLICE_SIZE; // 780

    class Slice {
    public:
        Slice(const uint8 *data) : _data(data, SLICE_SIZE) {}

        Common::Span<const uint8> region(WalldefRegionId id) const {
            const WalldefRegion &r = kWalldefRegions[static_cast<int>(id)];
            assert(r.offset + r.size <= SLICE_SIZE);
            return Common::Span<const uint8>(_data.data() + r.offset, r.size);
        }

        Common::Span<const uint8> raw() const { return _data; }

    private:
        Common::Span<const uint8> _data;
    };

    class Chunk {
    public:
        Chunk(const uint8 *data) : _data(data, CHUNK_SIZE) {}

        Slice slice(int idx) const {
            assert(idx >= 0 && idx < SLICE_COUNT);
            return Slice(_data.data() + idx * SLICE_SIZE);
        }

        Common::Span<const uint8> raw() const { return _data; }

    private:
        Common::Span<const uint8> _data;
    };

    DaxBlockWalldef();

    int chunkCount() const { return _chunks.size(); }
    const Chunk &chunk(int idx) const { return _chunks[idx]; }

private:
    void adjust() override;

    Common::Array<Chunk> _chunks;
};

} // namespace Data
} // namespace Goldbox
#endif // GOLDBOX_DAXBLOCK_H
