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

#include "goldbox/poolrad/data/poolrad_vm_layout.h"

#include "common/debug.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {

namespace {

static const uint16 kPoolradGeoFirstVmAddr = 0x4900;
static const uint16 kPoolradDatFirstVmAddr = 0x6B00;
static const uint16 kPoolradSystemFirstVmAddr = 0xC04B;
static const uint16 kPoolradGeoLastScriptFlagVmAddr =
	PoolradScriptFlags::kFirstVmAddr + PoolradScriptFlags::kFlagCount - 1;

// Geo bank mapping notes:
//   - Geo bank first VM word address: 0x4900
//   - Decompile-style byte offset:     offset = vmAddr * 2 - 0x9200
//   - Equivalent from bank base:       offset = (vmAddr - 0x4900) * 2
//
// Keep VM addresses canonical in code, but include byte offsets in comments
// because original Gold Box decompiles commonly reference GB_PTR_MEM_GEO + 0xXXX.

// Dat bank mapping notes:
//   - Dat bank first VM word address: 0x6B00
//   - Decompile-style byte offset:     offset = vmAddr * 2 - 0xD600
//   - Equivalent from bank base:       offset = (vmAddr - 0x6B00) * 2

// System bank mapping notes:
//   - System bank first VM word address: 0xC04B
//   - Equivalent from bank base:          offset = (vmAddr - 0xC04B) * 2

static const char *kPoolradVmFieldNames[Goldbox::kVmFieldCount] = {
	"WildernessX",
	"WildernessY",
	"GeoBlockId",
	"ClockUnits",
	"ClockMinuteOnes",
	"ClockMinuteTens",
	"ClockHour",
	"ClockDay",
	"ClockMonth",
	"ClockYearLo",
	"ClockYearHi",
	"NoMagicFlag",
	"IndoorModeFlag",
	"WallSetPrimary",
	"WallSetSecondary",
	"SavedMapX",
	"SavedMapY",
	"SavedMapId",
	"HideCoordsOrAutomapDisable",
	"GameSpeed",
	"SkyColor",
	"CeilingColor",
	"OptionBits",
	"ScriptFlagAA2",
	"ScriptFlagAB3"
};
static const char *kPoolradVmGlobalFieldNames[Goldbox::kVmGlobalFieldCount] = {
	"SelectedPcIndex",
	"CombatResult",
	"CombatantCount",
	"MovementBlock",
	"SearchFlags",
	"CombatIsAmbush",
	"MoraleThreshold",
	"SpokespersonChaBonus",
	"RestSafeTime",
	"RestInterruptChance",
	"TriedToLeaveMap",
	"PictureHeadId",
	"EnterTemplePending",
	"NoItemCombatFlag",
	"PartyCount",
	"DivisionModulo",
	"ShopPriceMultiplier",
	"MonsterThac0Bonus",
	"PartyThac0DmgBonus",
	"PartyMoveModifier",
	"DungeonX",
	"DungeonY",
	"DungeonDir",
	"MapWallType",
	"MapSquareInfo",
	"MonsterDistance",
	"ShopFlag"
};

static_assert(sizeof(kPoolradVmFieldNames) / sizeof(kPoolradVmFieldNames[0]) ==
	Goldbox::kVmFieldCount,
	"Poolrad VM field-name table must match VmFieldId enum count");
static_assert(sizeof(kPoolradVmGlobalFieldNames) /
	sizeof(kPoolradVmGlobalFieldNames[0]) == Goldbox::kVmGlobalFieldCount,
	"Poolrad VM global field-name table must match VmGlobalFieldId enum count");

static void validatePoolradLayoutMappings() {
	static bool validated = false;
	if (validated) {
		return;
	}

	PoolradVmLayout vmLayout;
	PoolradGlobalVmLayout globalLayout;

	assert(vmLayout.field(kVmFieldSavedMapX).vmAddr == 0x49F0);
	assert(vmLayout.field(kVmFieldSavedMapY).vmAddr == 0x49F1);
	assert(vmLayout.field(kVmFieldSavedMapId).vmAddr == 0x49F2);

	assert(globalLayout.field(kVmGlobalFieldPartyCount).vmAddr == 0x6E3E);
	assert(globalLayout.field(kVmGlobalFieldMonsterDistance).vmAddr == 0x6DC1);
	assert(globalLayout.field(kVmGlobalFieldShopFlag).vmAddr == 0x6E6C);

	validated = true;
}

} // namespace

