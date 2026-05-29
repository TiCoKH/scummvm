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
		} else if (contentType == ContentType::CHARACTER
				|| contentType == ContentType::ITEM
				|| contentType == ContentType::SPELL) {
			return new DaxBlockRaw();
		} else if (contentType == ContentType::SPRIT) {
			return new DaxBlockSprit();
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
		frameCount = 0;
	}

	DaxBlockSprit::DaxBlockSprit() : _frameCount(0), _validLayout(false) {}

	uint16 DaxBlockSprit::readUint16LE(const Common::Array<uint8> &data,
			uint pos) {
		if (pos + 1 >= data.size())
			return 0;
		return static_cast<uint16>(data[pos]
			| (static_cast<uint16>(data[pos + 1]) << 8));
	}

	void DaxBlockSprit::adjust() {
		_frameCount = 0;
		_validLayout = false;
		memset(_frames, 0, sizeof(_frames));

		if (_data.empty())
			return;

		const uint frames = _data[0];
		if (frames == 0 || frames > 8)
			return;

		uint offset = 1;
		for (uint frame = 0; frame < frames; ++frame) {
			// 21-byte per-frame header in EGA SPRIT blocks.
			if (_data.size() < offset + 21)
				return;

			// delay (4 bytes)
			offset += 4;

			const uint16 height = readUint16LE(_data, offset);
			offset += 2;

			const uint16 charWidth = readUint16LE(_data, offset);
			offset += 2;

			const int16 xPos = static_cast<int16>(readUint16LE(_data, offset));
			offset += 2;

			const int16 yPos = static_cast<int16>(readUint16LE(_data, offset));
			offset += 2;

			// skip 1 byte (sub-image count) + 8 bytes (CGA color mapping)
			offset += 9;

			const uint32 widthPx = static_cast<uint32>(charWidth) * 8;
			const uint32 heightPx = static_cast<uint32>(height);
			if (widthPx < 1 || heightPx < 1 || widthPx > 320 || heightPx > 200)
				return;

			// EGA packed planes: height * charWidth * 4 bytes.
			const uint32 egaDataSize = static_cast<uint32>(height)
				* static_cast<uint32>(charWidth) * 4;
			if (egaDataSize > 0x7fffffff)
				return;

			if (_data.size() < offset + egaDataSize)
				return;

			_frames[frame].width = static_cast<uint16>(widthPx);
			_frames[frame].height = height;
			_frames[frame].xPos = xPos;
			_frames[frame].yPos = yPos;
			_frames[frame].dataOffset = offset;
			_frames[frame].dataSize = egaDataSize;

			offset += egaDataSize;
		}

		// Match strict C# specification: no trailing payload bytes.
		_frameCount = static_cast<int>(frames);
		if (offset != _data.size()) {
			// Frames parsed OK but trailing data exists — mark layout
			// as non-strict but keep frameCount valid for rendering.
			_validLayout = false;
			return;
		}

		_validLayout = true;
	}

	const DaxBlockSprit::FrameInfo *DaxBlockSprit::frameInfo(int idx) const {
		if (idx < 0 || idx >= _frameCount)
			return nullptr;
		return &_frames[idx];
	}

	void DaxBlockPic::adjust() {
		GfxDataHeader gfx_header = popGfxHeader();
		height = gfx_header.height;
		width = gfx_header.cwidth * 8;
		frameCount = gfx_header.frameNum;
	}

DaxBlockWalldef::DaxBlockWalldef() {}

const uint16 DaxBlockWalldef::kTileSlotBase[5] = { 1, 46, 116, 186, 256 };

void DaxBlockWalldef::adjust() {
	_chunks.clear();
	size_t count = _data.size() / CHUNK_SIZE;
	for (size_t i = 0; i < count; ++i)
		_chunks.push_back(Chunk(_data.data() + i * CHUNK_SIZE));
}

void DaxBlockWalldef::resetChunk(int chunkIdx) {
	if (chunkIdx < 0 || chunkIdx >= (int)_chunks.size())
		return;
	_chunks[chunkIdx] = Chunk(_data.data() + chunkIdx * CHUNK_SIZE);
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

bool DaxBlockGeo::canMove(int row, int col, Direction dir) const {
	uint8 wallType = getWallType(row, col, dir);
	if (wallType == 0) return true; // No wall

	// Check if it's an open door
	if (isDoor(row, col, dir)) {
		uint8 doorState = getDoorState(row, col, dir);
		return doorState == 1; // 1 = open, 2 = closed
	}

	return false; // Wall blocks
}

bool DaxBlockGeo::isDoor(int row, int col, Direction dir) const {
	uint8 doorState = getDoorState(row, col, dir);
	return doorState > 0; // 0 = not a door, 1+ = door
}

uint8 DaxBlockGeo::getWallType(int row, int col, Direction dir) const {
	Walls w = wallsAt(row, col);
	switch (dir) {
	case NORTH: return w.north;
	case EAST:  return w.east;
	case SOUTH: return w.south;
	case WEST:  return w.west;
	default:    return 0;
	}
}

uint8 DaxBlockGeo::getDoorState(int row, int col, Direction dir) const {
	uint8 doorByte = doorAt(row, col);
	// Extract 2-bit value for direction:
	// Bits [1:0]=North, [3:2]=East, [5:4]=South, [7:6]=West
	int shift = dir * 2;
	return (doorByte >> shift) & 0x03;
}

DaxBlockGeo::MapCell DaxBlockGeo::cellAt(int row, int col) const {
	MapCell cell = {};
	Walls w = wallsAt(row, col);
	cell.wallType[NORTH] = w.north;
	cell.wallType[EAST]  = w.east;
	cell.wallType[SOUTH] = w.south;
	cell.wallType[WEST]  = w.west;
	cell.event = eventAt(row, col);
	for (int d = 0; d < 4; ++d)
		cell.doorState[d] = getDoorState(row, col, static_cast<Direction>(d));
	return cell;
}

} // namespace Data
} // namespace Goldbox
