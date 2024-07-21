// dax_symbol.h

#ifndef GOLDBOX_GFX_DAX_SYMBOL_H
#define GOLDBOX_GFX_DAX_SYMBOL_H

#include "goldbox/gfx/dax_tile.h"
#include "goldbox/gfx/surface.h"

namespace Goldbox {
namespace Gfx {

class DaxSymbol : public DaxTile {
public:
    // Define constants for specific symbols
    static const uint8 upArrow = 1;
    static const uint8 rightArrow = 2;
    static const uint8 downArrow = 3;
    static const uint8 leftArrow = 4;
    static const uint8 cornerSymbol = 21;
    static const uint8 verticalSymbol = 22;
    static const uint8 horizontalSymbol = 23;

    DaxSymbol() : DaxTile() {}
    DaxSymbol(DaxBlock8x8D *daxBlock) : DaxTile(daxBlock) {}
    ~DaxSymbol() override {}

};

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_DAX_SYMBOL_H
