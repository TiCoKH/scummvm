// DaxBlock.h
#ifndef GOLDBOX_DAXBLOCK_H
#define GOLDBOX_DAXBLOCK_H

#include "common/array.h"
#include "common/scummsys.h"

namespace Goldbox {

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

    struct GfxDataHeader {
        uint16 height;
        uint16 cwidth;
        uint16 frameNum; //???
        uint16 w_unk;
        uint8 elemNum;
        uint8 a_unk[8];

        // Default constructor
        GfxDataHeader() = default;

        // Copy constructor
        GfxDataHeader(const GfxDataHeader &other) = default;

        // Copy assignment operator
        GfxDataHeader& operator=(const GfxDataHeader &other) = default;
    };

    enum class ContentType {
        _8X8D, BACK, BIGPIC, BODY, CBODY, CHEAD, COMSPR, ECL, GEO, HEAD, MONCHA, PIC, SPRIT, TITLE, WALLDEF, UNKNOWN
    };

class DaxBlock {
public:
    virtual ~DaxBlock() = default; // Virtual destructor

    virtual void adjust(); // Pure virtual function

    // Common properties
    int8 blockId;
    Common::Array<uint8> _data;

    static DaxBlock* createDaxBlock(ContentType contentType);

};

class DaxBlockTitle : public DaxBlock {
public:
    int height;
    int width;
    int item_count;

    DaxBlockTitle();

private:
    void adjust() override;
    // Function to pop GfxDataHeader
    GfxDataHeader popGfxHeader();
};

class DaxBlock8x8D : public DaxBlock {
public:
    int height;
    int width;
    int item_count;

    DaxBlock8x8D();

private:
    void adjust() override;
    // Function to pop GfxDataHeader
    GfxDataHeader popGfxHeader();
};

} // namespace Goldbox
#endif // GOLDBOX_DAXBLOCK_H
