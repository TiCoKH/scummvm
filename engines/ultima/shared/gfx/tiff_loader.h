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

#ifndef ULTIMA_SHARED_GFX_TIFF_LOADER_H
#define ULTIMA_SHARED_GFX_TIFF_LOADER_H

#include "common/str.h"
#include "graphics/surface.h"
#include "ultima/shared/gfx/tiff.h"

namespace Ultima {
namespace Shared {
namespace Gfx {

/**
 * Class for loading TIFF files.
 *
 */
class TIFFLoader {
	
protected:
	TIFFLoader() {} // Protected constructor to prevent instances

public:
	/**
	 * Decode an image.
	 * @param[in] filepath          file name.
	 * @param[out] dest         if successful, surface will contain the image
	 *                          data (storage is allocated via create).
	 * @return false in case of an error
	 *
	 * @remark This function does not free the image buffer passed to it,
	 *         it is the callers responsibility to do so.
	 */

	static bool loadTIFFImage(const Common::String &filepath, Graphics::Surface *dest);
};
/** @} */
} // End of namespace Gfx 
} // End of namespace Shared
} // End of namespace Ultima 

#endif