const char *PoolradVmLayout::layoutName() const {
	return "poolrad";
}

Goldbox::VmFieldLocation PoolradVmLayout::field(Goldbox::VmFieldId fieldId) const {
	switch (fieldId) {
	// Note: VM addresses are word-addressed. Incrementing vmAddr by 1 means
	// +2 bytes in legacy decompile-style bank offsets.
	// GEO bank, ordered by VM address / offset
	case kVmFieldWildernessX:                  return {kVmBankGeo, 0x49C2}; // @ +0x186
	case kVmFieldWildernessY:                  return {kVmBankGeo, 0x49C4}; // @ +0x188
	case kVmFieldGeoBlockId:                   return {kVmBankGeo, 0x49C6}; // @ +0x18A
	case kVmFieldClockUnits:                   return {kVmBankGeo, 0x49C5}; // @ +0x18C
	case kVmFieldClockMinuteOnes:              return {kVmBankGeo, 0x49C7}; // @ +0x18E
	case kVmFieldClockMinuteTens:              return {kVmBankGeo, 0x49C8}; // @ +0x190
	case kVmFieldClockHour:                    return {kVmBankGeo, 0x49C9}; // @ +0x192
	case kVmFieldClockDay:                     return {kVmBankGeo, 0x49CA}; // @ +0x194
	case kVmFieldClockMonth:                   return {kVmBankGeo, 0x49CB}; // @ +0x196
	case kVmFieldClockYearLo:                  return {kVmBankGeo, 0x49CC}; // @ +0x198
	case kVmFieldClockYearHi:                  return {kVmBankGeo, 0x49CD}; // @ +0x19A
	case kVmFieldNoMagicFlag:                  return {kVmBankGeo, 0x49E5}; // @ +0x1CA
	case kVmFieldIndoorModeFlag:               return {kVmBankGeo, 0x49E6}; // @ +0x1CC
	case kVmFieldWallSetPrimary:               return {kVmBankGeo, 0x49E7}; // @ +0x1CE
	case kVmFieldWallSetSecondary:             return {kVmBankGeo, 0x49E8}; // @ +0x1D0
	case kVmFieldSavedMapX:                    return {kVmBankGeo, 0x49F0}; // @ +0x1E0
	case kVmFieldSavedMapY:                    return {kVmBankGeo, 0x49F1}; // @ +0x1E2
	case kVmFieldSavedMapId:                   return {kVmBankGeo, 0x49F2}; // @ +0x1E4
	case kVmFieldHideCoordsOrAutomapDisable:   return {kVmBankGeo, 0x49FB}; // @ +0x1F6
	case kVmFieldGameSpeed:                    return {kVmBankGeo, 0x49FC}; // @ +0x1F8
	case kVmFieldSkyColor:                     return {kVmBankGeo, 0x49FD}; // @ +0x1FA
	case kVmFieldCeilingColor:                 return {kVmBankGeo, 0x49FE}; // @ +0x1FC
	case kVmFieldOptionBits:                   return {kVmBankGeo, 0x49FF}; // @ +0x1FE
	case kVmFieldScriptFlagAA2:                return {kVmBankGeo, 0x4AA2}; // @ +0x344
	case kVmFieldScriptFlagAB3:                return {kVmBankGeo, 0x4AB3}; // @ +0x366
	default:
		return Goldbox::VmLayout::invalidLocation();
	}
}

const Goldbox::VmLayout &getPoolradVmLayout() {
	validatePoolradLayoutMappings();
	static PoolradVmLayout layout;
	return layout;
}

const char *PoolradGlobalVmLayout::layoutName() const {
	return "poolrad-global";
}

