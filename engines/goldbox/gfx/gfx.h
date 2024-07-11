#ifndef GOLD_BOX_GFX_H
#define GOLD_BOX_GFX_H

#include "common/array.h"
#include "common/stream.h"
#include "graphics/palette.h"

namespace GoldBox {
namespace Gfx {
#define EGA_PALETTE_COUNT 16

extern byte EGA_INDEXES[EGA_PALETTE_COUNT];


class GFX {
public:
	/**
	 * Sets the EGA palette
	 */
	static void setEgaPalette();
}

} // namespace Gfx
} // namespace GoldBox

#endif // GOLD_BOX_GFX_H
