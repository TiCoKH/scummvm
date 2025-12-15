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

#include "goldbox/poolrad/views/dialogs/portrait_display.h"
#include "goldbox/vm_interface.h"
#include "goldbox/gfx/surface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

PortraitDisplay::PortraitDisplay()
    : _headPic(nullptr), _bodyPic(nullptr), _windowLeft(0), _windowTop(0),
      _windowRight(0), _windowBottom(0), _hasWindow(false) {
}

PortraitDisplay::~PortraitDisplay() {
}

bool PortraitDisplay::loadHead(uint8 daxBlockId) {
    // Clean up old head picture
    if (_headPic) {
        delete _headPic;
        _headPic = nullptr;
    }

    // Load and assign new head portrait
    _headPic = _loadHeadFromDaxBlock(daxBlockId);
    return _headPic != nullptr;
}

bool PortraitDisplay::loadBody(uint8 daxBlockId) {
    // Clean up old body picture
    if (_bodyPic) {
        delete _bodyPic;
        _bodyPic = nullptr;
    }

    // Load and assign new body portrait
    _bodyPic = _loadBodyFromDaxBlock(daxBlockId);
    return _bodyPic != nullptr;
}

Goldbox::Gfx::Pic *PortraitDisplay::_loadHeadFromDaxBlock(uint8 daxBlockId) {
    // Attempt to load head portrait from DAX block
    Goldbox::Data::DaxBlockContainer &headContainer = VmInterface::getDaxHead();
    Goldbox::Data::DaxBlock *daxBlock = headContainer.getBlockById(daxBlockId);
    if (daxBlock) {
        return Goldbox::Gfx::Pic::read(dynamic_cast<Goldbox::Data::DaxBlockPic*>(daxBlock));
    }
    return nullptr;
}

Goldbox::Gfx::Pic *PortraitDisplay::_loadBodyFromDaxBlock(uint8 daxBlockId) {
    // Attempt to load body portrait from DAX block
    Goldbox::Data::DaxBlockContainer &bodyContainer = VmInterface::getDaxBody();
    Goldbox::Data::DaxBlock *daxBlock = bodyContainer.getBlockById(daxBlockId);
    if (daxBlock) {
        return Goldbox::Gfx::Pic::read(dynamic_cast<Goldbox::Data::DaxBlockPic*>(daxBlock));
    }
    return nullptr;
}

void PortraitDisplay::drawHead(Graphics::ManagedSurface *dst, int x, int y) const {
    if (!dst)
        return;

    if (_headPic) {
        _headPic->drawAtCharPos(dst, x, y);
    }
}

void PortraitDisplay::drawBody(Graphics::ManagedSurface *dst, int x, int y) const {
    if (!dst)
        return;

    if (_bodyPic) {
        _bodyPic->drawAtCharPos(dst, x, y);
    }
}

void PortraitDisplay::render(Graphics::ManagedSurface *dst, int x, int y, uint32 tpColorIndex) const {
    if (!dst)
        return;

    drawHead(dst, x, y);
    drawBody(dst, x, y + 5);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
