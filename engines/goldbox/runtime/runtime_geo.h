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

#ifndef GOLDBOX_RUNTIME_RUNTIME_GEO_H
#define GOLDBOX_RUNTIME_RUNTIME_GEO_H

#include "common/scummsys.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {

/**
 * Mutable runtime GEO map state.
 *
 * Mirrors the original PTR_GEO_BUFF: a 0x400-byte working copy of the
 * current dungeon map that ECL scripts and engine systems can read and
 * modify at runtime. When a new GEO block is loaded (map transition),
 * the runtime state is reset from the immutable DaxBlockGeo on disk.
 *
 * Memory layout (identical to legacy PTR_GEO_BUFF):
 *   +0x000..0x0FF  Plane 0: North/East wall nibbles (high=N, low=E)
 *   +0x100..0x1FF  Plane 1: South/West wall nibbles (high=S, low=W)
 *   +0x200..0x2FF  Plane 2: Event data (7-bit event ID + flag bit)
 *   +0x300..0x3FF  Plane 3: Door state (2 bits per direction)
 *
 * Direction encoding for door/wall flag operations (legacy wire format):
 *   0=North, 2=East, 4=South, 6=West
 */
class RuntimeGeoBlock {
public:
    static const int GRID_SIZE = 16;
    static const int CELL_COUNT = GRID_SIZE * GRID_SIZE;
    static const int PLANE_SIZE = CELL_COUNT;
    static const int BUFFER_SIZE = PLANE_SIZE * 4;

    RuntimeGeoBlock();

    /**
     * Reset runtime state from an immutable DaxBlockGeo (disk data).
     * Called on every GEO block load / map transition.
     */
    void loadFromGeoBlock(const Data::DaxBlockGeo &geo);

    /**
     * Reset runtime state from a raw 0x400-byte buffer.
     */
    void loadFromBuffer(const uint8 *buf, uint32 size);

    /** True if a valid map has been loaded. */
    bool isLoaded() const { return _loaded; }

    /** Currently loaded block ID (0xFF if none). */
    uint8 blockId() const { return _blockId; }

    void setBlockId(uint8 id) { _blockId = id; }

    /** Clear all runtime state. */
    void clear();

    // -----------------------------------------------------------------------
    // Read accessors (match DaxBlockGeo API)
    // -----------------------------------------------------------------------

    bool isInBounds(int x, int y) const;

    /**
     * Get wall nibble for a direction at (x, y).
     * Direction uses legacy wire format: 0=N, 2=E, 4=S, 6=W.
     * Returns 4-bit wall type (0-15), or 0 if out of bounds on map 0/10.
     */
    uint8 getMapNibble(int x, int y, uint8 wireDir) const;

    /**
     * Get the event byte at (x, y) from plane 2.
     * Returns full byte (bit 7 = flag, bits 0-6 = event ID).
     */
    uint8 getGeoData(int x, int y) const;

    /**
     * Get 2-bit door/wall flag for a direction at (x, y).
     * Direction uses legacy wire format: 0=N, 2=E, 4=S, 6=W.
     */
    uint8 getWallFlag(int x, int y, uint8 wireDir) const;

    // -----------------------------------------------------------------------
    // Write accessors (ECL script mutations)
    // -----------------------------------------------------------------------

    /**
     * Clear the 2-bit door flag for a direction at (x, y).
     * Direction uses legacy wire format: 0=N, 2=E, 4=S, 6=W.
     */
    void clearFlag(int x, int y, uint8 wireDir);

    /**
     * Set the 2-bit door flag to state 1 (open) for a direction at (x, y).
     * Direction uses legacy wire format: 0=N, 2=E, 4=S, 6=W.
     */
    void setTileDirectionState(int x, int y, uint8 wireDir);

    /**
     * Direct write to a plane byte (for save/load or ECL raw access).
     */
    void writePlane(int plane, int index, uint8 value);

    /**
     * Direct read from a plane byte.
     */
    uint8 readPlane(int plane, int index) const;

    /** Raw buffer access (for legacy save compatibility). */
    const uint8 *rawBuffer() const { return _buf; }

    /**
     * Set the map ID used for wrapping behavior.
     * Maps 0 and 10 return 0 for out-of-bounds; others wrap coordinates.
     */
    void setMapId(uint8 mapId) { _mapId = mapId; }
    uint8 mapId() const { return _mapId; }

private:
    void wrapCoords(int &x, int &y) const;
    int cellIndex(int x, int y) const { return x + y * GRID_SIZE; }

    uint8 _buf[BUFFER_SIZE];
    bool _loaded;
    uint8 _blockId;
    uint8 _mapId;
};

} // namespace Goldbox

#endif // GOLDBOX_RUNTIME_RUNTIME_GEO_H