Goldbox::VmFieldLocation PoolradGlobalVmLayout::field(
	Goldbox::VmGlobalFieldId fieldId) const {
	switch (fieldId) {
	// DAT bank, ordered by VM address / offset
	case kVmGlobalFieldSelectedPcIndex:        return {kVmBankDat, 0x6DB1}; // @ +0x562
	case kVmGlobalFieldMonsterDistance:        return {kVmBankDat, 0x6DC1}; // @ +0x582
	case kVmGlobalFieldMoraleThreshold:        return {kVmBankDat, 0x6DC6}; // @ +0x58C
	case kVmGlobalFieldCombatResult:           return {kVmBankDat, 0x6DC7}; // @ +0x58E
	case kVmGlobalFieldCombatantCount:         return {kVmBankDat, 0x6DC8}; // @ +0x590
	case kVmGlobalFieldMovementBlock:          return {kVmBankDat, 0x6DC9}; // @ +0x592
	case kVmGlobalFieldSearchFlags:            return {kVmBankDat, 0x6DCA}; // @ +0x594
	case kVmGlobalFieldCombatIsAmbush:         return {kVmBankDat, 0x6DCB}; // @ +0x596
	case kVmGlobalFieldSpokespersonChaBonus:   return {kVmBankDat, 0x6DCF}; // @ +0x59E
	case kVmGlobalFieldRestSafeTime:           return {kVmBankDat, 0x6DD2}; // @ +0x5A4
	case kVmGlobalFieldRestInterruptChance:    return {kVmBankDat, 0x6DD3}; // @ +0x5A6
	case kVmGlobalFieldTriedToLeaveMap:        return {kVmBankDat, 0x6DD5}; // @ +0x5AA
	case kVmGlobalFieldPictureHeadId:          return {kVmBankDat, 0x6DE1}; // @ +0x5C2
	case kVmGlobalFieldEnterTemplePending:     return {kVmBankDat, 0x6DE2}; // @ +0x5C4
	case kVmGlobalFieldNoItemCombatFlag:       return {kVmBankDat, 0x6DE3}; // @ +0x5C6
	case kVmGlobalFieldPartyCount:             return {kVmBankDat, 0x6E3E}; // @ +0x67C
	case kVmGlobalFieldDivisionModulo:         return {kVmBankDat, 0x6E3F}; // @ +0x67E
	case kVmGlobalFieldShopFlag:               return {kVmBankDat, 0x6E6C}; // @ +0x6D8
	case kVmGlobalFieldShopPriceMultiplier:    return {kVmBankDat, 0x6E6D}; // @ +0x6DA
	case kVmGlobalFieldMonsterThac0Bonus:      return {kVmBankDat, 0x6E70}; // @ +0x6E0
	case kVmGlobalFieldPartyThac0DmgBonus:     return {kVmBankDat, 0x6E71}; // @ +0x6E2
	case kVmGlobalFieldPartyMoveModifier:      return {kVmBankDat, 0x6E72}; // @ +0x6E4

	// SYSTEM bank, ordered by VM address / offset
	case kVmGlobalFieldDungeonX:               return {kVmBankSystem, 0xC04B}; // @ +0x000
	case kVmGlobalFieldDungeonY:               return {kVmBankSystem, 0xC04C}; // @ +0x002
	case kVmGlobalFieldDungeonDir:             return {kVmBankSystem, 0xC04D}; // @ +0x004
	case kVmGlobalFieldMapWallType:            return {kVmBankSystem, 0xC04E}; // @ +0x006
	case kVmGlobalFieldMapSquareInfo:          return {kVmBankSystem, 0xC04F}; // @ +0x008


	default:
		return Goldbox::VmLayout::invalidLocation();
	}
}

const Goldbox::VmGlobalLayout &getPoolradGlobalVmLayout() {
	validatePoolradLayoutMappings();
	static PoolradGlobalVmLayout layout;
	return layout;
}

namespace {

class PoolradEclRuntimeLayout : public Goldbox::ECL::EclRuntimeLayout {
public:
	const char *layoutName() const override {
		return "poolrad-ecl-runtime";
	}

