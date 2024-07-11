// DaxBlock.cpp
#include "common/util.h"
#include "daxblock.h"
#include "randommanager.h"
#include "common/memstream.h"

namespace GoldBox {

    DaxBlock* DaxBlock::createDaxBlock(ContentType contentType) {
        if (contentType == ContentType::_8X8D) {
            return new DaxBlock8x8D();
        } else {
            return nullptr;
        }
    }

PictureBlock::PictureBlock(Common::Array<uint8>& dataRef) : data(dataRef), stream(dataRef.data(), dataRef.size()) {}

void PictureBlock::getPictureProperties() {
    if (data.size() >= 4) {
        stream.seek(0); // Reset stream position to the start
        height = stream.readUint16LE();
        width = stream.readUint16LE();
    }
}

    DaxBlock8x8D::DaxBlock8x8D() {
        // Initialization specific to _8X8D
    }

    void DaxBlock8x8D::process() const {
        // Implementation specific to _8X8D
    }

    void DaxBlock8x8D::FlipIconLeftToRight() {
        // Implementation specific to _8X8D
    }

    void DaxBlock8x8D::MergeIcons(const DaxBlock8x8D &srcIcon) {
        // Implementation specific to _8X8D
    }

    void DaxBlock8x8D::DaxToPicture(int mask_color, int masked) {
        // Implementation specific to _8X8D
    }

    void DaxBlock8x8D::SetMaskedColor(int offset, int color, int masked, int mask_color) {
        // Implementation specific to _8X8D
    }

} // namespace GoldBox
