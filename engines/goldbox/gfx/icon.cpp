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

#include "goldbox/gfx/icon.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Gfx {

// Icon member functions
Icon::Icon(const Data::CombatIconData &iconData,
                             IconActionState actionState)
    : _size(static_cast<IconSize>(iconData.iconSize)),
      _actionState(actionState),
      _composite(nullptr),
      _readyIcon(nullptr),
      _actionIcon(nullptr),
      _readyIconFlipped(nullptr),
      _actionIconFlipped(nullptr),
      _readyHeadPic(nullptr),
      _readyBodyPic(nullptr),
      _actionHeadPic(nullptr),
      _actionBodyPic(nullptr) {

	uint8 sizeOffset = (_size == ICON_SIZE_LARGE) ? 64 : 0;
	uint8 readyHeadId = iconData.iconHead + sizeOffset;
	uint8 readyBodyId = iconData.iconBody + sizeOffset;
	uint8 actionHeadId = readyHeadId + 128;
	uint8 actionBodyId = readyBodyId + 128;

	// Load base sprites from DAX containers (cached)
	if (!loadBaseSprites(readyHeadId, readyBodyId, actionHeadId, actionBodyId)) {
		// Fallback: create empty placeholder if sprites missing
		_readyIcon = new Pic(24, 24);
		_readyIcon->clear(0);
		_actionIcon = new Pic(24, 24);
		_actionIcon->clear(0);
	} else {
		// Build composite icons using cached base sprites
		_readyIcon = buildComposite(false, iconData);
		_actionIcon = buildComposite(true, iconData);
	}

	// Build vertically flipped versions
	_readyIconFlipped = createFlipped(_readyIcon);
	_actionIconFlipped = createFlipped(_actionIcon);

	// Set current composite based on action state
	_composite = (actionState == ICON_ACTION_READY) ? _readyIcon : _actionIcon;
}

Icon::~Icon() {
	delete _readyIcon;
	delete _actionIcon;
	delete _readyIconFlipped;
	delete _actionIconFlipped;
	// _composite is not deleted here as it points to either _readyIcon or _actionIcon
	_composite = nullptr;

	// Clean up cached base sprites
	delete _readyHeadPic;
	delete _readyBodyPic;
	delete _actionHeadPic;
	delete _actionBodyPic;
}

bool Icon::loadBaseSprites(uint8 readyHeadId, uint8 readyBodyId, 
                                    uint8 actionHeadId, uint8 actionBodyId) {
	// Load ready state sprites from DAX containers
	Data::DaxBlockPic *readyHeadBlock = static_cast<Data::DaxBlockPic *>(
		VmInterface::getDaxCHead().getBlockById(readyHeadId));
	Data::DaxBlockPic *readyBodyBlock = static_cast<Data::DaxBlockPic *>(
		VmInterface::getDaxCBody().getBlockById(readyBodyId));

	if (!readyHeadBlock || !readyBodyBlock) {
		return false;
	}

	// Load action state sprites from DAX containers
	Data::DaxBlockPic *actionHeadBlock = static_cast<Data::DaxBlockPic *>(
		VmInterface::getDaxCHead().getBlockById(actionHeadId));
	Data::DaxBlockPic *actionBodyBlock = static_cast<Data::DaxBlockPic *>(
		VmInterface::getDaxCBody().getBlockById(actionBodyId));

	if (!actionHeadBlock || !actionBodyBlock) {
		return false;
	}

	// Cache the base Pics (pre-made from DAX)
	_readyHeadPic = Pic::read(readyHeadBlock);
	_readyBodyPic = Pic::read(readyBodyBlock);
	_actionHeadPic = Pic::read(actionHeadBlock);
	_actionBodyPic = Pic::read(actionBodyBlock);

	return true;
}

Pic *Icon::buildComposite(bool isAction, const Data::CombatIconData &iconData) {
	// Use cached base sprites (already loaded from DAX)
	Pic *headPic = isAction ? _actionHeadPic : _readyHeadPic;
	Pic *bodyPic = isAction ? _actionBodyPic : _readyBodyPic;

	if (!headPic || !bodyPic) {
		// Create empty placeholder if sprites missing
		Pic *placeholder = new Pic(24, 24);
		placeholder->clear(0);
		return placeholder;
	}

	// Create working copies for color remapping (don't modify cached originals)
	Pic *bodyPicCopy = new Pic(bodyPic->w, bodyPic->h);
	bodyPicCopy->blitFrom(*bodyPic);

	Pic *headPicCopy = new Pic(headPic->w, headPic->h);
	headPicCopy->blitFrom(*headPic);

	// Body is always full 24x24 tile, use it as the composite base
	Pic *composite = new Pic(bodyPicCopy->w, bodyPicCopy->h);

	// Apply color remapping to body parts
	// Body colors: iconColorBody1 (primary), iconColorBody2 (secondary)
	uint8 bodyColors[6] = {
		iconData.iconColorBody1, iconData.iconColorBody2,
		iconData.iconColorArm1, iconData.iconColorArm2,
		iconData.iconColorLeg1, iconData.iconColorLeg2
	};
	applyColorRemapping(bodyPicCopy, bodyColors, 6);

	// Apply color remapping to head
	uint8 headColors[2] = {
		iconData.iconColorHair,
		iconData.iconColorFace
	};
	applyColorRemapping(headPicCopy, headColors, 2);

	// Copy body first (full 24x24 base)
	composite->blitFrom(*bodyPicCopy);

	// Overlay head on top from position 0,0 with transparency
	headPicCopy->trDraw(composite, 0, 0, 0); // Color index 0 is transparent

	// Clean up temporary copies (keep cached originals)
	delete headPicCopy;
	delete bodyPicCopy;

	return composite;
}

