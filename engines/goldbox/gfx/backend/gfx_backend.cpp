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

#include "goldbox/gfx/backend/gfx_backend.h"
#include "goldbox/engine.h"
#include "goldbox/gfx/backend/amiga_gfx_backend.h"
#include "goldbox/gfx/backend/dos_gfx_backend.h"
#include "goldbox/gfx/backend/pc98_gfx_backend.h"

namespace Goldbox {
namespace Gfx {

GfxBackend *GfxBackend::create(Common::Platform platform) {
	switch (platform) {
	case Common::kPlatformAmiga:
		return new AmigaGfxBackend();
	case Common::kPlatformPC98:
		return new Pc98GfxBackend();
	default:
		return new DosGfxBackend();
	}
}

uint8 combatBackgroundColor() {
	return g_engine->getGfx().artColors().combatBackground;
}

} // namespace Gfx
} // namespace Goldbox
