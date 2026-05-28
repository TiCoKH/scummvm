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

#include "goldbox/gfx/icon_manager.h"
#include "goldbox/gfx/icon.h"

namespace Goldbox {
namespace Gfx {

IconManager::IconManager() {
	_iconBuffer.resize(SLOT_COUNT);
	_slotStates.resize(SLOT_COUNT);

	for (uint i = 0; i < SLOT_COUNT; ++i) {
		_iconBuffer[i] = nullptr;
		_slotStates[i] = SLOT_EMPTY;
	}
}

IconManager::~IconManager() {
	clear();
}

Icon *IconManager::getIcon(uint8 slotId) {
	if (slotId >= SLOT_COUNT) {
		return nullptr;
	}
	return _iconBuffer[slotId];
}

const Pic *IconManager::getPic(uint8 slotId) const {
	if (slotId >= SLOT_COUNT) {
		return nullptr;
	}
	Icon *icon = _iconBuffer[slotId];
	return icon ? icon->getReadyIcon() : nullptr;
}

const Pic *IconManager::getReadyPic(uint8 slotId) const {
	if (slotId >= SLOT_COUNT) {
		return nullptr;
	}
	Icon *icon = _iconBuffer[slotId];
	if (icon)
		return icon->getReadyIcon();
	return nullptr;
}

const Pic *IconManager::getActionPic(uint8 slotId) const {
	if (slotId >= SLOT_COUNT) {
		return nullptr;
	}
	Icon *icon = _iconBuffer[slotId];
	if (icon)
		return icon->getActionIcon();
	return nullptr;
}

void IconManager::setIcon(uint8 slotId, Icon *icon) {
	if (slotId >= SLOT_COUNT) {
		return;
	}

	// Release existing icon if present
	if (_iconBuffer[slotId]) {
		delete _iconBuffer[slotId];
	}

	_iconBuffer[slotId] = icon;
	_slotStates[slotId] = (icon != nullptr) ? SLOT_READY : SLOT_EMPTY;
}

void IconManager::releaseIcon(uint8 slotId) {
	if (slotId >= SLOT_COUNT) {
		return;
	}

	if (_iconBuffer[slotId]) {
		delete _iconBuffer[slotId];
		_iconBuffer[slotId] = nullptr;
	}
	_slotStates[slotId] = SLOT_EMPTY;
}

SlotState IconManager::getSlotState(uint8 slotId) const {
	if (slotId >= SLOT_COUNT) {
		return SLOT_EMPTY;
	}
	return _slotStates[slotId];
}

bool IconManager::isSlotEmpty(uint8 slotId) const {
	if (slotId >= SLOT_COUNT) {
		return true;
	}
	return _slotStates[slotId] == SLOT_EMPTY;
}

void IconManager::clear() {
	for (uint i = 0; i < SLOT_COUNT; ++i) {
		if (_iconBuffer[i]) {
			delete _iconBuffer[i];
			_iconBuffer[i] = nullptr;
		}
		_slotStates[i] = SLOT_EMPTY;
	}
}

bool IconManager::loadIcon(uint8 slotId, IconKind kind, uint16 blockId) {
	if (slotId >= SLOT_COUNT) {
		warning("IconManager::loadIcon: Invalid slotId %d (max %d)", slotId, SLOT_COUNT - 1);
		return false;
	}

	debug(8, "IconManager::loadIcon: Loading slot %d from kind=%d blockId=%d", slotId, kind, blockId);

	// Use Icon constructor that loads both ready and action blocks
	Icon *icon = new Icon(blockId, kind);

	if (!icon) {
		warning("IconManager::loadIcon: Failed to create Icon for slot %d", slotId);
		return false;
	}

	// Cleanup previous slot contents
	releaseIcon(slotId);

	_iconBuffer[slotId] = icon;
	_slotStates[slotId] = SLOT_READY;

	debug(8, "  - Icon loaded into slot %d, state=READY", slotId);
	return true;
}

bool IconManager::loadIcon(uint8 slotId, const CombatIconData &iconData, bool isAction) {
	if (slotId >= SLOT_COUNT) {
		return false;
	}

	// Build composite icon via Icon and store in manager
	Icon *icon = new Icon(iconData, isAction ? ICON_STATE_ATTACK : ICON_STATE_READY);

	// Replace existing icon if present
	if (_iconBuffer[slotId]) {
		delete _iconBuffer[slotId];
	}
	_iconBuffer[slotId] = icon;
	_slotStates[slotId] = SLOT_READY;

	return true;
}

bool IconManager::drawAtPos(Graphics::ManagedSurface *dst, int iconX, int iconY, uint8 /*reserved*/, uint8 frame, uint8 slotId) {
	if (!dst) {
		warning("IconManager::drawAtPos: Destination surface is NULL");
		return false;
	}
	if (slotId >= SLOT_COUNT) {
		warning("IconManager::drawAtPos: Invalid slotId %d (max %d)", slotId, SLOT_COUNT - 1);
		return false;
	}

	debug(8, "IconManager::drawAtPos: x=%d, y=%d, frame=%d, slot=%d", iconX, iconY, frame, slotId);

	Icon *icon = _iconBuffer[slotId];
	if (!icon) {
		warning("IconManager::drawAtPos: Slot %d is empty (state=%d)", slotId, _slotStates[slotId]);
		return false;
	}
	debug(8, "  - Icon found in slot %d", slotId);

	// Update slot state to indicate use
	_slotStates[slotId] = SLOT_IN_USE;

	// Set desired action state
	debug(8, "  - Setting action state: %s", frame ? "ATTACK" : "READY");
	icon->setActionState(frame ? ICON_STATE_ATTACK : ICON_STATE_READY);

	// Draw using icon-grid coordinates (Icon expects x, y order)
	debug(8, "  - Drawing icon at grid position (x=%d, y=%d)", iconX, iconY);
	icon->drawAtIconPos(dst, iconX, iconY);
	debug(8, "  - Icon drawn successfully");

	// Restore state to ready after rendering
	_slotStates[slotId] = SLOT_READY;
	return true;
}

} // namespace Gfx
} // namespace Goldbox
