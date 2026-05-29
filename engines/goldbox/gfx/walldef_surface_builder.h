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

#ifndef GOLDBOX_GFX_WALLDEF_SURFACE_BUILDER_H
#define GOLDBOX_GFX_WALLDEF_SURFACE_BUILDER_H

#include "common/array.h"
#include "common/ptr.h"
#include "goldbox/data/daxblock.h"

namespace Graphics {
class ManagedSurface;
}

namespace Goldbox {
namespace Gfx {

class DaxTile;
class Pic;

class Tile8x8Cache {
public:
	static const int kSlotCount = 5;

	Tile8x8Cache();

	void setSlot(int slot, const DaxTile *tiles);
	const DaxTile *getSlot(int slot) const;

	int slotForGlobalTileId(uint16 globalTileId) const;
	int localTileIndex(uint16 globalTileId) const;
	const Graphics::ManagedSurface *tileSurface(uint16 globalTileId) const;

	static uint16 firstGlobalTileIdForSlot(int slot);
	static uint16 lastGlobalTileIdForSlot(int slot);

private:
	const DaxTile *_slots[kSlotCount];
};

class WallSurfaceSet {
public:
	WallSurfaceSet();

	Pic *region(Data::WalldefRegionId id);
	const Pic *region(Data::WalldefRegionId id) const;

	void setRegion(Data::WalldefRegionId id, const Common::SharedPtr<Pic> &pic);

private:
	Common::Array<Common::SharedPtr<Pic> > _regions;
};

class WalldefSurfaceBuilder {
public:
	static WallSurfaceSet buildSlice(const Data::DaxBlockWalldef::Slice &slice,
			const Tile8x8Cache &tileCache);

	static Common::Array<WallSurfaceSet> buildChunk(
			const Data::DaxBlockWalldef::Chunk &chunk,
			const Tile8x8Cache &tileCache);
};

/**
 * Runtime cache of pre-built WallSurfaceSet arrays for the three dynamic
 * walldef symbol-set slots (1–3). Replaces C# gbl.wallDef (WallDefs class).
 *
 * Slot mapping (mirrors gbl.symbol_set_fix):
 *   wallType  1- 5 → slot 1, slice 0-4
 *   wallType  6-10 → slot 2, slice 0-4
 *   wallType 11-15 → slot 3, slice 0-4
 *
 * Each slot is loaded via ECL opcode 0x37 (LOAD_PIECES / loadWallSet).
 */
class WalldefSlotCache {
public:
	static const int kSlotCount = 3; // dynamic slots 1, 2, 3

	WalldefSlotCache();

	/**
	 * Build and store the WallSurfaceSet array for slot (1..3).
	 * Applies the tile ID offset for the slot before building surfaces.
	 * @param slot      Symbol-set slot (1..3)
	 * @param walldef   Source DaxBlockWalldef (must be non-null)
	 * @param chunkIdx  Which chunk within the block (0-based)
	 * @param tileCache Active tile atlas cache
	 */
	void loadSlot(int slot, Data::DaxBlockWalldef *walldef, int chunkIdx,
			const Tile8x8Cache &tileCache);

	/** Clear all WallSurfaceSet data for a slot. */
	void clearSlot(int slot);

	/**
	 * Return the WallSurfaceSet for a given wallType (1..15).
	 * Selects slot as (wallType-1)/5 + 1, slice as (wallType-1)%5.
	 * Returns nullptr if the slot is not loaded or out of range.
	 */
	const WallSurfaceSet *surfaceSetForWallType(uint8 wallType) const;

	/** Debug: return the walldef block ID used to load a slot (1..3). */
	uint8 walldefBlockIdForSlot(int slot) const;

	/** Debug: return the chunk index used to load a slot (1..3). */
	int chunkIndexForSlot(int slot) const;

private:
	// _slices[slot-1] holds one WallSurfaceSet per slice within the loaded chunk
	Common::Array<WallSurfaceSet> _slices[kSlotCount];

	// Debug metadata: track which walldef block/chunk was used per slot
	uint8 _walldefBlockId[kSlotCount];
	int _chunkIdx[kSlotCount];
};

} // namespace Gfx
} // namespace Goldbox

#endif