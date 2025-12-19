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

#include "goldbox/core/icon_manager.h"
#include "goldbox/gfx/icon.h"
#include "goldbox/vm_interface.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {

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

Gfx::Icon *IconManager::getIcon(uint8 slotId) {
	if (slotId >= SLOT_COUNT) {
		return nullptr;
	}
	return _iconBuffer[slotId];
}

const Gfx::Pic *IconManager::getPic(uint8 slotId) const {
	if (slotId >= SLOT_COUNT) {
		return nullptr;
	}
	Gfx::Icon *icon = _iconBuffer[slotId];
	return icon ? icon->getReadyIcon() : nullptr;
}

const Gfx::Pic *IconManager::getReadyPic(uint8 slotId) const {
	if (slotId >= SLOT_COUNT) {
		return nullptr;
	}
	Gfx::Icon *icon = _iconBuffer[slotId];
	if (icon)
		return icon->getReadyIcon();
	return nullptr;
}

const Gfx::Pic *IconManager::getActionPic(uint8 slotId) const {
	if (slotId >= SLOT_COUNT) {
		return nullptr;
	}
	Gfx::Icon *icon = _iconBuffer[slotId];
	if (icon)
		return icon->getActionIcon();
	return nullptr;
}

void IconManager::setIcon(uint8 slotId, Gfx::Icon *icon) {
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

	debug("IconManager::loadIcon: Loading slot %d from kind=%d blockId=%d", slotId, kind, blockId);

	// Use new Icon constructor that loads both ready and action blocks
	Gfx::Icon *icon = new Gfx::Icon(blockId, static_cast<Gfx::IconKind>(kind));

	if (!icon) {
		warning("IconManager::loadIcon: Failed to create Icon for slot %d", slotId);
		return false;
	}

	// Cleanup previous slot contents
	releaseIcon(slotId);

	_iconBuffer[slotId] = icon;
	_slotStates[slotId] = SLOT_READY;

	debug("  - Icon loaded into slot %d, state=READY", slotId);
	return true;
}


bool IconManager::loadIcon(uint8 slotId, const Data::CombatIconData &iconData, bool isAction) {
	if (slotId >= SLOT_COUNT) {
		return false;
	}

	// Build composite icon via Gfx::Icon and store in manager
	Gfx::Icon *icon = new Gfx::Icon(iconData, isAction ? Gfx::ICON_STATE_ATTACK : Gfx::ICON_STATE_READY);

	// Replace existing icon if present
	if (_iconBuffer[slotId]) {
		delete _iconBuffer[slotId];
	}
	_iconBuffer[slotId] = icon;
	_slotStates[slotId] = SLOT_READY;

	return true;
}

bool IconManager::drawAtPos(Graphics::ManagedSurface *dst, uint8 slotId, uint8 actionFlag,
			  uint8 /*reserved*/, int iconY, int iconX) {
	if (!dst) {
		warning("IconManager::drawAtPos: Destination surface is NULL");
		return false;
	}
	if (slotId >= SLOT_COUNT) {
		warning("IconManager::drawAtPos: Invalid slotId %d (max %d)", slotId, SLOT_COUNT - 1);
		return false;
	}
	
	debug("IconManager::drawAtPos: slot=%d, actionFlag=%d, y=%d, x=%d", slotId, actionFlag, iconY, iconX);
	
	Gfx::Icon *icon = _iconBuffer[slotId];
	if (!icon) {
		warning("IconManager::drawAtPos: Slot %d is empty (state=%d)", slotId, _slotStates[slotId]);
		return false;
	}
	debug("  - Icon found in slot %d", slotId);

	// Update slot state to indicate use
	_slotStates[slotId] = SLOT_IN_USE;

	// Set desired action state
	debug("  - Setting action state: %s", actionFlag ? "ATTACK" : "READY");
	icon->setActionState(actionFlag ? Gfx::ICON_STATE_ATTACK : Gfx::ICON_STATE_READY);

	// Draw using icon-grid coordinates (Icon expects x, y order)
	debug("  - Drawing icon at grid position (x=%d, y=%d)", iconX, iconY);
	icon->drawAtIconPos(dst, iconX, iconY);
	debug("  - Icon drawn successfully");

	// Restore state to ready after rendering
	_slotStates[slotId] = SLOT_READY;
	return true;
}

} // namespace Goldbox
