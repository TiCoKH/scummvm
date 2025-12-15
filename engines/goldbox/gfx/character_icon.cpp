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

#include "goldbox/gfx/character_icon.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Gfx {

// CharacterIcon member functions
CharacterIcon::CharacterIcon(const Data::CombatIconData &iconData,
                             IconActionState actionState)
    : _size(static_cast<IconSize>(iconData.iconSize)),
      _actionState(actionState),
      _composite(nullptr),
      _readyIcon(nullptr),
      _actionIcon(nullptr),
      _readyIconFlipped(nullptr),
      _actionIconFlipped(nullptr) {

	uint8 sizeOffset = (_size == ICON_SIZE_LARGE) ? 64 : 0;
	uint8 readyHeadId = iconData.iconHead + sizeOffset;
	uint8 readyBodyId = iconData.iconBody + sizeOffset;
	uint8 actionHeadId = readyHeadId + 128;
	uint8 actionBodyId = readyBodyId + 128;

	// Build ready state icon (normal)
	_readyIcon = buildComposite(readyHeadId, readyBodyId, iconData);

	// Build action state icon (normal)
	_actionIcon = buildComposite(actionHeadId, actionBodyId, iconData);

	// Build vertically flipped versions
	_readyIconFlipped = createFlipped(_readyIcon);
	_actionIconFlipped = createFlipped(_actionIcon);

	// Set current composite based on action state
	_composite = (actionState == ICON_ACTION_READY) ? _readyIcon : _actionIcon;
}

CharacterIcon::~CharacterIcon() {
	delete _readyIcon;
	delete _actionIcon;
	delete _readyIconFlipped;
	delete _actionIconFlipped;
	// _composite is not deleted here as it points to either _readyIcon or _actionIcon
	_composite = nullptr;
}

Pic *CharacterIcon::buildComposite(uint8 headId, uint8 bodyId,
								   const Data::CombatIconData &iconData) {
	// Load head and body sprites from DAX containers via VmInterface
	Data::DaxBlockPic *headBlock = static_cast<Data::DaxBlockPic *>(VmInterface::getDaxCHead().getBlockById(headId));
	Data::DaxBlockPic *bodyBlock = static_cast<Data::DaxBlockPic *>(VmInterface::getDaxCBody().getBlockById(bodyId));

	if (!headBlock || !bodyBlock) {
		// Create empty placeholder if sprites missing
		Pic *placeholder = new Pic(24, 24);
		placeholder->clear(0);
		return placeholder;
	}

	Pic *bodyPic = Pic::read(bodyBlock);
	Pic *headPic = Pic::read(headBlock);

	// Body is always full 24x24 tile, use it as the composite base
	// Head varies (10x24 or 8x24) and overlays on top from position 0,0
	Pic *composite = new Pic(bodyPic->w, bodyPic->h);

	// Apply color remapping to body parts
	// Body colors: iconColorBody1 (primary), iconColorBody2 (secondary)
	uint8 bodyColors[6] = {
		iconData.iconColorBody1, iconData.iconColorBody2,
		iconData.iconColorArm1, iconData.iconColorArm2,
		iconData.iconColorLeg1, iconData.iconColorLeg2
	};
	applyColorRemapping(bodyPic, bodyColors, 6);

	// Apply color remapping to head
	uint8 headColors[2] = {
		iconData.iconColorHair,
		iconData.iconColorFace
	};
	applyColorRemapping(headPic, headColors, 2);

	// Copy body first (full 24x24 base)
	composite->blitFrom(*bodyPic);

	// Overlay head on top from position 0,0 with transparency
	headPic->trDraw(composite, 0, 0, 0); // Color index 0 is transparent

	// Clean up temporary surfaces
	delete headPic;
	delete bodyPic;

	return composite;
}

Pic *CharacterIcon::createFlipped(const Pic *source) const {
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

void CharacterIcon::applyColorRemapping(Pic *layer, const uint8 *colorMap, uint8 colorCount) const {
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

void CharacterIcon::draw(Graphics::ManagedSurface *dst, int x, int y) const {
	if (_composite) {
		_composite->draw(dst, x, y);
	}
}

void CharacterIcon::drawAtCharPos(Graphics::ManagedSurface *dst, int charX, int charY) const {
	if (_composite) {
		_composite->drawAtCharPos(dst, charX, charY);
	}
}

int CharacterIcon::getWidth() const {
	return _composite ? _composite->w : 0;
}

int CharacterIcon::getHeight() const {
	return _composite ? _composite->h : 0;
}

// Static utility functions
uint8 CharacterIcon::calculateSpriteId(uint8 baseId, uint8 size, bool isAction) {
	uint8 sizeOffset = (size == SIZE_LARGE) ? SIZE_OFFSET_LARGE : 0;
	uint8 actionOffset = isAction ? ACTION_OFFSET : 0;
	return baseId + sizeOffset + actionOffset;
}

uint8 CharacterIcon::extractBaseId(uint8 spriteId) {
	if (spriteId >= ACTION_OFFSET) {
		spriteId -= ACTION_OFFSET;
	}
	if (spriteId >= SIZE_OFFSET_LARGE) {
		spriteId -= SIZE_OFFSET_LARGE;
	}
	return spriteId;
}

bool CharacterIcon::isValidHeadBaseId(uint8 baseId) {
	return baseId >= SMALL_HEAD_MIN && baseId <= SMALL_HEAD_MAX;
}

bool CharacterIcon::isValidBodyBaseId(uint8 baseId) {
	return baseId >= SMALL_BODY_MIN && baseId <= SMALL_BODY_MAX;
}

uint8 CharacterIcon::nextHead(uint8 currentBaseId) {
	return (currentBaseId >= SMALL_HEAD_MAX) ? SMALL_HEAD_MIN : currentBaseId + 1;
}

uint8 CharacterIcon::prevHead(uint8 currentBaseId) {
	return (currentBaseId == SMALL_HEAD_MIN) ? SMALL_HEAD_MAX : currentBaseId - 1;
}

uint8 CharacterIcon::nextBody(uint8 currentBaseId) {
	return (currentBaseId >= SMALL_BODY_MAX) ? SMALL_BODY_MIN : currentBaseId + 1;
}

uint8 CharacterIcon::prevBody(uint8 currentBaseId) {
	return (currentBaseId == SMALL_BODY_MIN) ? SMALL_BODY_MAX : currentBaseId - 1;
}

uint8 CharacterIcon::nextColor(uint8 currentColor) {
	return (currentColor + 1) & 0x0F;
}

uint8 CharacterIcon::prevColor(uint8 currentColor) {
	return (currentColor == 0) ? COLOR_MAX : currentColor - 1;
}

void CharacterIcon::getDimensions(uint8 size, int &width, int &height) {
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
