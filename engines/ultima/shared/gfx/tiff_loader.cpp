/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "ultima/shared/gfx/tiff_loader.h"
#include "ultima/shared/gfx/tiff.h"
#include "ultima/shared/core/file.h"
#include "common/util.h"
#include "common/memstream.h"

namespace Ultima {
namespace Shared {
namespace Gfx {

bool TIFFLoader::loadTIFFImage(const Common::String &filepath, Graphics::Surface *dest) {
	assert(dest);
	Shared::File source(filepath);
	Image::Ultima::Shared::Gfx::TIFFDecoder tiff;
	if (!tiff.loadStream(source)) {
		error("Error while reading TIFF image");
		return false;
	}
	const Graphics::Surface *sourceSurface = tiff.getSurface();
	dest->create(sourceSurface->h, sourceSurface->w, sourceSurface->format);
	dest->copyFrom(*sourceSurface);
	return true;
}

} // End of namespace Gfx 
} // End of namespace Shared
} // End of namespace Ultima 

