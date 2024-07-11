// DaxBlock.h
#ifndef GOLD_BOX_DAXBLOCK_H
#define GOLD_BOX_DAXBLOCK_H

#include "common/array.h"
#include "common/scummsys.h"

namespace GoldBox {

    struct DaxHeader {
        uint8 id;
        uint32 offset;
        uint16 rawSize; // decodeSize
        uint16 compSize; // dataLength

        // Default constructor
        DaxHeader() = default;

        // Copy constructor
        DaxHeader(const DaxHeader &other) = default;

        // Copy assignment operator
        DaxHeader& operator=(const DaxHeader &other) = default;
    };

    enum class ContentType {
        _8X8D, BACK, BIGPIC, BODY, CBODY, CHEAD, COMSPR, ECL, GEO, HEAD, MONCHA, PIC, SPRIT, TITLE, WALLDEF, UNKNOWN
    };

class DaxBlock {
public:
    virtual ~DaxBlock() = default; // Virtual destructor

    virtual void process() const = 0; // Pure virtual function

    // Common properties
    int8 blockId;
    Common::Array<uint8> data;
    ContentType type;

    static DaxBlock* createDaxBlock(ContentType contentType);

};

class PictureBlock {
public:
    //Picture type block properties
    int height;
    int width;
    int item_count;

    PictureBlock(Common::Array<uint8>& dataRef);
    virtual ~PictureBlock() = default; // Virtual destructor

    void getPictureProperties();
protected:
    Common::Array<uint8>& data;
    Common::MemorySeekableReadWriteStream stream;
};

class DaxBlock8x8D : public DaxBlock, public PictureBlock {
public:
    int x_pos;
    int y_pos;
    Common::Array<uint8> field_9;
    int bpp;

    DaxBlock8x8D();

    void process() const override;

    void FlipIconLeftToRight();
    void MergeIcons(const DaxBlock8x8D &srcIcon);

private:
    void DaxToPicture(int mask_color, int masked);
    void SetMaskedColor(int offset, int color, int masked, int mask_color);
// Specific properties and methods for _8X8D
};

class DaxBlockBacPac : public DaxBlock {
public:
    int x_pos;
    int y_pos;
    Common::Array<uint8> field_9;
    int bpp;

    DaxBlockBacPac();

    void process() const override;

private:
    void DaxToPicture(int mask_color, int masked);
    void SetMaskedColor(int offset, int color, int masked, int mask_color);
};

class DaxBlockBody : public DaxBlock {
public:
    int x_pos;
    int y_pos;
    Common::Array<uint8> field_9;
    int bpp;

    DaxBlockBody();

    void process() const override;

private:
    void DaxToPicture(int mask_color, int masked);
    void SetMaskedColor(int offset, int color, int masked, int mask_color);
};

class DaxBlockCBody : public DaxBlock {
public:
    int x_pos;
    int y_pos;
    Common::Array<uint8> field_9;
    int bpp;

    DaxBlockCBody();

    void process() const override;

private:
    void DaxToPicture(int mask_color, int masked);
    void SetMaskedColor(int offset, int color, int masked, int mask_color);
};

class DaxBlockCHead : public DaxBlock {
public:
    int x_pos;
    int y_pos;
    Common::Array<uint8> field_9;
    int bpp;

    DaxBlockCHead();

    void process() const override;

private:
    void DaxToPicture(int mask_color, int masked);
    void SetMaskedColor(int offset, int color, int masked, int mask_color);
};

class DaxBlockCHead : public DaxBlock {
public:
    int x_pos;
    int y_pos;
    Common::Array<uint8> field_9;
    int bpp;

    DaxBlockCHead();

    void process() const override;

private:
    void DaxToPicture(int mask_color, int masked);
    void SetMaskedColor(int offset, int color, int masked, int mask_color);
};

} // namespace GoldBox
#endif // GOLD_BOX_DAXBLOCK_H
