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

	static Common::Array<WallSurfaceSet> buildChunk(
			const Data::DaxBlockWalldef &walldef,
			int chunkIdx,
			const Tile8x8Cache &tileCache);
};

} // namespace Gfx
} // namespace Goldbox

#endif