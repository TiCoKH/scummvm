// DaxBlock.cpp
#include "common/util.h"
#include "daxblock.h"
#include "randommanager.h"

namespace GoldBox {

    DaxBlock::DaxBlock(int masked, int _item_count, int _width, int _height) 
        : item_count(_item_count), width(_width), height(_height) {
        bpp = height * width * 8;
        int ram_size = item_count * bpp;
        data.resize(ram_size);
        field_9.resize(8);
    }

    DaxBlock::DaxBlock(Common::ReadStream &stream, int masked, int mask_color) {
        height = stream.readUint16LE();
        width = stream.readUint16LE();
        bpp = height * width * 8;
        x_pos = stream.readUint16LE();
        y_pos = stream.readUint16LE();
        item_count = stream.readByte();

        field_9.resize(8);
        stream.read(field_9.begin(), 8);

        int ram_size = item_count * bpp;
        data.resize(ram_size);

        DaxToPicture(mask_color, masked, stream);
    }

    void DaxBlock::FlipIconLeftToRight() {
        Common::Array<uint8> t_data(data.size());

        int t_width = width * 8;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < t_width; x++) {
                int di = (y * t_width) + x;
                int si = (y * t_width) + (t_width - x) - 1;
                t_data[di] = data[si];
            }
        }

        memcpy(data.begin(), t_data.begin(), data.size());
    }

    void DaxBlock::Recolor(bool useRandom, const Common::Array<uint8> &newColors, const Common::Array<uint8> &oldColors) {
        Common::RandomSource &random_source = RandomManager::instance().getRandomSource();

        for (int colorIdx = 0; colorIdx < 16; colorIdx++) {
            if (oldColors[colorIdx] != newColors[colorIdx]) {
                int offset = 0;

                for (int posY = 0; posY < height; posY++) {
                    for (int posX = 0; posX < (width * 8); posX++) {
                        if (data[offset] == oldColors[colorIdx] &&
                            (!useRandom || (random_source.getRandomNumber(3) == 0))) {
                            data[offset] = newColors[colorIdx];
                        }

                        offset += 1;
                    }
                }
            }
        }
    }

    void DaxBlock::MergeIcons(const DaxBlock &srcIcon) {
        for (int i = 0; i < srcIcon.bpp; i++) {
            uint8 a = data[i];
            uint8 b = srcIcon.data[i];

            if (a == 16 && b == 16) {
                data[i] = 16;
            } else if (a == 16) {
                data[i] = b;
            } else if (b == 16) {
                data[i] = a;
            } else {
                data[i] = (a | b);
            }
        }
    }

    void DaxBlock::DaxToPicture(int mask_color, int masked, Common::ReadStream &stream) {
        int dest_offset = 0;

        for (int loop1_var = 1; loop1_var <= item_count; loop1_var++) {
            for (int loop2_var = 0; loop2_var < height; loop2_var++) {
                for (int loop3_var = 0; loop3_var < (width * 4); loop3_var++) {
                    uint8 c = stream.readByte();

                    SetMaskedColor(dest_offset, c >> 4, masked, mask_color);

                    dest_offset += 1;

                    SetMaskedColor(dest_offset, c & 0x0F, masked, mask_color);

                    dest_offset += 1;
                }
            }
        }
    }

    void DaxBlock::SetMaskedColor(int offset, int color, int masked, int mask_color) {
        if (masked == 1 && color == mask_color) {
            data[offset] = 16;
        } else {
            data[offset] = static_cast<uint8>(color);
        }
    }

} // namespace GoldBox