	uint16 field(Goldbox::ECL::EclRuntimeFieldId fieldId) const override {
		switch (fieldId) {
		case Goldbox::ECL::kEclRuntimeBreakFlag:         return 0x442E;
		case Goldbox::ECL::kEclRuntimeExitScript:        return 0x442F;
		case Goldbox::ECL::kEclRuntimeProgramState:      return 0x4430;
		case Goldbox::ECL::kEclRuntimeGameState:         return 0x4431;
		case Goldbox::ECL::kEclRuntimeMenuCombatState:   return 0x4432;
		case Goldbox::ECL::kEclRuntimeHaltFlag:          return 0x49FF;
		case Goldbox::ECL::kEclRuntimeSelectedCharPtr:   return 0x6000;
		case Goldbox::ECL::kEclRuntimeNextCharPtr:       return 0x6002;
		case Goldbox::ECL::kEclRuntimeTextBuffer:        return 0x84C8;
		case Goldbox::ECL::kEclRuntimeTextPrintFlag:     return 0x84DE;
		case Goldbox::ECL::kEclRuntimeTextOutputFlag:    return 0x84DF;
		case Goldbox::ECL::kEclRuntimeMenuStatus:        return 0x84E4;
		case Goldbox::ECL::kEclRuntimeSpriteState:       return 0x84E3;
		case Goldbox::ECL::kEclRuntimeUiFlag:            return 0x84E9;
		case Goldbox::ECL::kEclRuntimeOperandBufferLow:  return 0x7046;
		case Goldbox::ECL::kEclRuntimeOperandBufferHigh: return 0x7086;
		case Goldbox::ECL::kEclRuntimeMonsterCount:      return 0x0580;
		case Goldbox::ECL::kEclRuntimeMonsterData:       return 0x0582;
		case Goldbox::ECL::kEclRuntimeEncounterFlags:    return 0x0594;
		default:
			return Goldbox::ECL::EclRuntimeLayout::kInvalidVmAddr;
		}
	}
};

} // namespace

const Goldbox::ECL::EclRuntimeLayout &getPoolradEclRuntimeLayout() {
	validatePoolradLayoutMappings();
	static PoolradEclRuntimeLayout layout;
	static bool runtimeValidated = false;
	if (!runtimeValidated) {
		assert(layout.field(Goldbox::ECL::kEclRuntimeBreakFlag) == 0x442E);
		assert(layout.field(Goldbox::ECL::kEclRuntimeGameState) == 0x4431);
		assert(layout.field(Goldbox::ECL::kEclRuntimeMonsterCount) == 0x0580);
		runtimeValidated = true;
	}
	return layout;
}

const char *poolradVmFieldName(Goldbox::VmFieldId fieldId) {
	if (fieldId < 0 || fieldId >= Goldbox::kVmFieldCount) {
		return "<invalid>";
	}

	return kPoolradVmFieldNames[fieldId];
}

bool poolradGeoVmAddrOffset(uint16 vmAddr, uint16 &byteOffset) {
	if (vmAddr < kPoolradGeoFirstVmAddr ||
			vmAddr > kPoolradGeoLastScriptFlagVmAddr) {
		return false;
	}

	int32 offset = Goldbox::VmAddressMapper::toByteOffset(vmAddr,
		kPoolradGeoFirstVmAddr);
	if (offset < 0 || offset > 0xFFFF) {
		return false;
	}

	byteOffset = static_cast<uint16>(offset);
	return true;
}

bool poolradVmFieldOffset(Goldbox::VmFieldId fieldId, uint16 &byteOffset) {
	const Goldbox::VmLayout &layout = getPoolradVmLayout();
	Goldbox::VmFieldLocation location = layout.field(fieldId);
	if (!Goldbox::VmLayout::isValid(location) || location.bankId != kVmBankGeo) {
		return false;
	}

	return poolradGeoVmAddrOffset(location.vmAddr, byteOffset);
}

