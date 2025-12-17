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
 */

#include "goldbox/gfx/dax_renderer.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Gfx {

Pic *DaxRenderer::readHead(uint8 spriteId) {
    Data::DaxBlockPic *block = static_cast<Data::DaxBlockPic *>(
        VmInterface::getDaxCHead().getBlockById(spriteId));
    if (!block) {
        return nullptr;
    }
    return Pic::read(block);
}

Pic *DaxRenderer::readBody(uint8 spriteId) {
    Data::DaxBlockPic *block = static_cast<Data::DaxBlockPic *>(
        VmInterface::getDaxCBody().getBlockById(spriteId));
    if (!block) {
        return nullptr;
    }
    return Pic::read(block);
}

} // namespace Gfx
} // namespace Goldbox
