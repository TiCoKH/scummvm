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

#ifndef GOLDBOX_DATA_COMBAT_ICON_DATA_H
#define GOLDBOX_DATA_COMBAT_ICON_DATA_H

#include "common/scummsys.h"

namespace Goldbox {
namespace Data {

/**
 * Immutable snapshot DTO describing the visual appearance of a combat icon.
 *
 * This struct is the boundary type between the gameplay data layer (Data) and
 * the rendering layer (Gfx).  Renderers must only read it — never mutate it.
 *
 * Stored as part of PlayerCharacter (savegame state). Passed by const & into
 * every gfx API.  The gfx layer MUST NOT hold a non-const pointer or reference
 * to any PlayerCharacter member; take a value-copy if the renderer needs to
 * keep the data past the call site.
 */
struct CombatIconData {
	uint8 iconHead;
	uint8 iconBody;
	uint8 iconSize;
	uint8 iconSlotId;
	uint8 iconColorBody1, iconColorBody2;
	uint8 iconColorArm1,  iconColorArm2;
	uint8 iconColorLeg1,  iconColorLeg2;
	uint8 iconColorHair,  iconColorFace;
	uint8 iconColorShield1, iconColorShield2;
	uint8 iconColorWeapon1, iconColorWeapon2;

	CombatIconData()
	    : iconHead(0), iconBody(0), iconSize(0), iconSlotId(0),
	      iconColorBody1(0), iconColorBody2(0),
	      iconColorArm1(0),  iconColorArm2(0),
	      iconColorLeg1(0),  iconColorLeg2(0),
	      iconColorHair(0),  iconColorFace(0),
	      iconColorShield1(0), iconColorShield2(0),
	      iconColorWeapon1(0), iconColorWeapon2(0) {}

	// --- Convenience setters used by character-creation/equipment code ---

	void setBodyColor(uint8 value) {
		iconColorBody1 = value & 0x0F;
		iconColorBody2 = value >> 4;
	}

	void setArmColor(uint8 value) {
		iconColorArm1 = value & 0x0F;
		iconColorArm2 = value >> 4;
	}

	void setLegColor(uint8 value) {
		iconColorLeg1 = value & 0x0F;
		iconColorLeg2 = value >> 4;
	}

	void setHairFaceColor(uint8 value) {
		iconColorHair = value & 0x0F;
		iconColorFace = value >> 4;
	}

	void setShieldColor(uint8 value) {
		iconColorShield1 = value & 0x0F;
		iconColorShield2 = value >> 4;
	}

	void setWeaponColor(uint8 value) {
		iconColorWeapon1 = value & 0x0F;
		iconColorWeapon2 = value >> 4;
	}

	bool operator==(const CombatIconData &o) const {
		return iconHead         == o.iconHead         &&
		       iconBody         == o.iconBody         &&
		       iconSize         == o.iconSize         &&
		       iconSlotId       == o.iconSlotId       &&
		       iconColorBody1   == o.iconColorBody1   &&
		       iconColorBody2   == o.iconColorBody2   &&
		       iconColorArm1    == o.iconColorArm1    &&
		       iconColorArm2    == o.iconColorArm2    &&
		       iconColorLeg1    == o.iconColorLeg1    &&
		       iconColorLeg2    == o.iconColorLeg2    &&
		       iconColorHair    == o.iconColorHair    &&
		       iconColorFace    == o.iconColorFace    &&
		       iconColorShield1 == o.iconColorShield1 &&
		       iconColorShield2 == o.iconColorShield2 &&
		       iconColorWeapon1 == o.iconColorWeapon1 &&
		       iconColorWeapon2 == o.iconColorWeapon2;
	}

	bool operator!=(const CombatIconData &o) const { return !(*this == o); }

	// Validation constants
	static const uint8 ICON_HEAD_MAX = 13;
	static const uint8 ICON_BODY_MAX = 31;
};

} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_COMBAT_ICON_DATA_H
