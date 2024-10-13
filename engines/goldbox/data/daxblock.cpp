// DaxBlock.cpp
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

} // namespace Data
} // namespace Goldbox