void debugDumpPoolradVmLayout() {
	debug("Poolrad VM layout dump (%s)", getPoolradVmLayout().layoutName());
	for (int i = 0; i < Goldbox::kVmFieldCount; ++i) {
		Goldbox::VmFieldId fieldId = static_cast<Goldbox::VmFieldId>(i);
		Goldbox::VmFieldLocation location = getPoolradVmLayout().field(fieldId);
		uint16 offset = 0;
		if (poolradVmFieldOffset(fieldId, offset)) {
			debug("  %-28s bank=%d vm=0x%04X off=+0x%03X",
				poolradVmFieldName(fieldId),
				static_cast<int>(location.bankId),
				location.vmAddr,
				offset);
		} else {
			debug("  %-28s bank=%d vm=0x%04X off=<n/a>",
				poolradVmFieldName(fieldId),
				static_cast<int>(location.bankId),
				location.vmAddr);
		}
	}
}

const char *poolradVmGlobalFieldName(Goldbox::VmGlobalFieldId fieldId) {
	if (fieldId < 0 || fieldId >= Goldbox::kVmGlobalFieldCount) {
		return "<invalid>";
	}

	return kPoolradVmGlobalFieldNames[fieldId];
}

bool PoolradScriptFlags::isScriptFlagAddr(uint16 vmAddr) {
	return vmAddr >= kFirstVmAddr && vmAddr < (kFirstVmAddr + kFlagCount);
}

uint16 PoolradScriptFlags::flagAddr(uint8 index) {
	return static_cast<uint16>(kFirstVmAddr + index);
}

bool PoolradScriptFlags::flagOffset(uint8 index, uint16 &byteOffset) {
	if (index >= kFlagCount) {
		return false;
	}

	return poolradGeoVmAddrOffset(flagAddr(index), byteOffset);
}

void PoolradScriptFlags::resetOnScriptLoad(Goldbox::VmWordBank &geoBank) {
	for (uint16 i = 0; i < kTransientFlagCount; ++i) {
		geoBank.writeWord(flagAddr(static_cast<uint8>(i)), 0);
	}
}

void PoolradScriptFlags::clearAll(Goldbox::VmWordBank &geoBank) {
	for (uint16 i = 0; i < kFlagCount; ++i) {
		geoBank.writeWord(flagAddr(static_cast<uint8>(i)), 0);
	}
}

bool poolradVmGlobalFieldOffset(Goldbox::VmGlobalFieldId fieldId,
	uint16 &byteOffset) {
	const Goldbox::VmGlobalLayout &layout = getPoolradGlobalVmLayout();
	Goldbox::VmFieldLocation location = layout.field(fieldId);
	if (!Goldbox::VmLayout::isValid(location)) {
		return false;
	}

	int32 offset = -1;
	if (location.bankId == kVmBankDat) {
		offset = Goldbox::VmAddressMapper::toByteOffset(location.vmAddr,
			kPoolradDatFirstVmAddr);
	} else if (location.bankId == kVmBankSystem) {
		offset = Goldbox::VmAddressMapper::toByteOffset(location.vmAddr,
			kPoolradSystemFirstVmAddr);
	} else {
		return false;
	}

	if (offset < 0 || offset > 0xFFFF) {
		return false;
	}

	byteOffset = static_cast<uint16>(offset);
	return true;
}

void debugDumpPoolradGlobalVmLayout() {
	debug("Poolrad global VM layout dump (%s)",
		getPoolradGlobalVmLayout().layoutName());
	for (int i = 0; i < Goldbox::kVmGlobalFieldCount; ++i) {
		Goldbox::VmGlobalFieldId fieldId =
			static_cast<Goldbox::VmGlobalFieldId>(i);
		Goldbox::VmFieldLocation location =
			getPoolradGlobalVmLayout().field(fieldId);
		uint16 offset = 0;
		if (poolradVmGlobalFieldOffset(fieldId, offset)) {
			debug("  %-28s bank=%d vm=0x%04X off=+0x%03X",
				poolradVmGlobalFieldName(fieldId),
				static_cast<int>(location.bankId),
				location.vmAddr,
				offset);
		} else {
			debug("  %-28s bank=%d vm=0x%04X off=<n/a>",
				poolradVmGlobalFieldName(fieldId),
				static_cast<int>(location.bankId),
				location.vmAddr);
		}
	}
}

} // namespace Data
} // namespace Poolrad
} // namespace Goldbox
