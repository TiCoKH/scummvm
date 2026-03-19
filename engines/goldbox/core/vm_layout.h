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

#ifndef GOLDBOX_CORE_VM_LAYOUT_H
#define GOLDBOX_CORE_VM_LAYOUT_H

#include "common/scummsys.h"
#include "goldbox/core/vm_bank.h"

namespace Goldbox {

enum VmFieldId {
	kVmFieldWildernessX = 0,
	kVmFieldWildernessY,
	kVmFieldGeoBlockId,
	kVmFieldClockUnits,
	kVmFieldClockMinuteOnes,
	kVmFieldClockMinuteTens,
	kVmFieldClockHour,
	kVmFieldClockDay,
	kVmFieldClockMonth,
	kVmFieldClockYearLo,
	kVmFieldClockYearHi,
	kVmFieldNoMagicFlag,
	kVmFieldIndoorModeFlag,
	kVmFieldWallSetPrimary,
	kVmFieldWallSetSecondary,
	kVmFieldSavedMapX,
	kVmFieldSavedMapY,
	kVmFieldSavedMapId,
	kVmFieldHideCoordsOrAutomapDisable,
	kVmFieldGameSpeed,
	kVmFieldSkyColor,
	kVmFieldCeilingColor,
	kVmFieldOptionBits,
	kVmFieldScriptFlagAA2,
	kVmFieldScriptFlagAB3,
	kVmFieldCount
};

enum VmGlobalFieldId {
	kVmGlobalFieldSelectedPcIndex = 0,
	kVmGlobalFieldCombatResult,
	kVmGlobalFieldCombatantCount,
	kVmGlobalFieldMovementBlock,
	kVmGlobalFieldSearchFlags,
	kVmGlobalFieldCombatIsAmbush,
	kVmGlobalFieldMoraleThreshold,
	kVmGlobalFieldSpokespersonChaBonus,
	kVmGlobalFieldRestSafeTime,
	kVmGlobalFieldRestInterruptChance,
	kVmGlobalFieldTriedToLeaveMap,
	kVmGlobalFieldPictureHeadId,
	kVmGlobalFieldEnterTemplePending,
	kVmGlobalFieldNoItemCombatFlag,
	kVmGlobalFieldPartyCount,
	kVmGlobalFieldDivisionModulo,
	kVmGlobalFieldShopPriceMultiplier,
	kVmGlobalFieldMonsterThac0Bonus,
	kVmGlobalFieldPartyThac0DmgBonus,
	kVmGlobalFieldPartyMoveModifier,
	kVmGlobalFieldDungeonX,
	kVmGlobalFieldDungeonY,
	kVmGlobalFieldDungeonDir,
	kVmGlobalFieldMapWallType,
	kVmGlobalFieldMapSquareInfo,
	kVmGlobalFieldMonsterDistance,
	kVmGlobalFieldShopFlag,
	kVmGlobalFieldCount
};

struct VmFieldLocation {
	VmBankId bankId;
	// Canonical VM byte address. Convert to bank byte offset with
	// VmAddressMapper: offset = vmAddr - bankBase.
	uint16 vmAddr;
};

class VmLayout {
public:
	static const uint16 kInvalidVmAddr = 0xFFFF;

	virtual ~VmLayout() {}

	virtual const char *layoutName() const = 0;
	virtual VmFieldLocation field(VmFieldId fieldId) const = 0;

	bool hasField(VmFieldId fieldId) const;

	static VmFieldLocation invalidLocation();
	static bool isValid(const VmFieldLocation &location);
};

class VmGlobalLayout {
public:
	virtual ~VmGlobalLayout() {}

	virtual const char *layoutName() const = 0;
	virtual VmFieldLocation field(VmGlobalFieldId fieldId) const = 0;

	bool hasField(VmGlobalFieldId fieldId) const;
};

} // namespace Goldbox

#endif // GOLDBOX_CORE_VM_LAYOUT_H
