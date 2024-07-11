#ifndef GOLD_BOX_GFX_SCREEN_MANAGER_H
#define GOLD_BOX_GFX_SCREEN_MANAGER_H

#include "image/image_decoder.h"
#include "graphics/managed_surface.h"

namespace GoldBox {
namespace Gfx {

class ScreenManager : public Image::ImageDecoder {
private:
	Graphics::Surface _surface;
public:
	ScreenManager() {}
	~ScreenManager() override;

	void destroy() override;
	bool loadStream(Common::SeekableReadStream &stream, int16 w, int16 h);
	bool loadStream(Common::SeekableReadStream &stream) override {
		return loadStream(stream, 320, 200);
	}

	const Graphics::Surface *getSurface() const override {
		return &_surface;
	}
	const byte *getPalette() const override { return nullptr; }
	uint16 getPaletteColorCount() const override { return 0; }
	void clear() { _surface.free(); }
};

} // namespace Gfx
} // namespace GoldBox
 #endif