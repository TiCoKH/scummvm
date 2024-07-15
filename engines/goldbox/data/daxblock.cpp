// DaxBlock.cpp
#include "common/util.h"
#include "common/memstream.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {

    DaxBlock* DaxBlock::createDaxBlock(ContentType contentType) {
        if (contentType == ContentType::_8X8D) {
            return new DaxBlock8x8D();
        } else {
            return nullptr;
        }
    }

    DaxBlock8x8D::DaxBlock8x8D() {
        height = 8;
        width = 8;
    }

    void DaxBlock8x8D::adjust() {
        if (blockId != 201) {
            item_count = _data.size() / 8;
        } else {

        }
    }

} // namespace Goldbox
