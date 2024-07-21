// DaxBlock.cpp
#include "common/util.h"
#include "common/memstream.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {

    DaxBlock* DaxBlock::createDaxBlock(ContentType contentType) {
        if (contentType == ContentType::_8X8D) {
            return new DaxBlock8x8D();
        } else if (contentType == ContentType::TITLE) {
            return new DaxBlockTitle();
        } else {
            return nullptr;
        }
    }

    GfxDataHeader DaxBlock8x8D::popGfxHeader() {
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
        _data.erase(_data.begin(), _data.begin() + sizeof(GfxDataHeader));

        return header;
    }

    DaxBlock8x8D::DaxBlock8x8D() {
        height = 8;
        width = 8;
    }

    void DaxBlock8x8D::adjust() {
        if (blockId != 201) {
            GfxDataHeader gfx_header = popGfxHeader();
            height = gfx_header.height;
            width = gfx_header.cwidth * 8;
            item_count = gfx_header.elemNum;
        }
        item_count = _data.size() / 8;
    }

    DaxBlockTitle::DaxBlockTitle() {
        height = 0;
        width = 0;
    }

    void DaxBlockTitle::adjust() {
        GfxDataHeader gfx_header = popGfxHeader();
        height = gfx_header.height;
        width = gfx_header.cwidth * 8;
        item_count = gfx_header.elemNum;
    }

} // namespace Goldbox
