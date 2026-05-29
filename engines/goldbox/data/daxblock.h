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

// Forward declaration for DaxBlockGeo and DaxBlockWalldef
class DaxBlockGeo;
class DaxBlockWalldef;

using DungeonMap = DaxBlockGeo;
using WallSet = DaxBlockWalldef;

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
        TILE, BACK, BIGPIC, CTILE, ECL, GEO, CHARACTER, PIC, SPRIT, WALLDEF, ITEM, SPELL, UNKNOWN
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

class DaxBlockRaw : public DaxBlock {
public:
    DaxBlockRaw() {}

private:
    void adjust() override {}
};

class DaxBlockSprit : public DaxBlock {
public:
    struct FrameInfo {
        uint16 width;    // pixels (charWidth * 8)
        uint16 height;   // pixels
        int16 xPos;      // viewport x offset
        int16 yPos;      // viewport y offset
        uint32 dataOffset; // byte offset into _data for EGA plane data
        uint32 dataSize;   // EGA plane data size (height * charWidth * 4)
    };

    DaxBlockSprit();

    int frameCount() const { return _frameCount; }
    bool isValidLayout() const { return _validLayout; }
    const FrameInfo *frameInfo(int idx) const;

    /** Access raw block data for EGA plane decoding. */
    const Common::Array<uint8> &rawData() const { return _data; }

private:
    void adjust() override;

    static uint16 readUint16LE(const Common::Array<uint8> &data, uint pos);

    int _frameCount;
    bool _validLayout;
    FrameInfo _frames[8];
};


class DaxBlockPic : public DaxBlock {
public:
    int height;
    int width;
    int frameCount;

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
    static const int VIEW_COUNT = static_cast<int>(WalldefRegionId::WALDEF_REGION_COUNT);

    class Slice {
    public:
        Slice(const uint8 *data) : _data(data, SLICE_SIZE) {}

        Common::Span<const uint8> region(WalldefRegionId id) const {
            const WalldefRegion &r = kWalldefRegions[static_cast<int>(id)];
            assert(r.offset + r.size <= SLICE_SIZE);
            return Common::Span<const uint8>(_data.data() + r.offset, r.size);
        }

        int cols(WalldefRegionId id) const;
        int rows(WalldefRegionId id) const;
        uint8 tileIndex(WalldefRegionId id, int row, int col) const;

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

        uint8 tileIndex(int sliceIdx, WalldefRegionId id, int row, int col) const {
            return slice(sliceIdx).tileIndex(id, row, col);
        }

        Common::Span<const uint8> region(int sliceIdx, WalldefRegionId id) const {
            return slice(sliceIdx).region(id);
        }

        Common::Span<const uint8> raw() const { return _data; }

    private:
        Common::Span<const uint8> _data;
    };

    /**
     * Tile ID slot base offsets. Index N gives the first global tile ID that
     * belongs to cache slot N. Matches gbl.symbol_set_fix = {1,46,116,186,256}
     * from the C# reimplementation.
     * - Slot 0: IDs  1-45   (universal/common tiles, 8x8D block 203)
     * - Slot 1: IDs 46-115  (WALLDEF symbolSet 1)
     * - Slot 2: IDs 116-185 (WALLDEF symbolSet 2)
     * - Slot 3: IDs 186-255 (WALLDEF symbolSet 3)
     * - Slot 4: IDs 256+    (extra/extended tiles)
     */
    static const uint16 kTileSlotBase[5];

    DaxBlockWalldef();

    int chunkCount() const { return _chunks.size(); }
    const Chunk &chunk(int idx) const { return _chunks[idx]; }

    /** Reset chunk span to point at original unpatched data. */
    void resetChunk(int chunkIdx);

private:
    void adjust() override;

    Common::Array<Chunk> _chunks;
};

class DaxBlockEcl : public DaxBlock {
public:
    DaxBlockEcl();

    Common::Span<const uint8> program() const { return _program; }

private:
    void adjust() override;

    Common::Span<const uint8> _program;
};

