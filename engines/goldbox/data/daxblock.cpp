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

namespace Goldbox {
namespace Data {

    DaxBlock* DaxBlock::createDaxBlock(ContentType contentType) {
        if (contentType == ContentType::_8X8D) {
            return new DaxBlock8x8D();
        } else if (contentType == ContentType::TITLE) {
            return new DaxBlockPic();
        } else if (contentType == ContentType::WALLDEF) {
            return new DaxBlockWalldef();
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

} // namespace Data
} // namespace Goldbox
