#include "common/system.h"
#include "common/debug.h"
#include "graphics/palette.h"
#include "graphics/paletteman.h"
#include "graphics/screen.h"
#include "goldbox/gfx/gfx.h"

namespace GoldBox {
namespace Gfx {

byte EGA_INDEXES[EGA_PALETTE_COUNT];

void GFX::setEgaPalette() {
	Graphics::Palette ega = Graphics::Palette::createEGAPalette();
	g_system->getPaletteManager()->setPalette(ega);

	uint32 c = 0xffffffff;
	g_system->getPaletteManager()->setPalette((const byte *)&c, 255, 1);

	// Set the EGA palette indexes to be themselves
	for (int i = 0; i < EGA_PALETTE_COUNT; ++i)
		EGA_INDEXES[i] = i;
}

} // namespace Gfx
} // namespace GoldBox
