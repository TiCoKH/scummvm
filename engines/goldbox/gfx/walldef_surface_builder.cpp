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

#include "goldbox/gfx/walldef_surface_builder.h"

#include "common/bitarray.h"
#include "goldbox/gfx/dax_tile.h"
#include "goldbox/gfx/pic.h"

namespace Goldbox {
namespace Gfx {

namespace {

static const uint16 kTileCacheFirstIds[Tile8x8Cache::kSlotCount] = {
	1, 0x2E, 0x74, 0xBA, 0x100
};

static const uint16 kTileCacheLastIds[Tile8x8Cache::kSlotCount] = {
	0x2D, 0x73, 0xB9, 0xFF, 0x11E
};

void blitSymbol(const Graphics::ManagedSurface &symbol, Pic &dst, int dstX,
		int dstY, Common::BitArray *mask) {
	for (int py = 0; py < symbol.h; ++py) {
		for (int px = 0; px < symbol.w; ++px) {
			const uint8 pixel = symbol.getPixel(px, py);
			if (pixel == 0)
				continue;

			const int x = dstX + px;
			const int y = dstY + py;
			dst.setPixel(x, y, pixel);
			if (mask)
				mask->unset(y * dst.w + x);
		}
	}
}

Common::SharedPtr<Pic> buildRegionSurface(
		const Data::DaxBlockWalldef::Slice &slice,
		Data::WalldefRegionId regionId,
		const Tile8x8Cache &tileCache) {
	const int cols = slice.cols(regionId);
	const int rows = slice.rows(regionId);
	Common::SharedPtr<Pic> region(new Pic(cols * 8, rows * 8));
	region->clear(0);
	region->setTransparentIndex(0);

	Common::BitArray *mask = region->ensureTransparencyMask();
	for (uint i = 0; i < mask->size(); ++i)
		mask->set(i);

	for (int row = 0; row < rows; ++row) {
		for (int col = 0; col < cols; ++col) {
			const uint16 globalTileId = slice.tileIndex(regionId, row, col);
			if (globalTileId == 0)
				continue;

			const Graphics::ManagedSurface *tile =
					tileCache.tileSurface(globalTileId);
			if (!tile)
				continue;

			blitSymbol(*tile, *region, col * 8, row * 8, mask);
		}
	}

	return region;
}

} // namespace

Tile8x8Cache::Tile8x8Cache() {
	for (int i = 0; i < kSlotCount; ++i)
		_slots[i] = nullptr;
}

void Tile8x8Cache::setSlot(int slot, const DaxTile *tiles) {
	if (slot < 0 || slot >= kSlotCount)
		return;

	_slots[slot] = tiles;
}

const DaxTile *Tile8x8Cache::getSlot(int slot) const {
	if (slot < 0 || slot >= kSlotCount)
		return nullptr;

	return _slots[slot];
}

int Tile8x8Cache::slotForGlobalTileId(uint16 globalTileId) const {
	for (int slot = 0; slot < kSlotCount; ++slot) {
		if (globalTileId >= kTileCacheFirstIds[slot]
				&& globalTileId <= kTileCacheLastIds[slot])
			return slot;
	}

	return -1;
}

int Tile8x8Cache::localTileIndex(uint16 globalTileId) const {
	const int slot = slotForGlobalTileId(globalTileId);
	if (slot < 0)
		return -1;

	return globalTileId - kTileCacheFirstIds[slot];
}

const Graphics::ManagedSurface *Tile8x8Cache::tileSurface(
		uint16 globalTileId) const {
	const int slot = slotForGlobalTileId(globalTileId);
	if (slot < 0)
		return nullptr;

	const DaxTile *tiles = _slots[slot];
	if (!tiles)
		return nullptr;

	const int localIndex = localTileIndex(globalTileId);
	if (localIndex < 0)
		return nullptr;

	return tiles->getTileSurface(localIndex);
}

uint16 Tile8x8Cache::firstGlobalTileIdForSlot(int slot) {
	if (slot < 0 || slot >= kSlotCount)
		return 0;

	return kTileCacheFirstIds[slot];
}

uint16 Tile8x8Cache::lastGlobalTileIdForSlot(int slot) {
	if (slot < 0 || slot >= kSlotCount)
		return 0;

	return kTileCacheLastIds[slot];
}

WallSurfaceSet::WallSurfaceSet() {
	_regions.resize(Data::DaxBlockWalldef::VIEW_COUNT);
}

Pic *WallSurfaceSet::region(Data::WalldefRegionId id) {
	return _regions[static_cast<int>(id)].get();
}

const Pic *WallSurfaceSet::region(Data::WalldefRegionId id) const {
	return _regions[static_cast<int>(id)].get();
}

void WallSurfaceSet::setRegion(Data::WalldefRegionId id,
		const Common::SharedPtr<Pic> &pic) {
	_regions[static_cast<int>(id)] = pic;
}

WallSurfaceSet WalldefSurfaceBuilder::buildSlice(
		const Data::DaxBlockWalldef::Slice &slice,
		const Tile8x8Cache &tileCache) {
	WallSurfaceSet result;

	for (int i = 0; i < Data::DaxBlockWalldef::VIEW_COUNT; ++i) {
		const Data::WalldefRegionId regionId =
				static_cast<Data::WalldefRegionId>(i);
		result.setRegion(regionId,
				buildRegionSurface(slice, regionId, tileCache));
	}

	return result;
}

Common::Array<WallSurfaceSet> WalldefSurfaceBuilder::buildChunk(
		const Data::DaxBlockWalldef::Chunk &chunk,
		const Tile8x8Cache &tileCache) {
	Common::Array<WallSurfaceSet> result;
	result.reserve(Data::DaxBlockWalldef::SLICE_COUNT);

	for (int i = 0; i < Data::DaxBlockWalldef::SLICE_COUNT; ++i)
		result.push_back(buildSlice(chunk.slice(i), tileCache));

	return result;
}

Common::Array<WallSurfaceSet> WalldefSurfaceBuilder::buildChunk(
		const Data::DaxBlockWalldef &walldef,
		int chunkIdx,
		const Tile8x8Cache &tileCache) {
	if (chunkIdx < 0 || chunkIdx >= walldef.chunkCount())
		return Common::Array<WallSurfaceSet>();

	return buildChunk(walldef.chunk(chunkIdx), tileCache);
}

} // namespace Gfx
} // namespace Goldbox