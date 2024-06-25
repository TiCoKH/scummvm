// DaxBlock.h
#ifndef GOLD_BOX_DAXBLOCK_H
#define GOLD_BOX_DAXBLOCK_H

#include "common/array.h"
#include "common/stream.h"
#include "common/scummsys.h"

namespace GoldBox {

    struct DaxHeader {
        uint8 id;
        uint32 offset;
        uint16 rawSize; // decodeSize
        uint16 compSize; // dataLength
    };

    class DaxBlock {
    public:
        int height;
        int width;
        int x_pos;
        int y_pos;
        int item_count;
        Common::Array<uint8> field_9;
        int bpp;
        Common::Array<uint8> data;
        DaxHeader header;

        DaxBlock(int masked, int _item_count, int _width, int _height);
        DaxBlock(Common::ReadStream &stream, int masked, int mask_color);

        void FlipIconLeftToRight();
        void Recolor(bool useRandom, const Common::Array<uint8> &newColors, const Common::Array<uint8> &oldColors);
        void MergeIcons(const DaxBlock &srcIcon);

    private:
        void DaxToPicture(int mask_color, int masked, Common::ReadStream &stream);
        void SetMaskedColor(int offset, int color, int masked, int mask_color);
    };

} // namespace GoldBox

#endif // GOLD_BOX_DAXBLOCK_H
