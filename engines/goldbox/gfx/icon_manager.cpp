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

} // namespace Gfx
} // namespace Goldbox
