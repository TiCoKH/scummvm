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
        uint8 id;
        uint32 offset;
        uint16 rawSize; // decodeSize
        uint16 compSize; // dataLength

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

class DaxBlock8x8D : public DaxBlock {
public:
    int height;
    int width;
    int item_count;

    DaxBlock8x8D();

    void adjust() override;
};

} // namespace Goldbox
#endif // GOLDBOX_DAXBLOCK_H