Pic *Icon::createFlipped(const Pic *source) const {
	if (!source) {
		return nullptr;
	}

	Pic *flipped = new Pic(source->w, source->h);

	// Copy pixels in reverse row order (vertical flip)
	for (int y = 0; y < source->h; ++y) {
		const byte *srcRow = (const byte *)source->getBasePtr(0, source->h - 1 - y);
		byte *dstRow = (byte *)flipped->getBasePtr(0, y);
		Common::copy(srcRow, srcRow + source->w, dstRow);
	}

	return flipped;
}

void Icon::applyColorRemapping(Pic *layer, const uint8 *colorMap, uint8 colorCount) const {
	if (!layer || !colorMap) {
		return;
	}

	// Create a color lookup table
	// This maps palette indices used in the sprite to the character's custom colors
	// The original sprites use placeholder colors (e.g., 1-6) that get remapped
	// to the character's chosen palette indices

	byte *pixels = (byte *)layer->getPixels();
	int pixelCount = layer->w * layer->h;

	for (int i = 0; i < pixelCount; ++i) {
		byte pixel = pixels[i];

		// Remap colors 1-N to the character's color choices
		// Color 0 remains transparent, colors beyond the map range are unchanged
		if (pixel > 0 && pixel <= colorCount) {
			pixels[i] = colorMap[pixel - 1];
		}
	}
}

void Icon::draw(Graphics::ManagedSurface *dst, int x, int y) const {
	if (_composite) {
		_composite->draw(dst, x, y);
	}
}

void Icon::drawAtCharPos(Graphics::ManagedSurface *dst, int charX, int charY) const {
	if (_composite) {
		_composite->drawAtCharPos(dst, charX, charY);
	}
}

int Icon::getWidth() const {
	return _composite ? _composite->w : 0;
}

int Icon::getHeight() const {
	return _composite ? _composite->h : 0;
}

// Static utility functions
uint8 Icon::calculateSpriteId(uint8 baseId, uint8 size, bool isAction) {
	uint8 sizeOffset = (size == SIZE_LARGE) ? SIZE_OFFSET_LARGE : 0;
	uint8 actionOffset = isAction ? ACTION_OFFSET : 0;
	return baseId + sizeOffset + actionOffset;
}

uint8 Icon::extractBaseId(uint8 spriteId) {
	if (spriteId >= ACTION_OFFSET) {
		spriteId -= ACTION_OFFSET;
	}
	if (spriteId >= SIZE_OFFSET_LARGE) {
		spriteId -= SIZE_OFFSET_LARGE;
	}
	return spriteId;
}

bool Icon::isValidHeadBaseId(uint8 baseId) {
	return baseId >= SMALL_HEAD_MIN && baseId <= SMALL_HEAD_MAX;
}

bool Icon::isValidBodyBaseId(uint8 baseId) {
	return baseId >= SMALL_BODY_MIN && baseId <= SMALL_BODY_MAX;
}

uint8 Icon::nextHead(uint8 currentBaseId) {
	return (currentBaseId >= SMALL_HEAD_MAX) ? SMALL_HEAD_MIN : currentBaseId + 1;
}

uint8 Icon::prevHead(uint8 currentBaseId) {
	return (currentBaseId == SMALL_HEAD_MIN) ? SMALL_HEAD_MAX : currentBaseId - 1;
}

uint8 Icon::nextBody(uint8 currentBaseId) {
	return (currentBaseId >= SMALL_BODY_MAX) ? SMALL_BODY_MIN : currentBaseId + 1;
}

uint8 Icon::prevBody(uint8 currentBaseId) {
	return (currentBaseId == SMALL_BODY_MIN) ? SMALL_BODY_MAX : currentBaseId - 1;
}

uint8 Icon::nextColor(uint8 currentColor) {
	return (currentColor + 1) & 0x0F;
}

uint8 Icon::prevColor(uint8 currentColor) {
	return (currentColor == 0) ? COLOR_MAX : currentColor - 1;
}

void Icon::getDimensions(uint8 size, int &width, int &height) {
	if (size == SIZE_SMALL) {
		width = SMALL_ICON_WIDTH;
		height = SMALL_ICON_HEIGHT;
	} else {
		width = LARGE_ICON_WIDTH;
		height = LARGE_ICON_HEIGHT;
	}
}

} // namespace Gfx
} // namespace Goldbox