class DaxBlockGeo : public DaxBlock {
public:
    enum Direction { NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3 };

    /**
     * 2-bit door state packed per direction into plane 3 of the GEO block.
     * Matches GeoWallRecord door constants from the C# reimplementation:
     *   ndoor/edoor/sdoor/wdoor      = DOOR_OPEN   (1)
     *   ndoor_locked/...             = DOOR_LOCKED  (2)
     *   ndoor_wizard/...             = DOOR_WIZARD  (3)
     * Bit layout within doorAt() byte:
     *   bits [1:0] = North, [3:2] = East, [5:4] = South, [7:6] = West
     */
    enum DoorState {
        DOOR_NONE   = 0,
        DOOR_OPEN   = 1,
        DOOR_LOCKED = 2,
        DOOR_WIZARD = 3
    };

    /**
     * Per-direction door bit-masks within the raw door byte.
     * Mirrors GeoWallRecord: ndoor=1, ndoor_locked=2, edoor=4, edoor_locked=8, etc.
     */
    enum DoorMask {
        DOOR_MASK_NORTH        = 0x01,
        DOOR_MASK_NORTH_LOCKED = 0x02,
        DOOR_MASK_NORTH_WIZARD = 0x03,
        DOOR_MASK_EAST         = 0x04,
        DOOR_MASK_EAST_LOCKED  = 0x08,
        DOOR_MASK_EAST_WIZARD  = 0x0C,
        DOOR_MASK_SOUTH        = 0x10,
        DOOR_MASK_SOUTH_LOCKED = 0x20,
        DOOR_MASK_SOUTH_WIZARD = 0x30,
        DOOR_MASK_WEST         = 0x40,
        DOOR_MASK_WEST_LOCKED  = 0x80,
        DOOR_MASK_WEST_WIZARD  = 0xC0
    };

    /**
     * Decoded representation of a single GEO grid cell.
     * Mirrors MapInfo from the C# reimplementation.
     *
     * Wall type nibbles (4 bits each, from planes 0 and 1):
     *   wallType[NORTH/EAST] packed in plane 0 byte: high nibble=N, low nibble=E
     *   wallType[SOUTH/WEST] packed in plane 1 byte: high nibble=S, low nibble=W
     * Event byte (plane 2): event number 0-127 triggering ECL execution.
     * Door state (plane 3): 2-bit state per direction (see DoorState enum).
     */
    struct MapCell {
        uint8 wallType[4];  // indexed by Direction
        uint8 doorState[4]; // 2-bit DoorState per Direction
        uint8 event;        // ECL event number (0 = none)
    };

    struct Walls {
        uint8 north;
        uint8 east;
        uint8 south;
        uint8 west;
    };

    static const int GRID_SIZE = 16;
    static const int CELL_COUNT = GRID_SIZE * GRID_SIZE;

    DaxBlockGeo();

    uint16 dataLength() const { return _dataLength; }
    Common::Span<const uint8> raw() const { return _raw; }

    Walls wallsAt(int row, int col) const;
    uint8 eventAt(int row, int col) const;
    uint8 doorAt(int row, int col) const;

    /**
     * Return fully decoded cell at (row, col).
     * Combines wallsAt(), eventAt(), and getDoorState() into one struct.
     */
    MapCell cellAt(int row, int col) const;

    bool canMove(int row, int col, Direction dir) const;
    bool isDoor(int row, int col, Direction dir) const;
    uint8 getWallType(int row, int col, Direction dir) const;
    uint8 getDoorState(int row, int col, Direction dir) const;

private:
    void adjust() override;

    int cellIndex(int row, int col) const;

    uint16 _dataLength;
    Common::Span<const uint8> _raw;
    Common::Span<const uint8> _northEastWalls;
    Common::Span<const uint8> _southWestWalls;
    Common::Span<const uint8> _events;
    Common::Span<const uint8> _doors;
};

} // namespace Data
} // namespace Goldbox
#endif // GOLDBOX_DAXBLOCK_H
