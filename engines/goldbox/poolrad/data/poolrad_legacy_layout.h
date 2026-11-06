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

#ifndef GOLDBOX_POOLRAD_DATA_POOLRAD_LEGACY_LAYOUT_H
#define GOLDBOX_POOLRAD_DATA_POOLRAD_LEGACY_LAYOUT_H

#include "common/scummsys.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {

/**
 * Dedicated codec boundary for the Pool of Radiance (DOS x86, v1.3) on-disk
 * "wire" formats (GB13-034 / Requirement 38).
 *
 * This namespace is the single authority for the DOS legacy byte layout. The
 * GB13-033 native versioned save schema (NativeSave::sync) MUST NOT reference
 * these constants and this header MUST NOT depend on the native schema, so
 * that evolving the native save cannot shift the DOS wire format. The constants
 * are mirror/assert values describing the layout implemented by:
 *   - PoolradCharacter::load/save      (285-byte character record)
 *   - CharacterInventory::load/save    (63-byte .ITM item record)
 *   - Effect::load/save                (9-byte .SPC effect node)
 *   - PoolradEngine::saveGameSlotX86   (SAVGAM?.DAT game-state container)
 *
 * All values are byte offsets/widths into the little-endian DOS record. They
 * are compile-time constants (no runtime global constructors).
 *
 * POINTER PLACEHOLDER fields hold DOS heap/far-pointer addresses that are not
 * stable data: the loader discards them and the saver writes a canonical value
 * (0 for addresses, a rebuilt value for the leading FAVAIL byte). A raw-byte
 * round-trip comparison MUST normalize these before classifying a diff.
 */
namespace LegacyLayout {

// --- 285-byte character record (poolrad_character.cpp load/save) ------------
// Record ends at the last field movement.current @0x11C, so size = 0x11D.
static const uint32 kCharacterRecordSize = 285; // 0x11D

// Named field offsets (data fields unless marked POINTER/NORMALIZED).
static const uint32 kCharName            = 0x000; // Pascal string, 16 bytes (1 len + 15)
static const uint32 kCharNameFieldWidth  = 16;
static const uint32 kCharStrength        = 0x010;
static const uint32 kCharStrException    = 0x016;
static const uint32 kCharMemorizedSpells = 0x017; // 21 bytes
static const uint32 kCharSpellRegainRate = 0x02C;
static const uint32 kCharThac0Base       = 0x02D;
static const uint32 kCharRace            = 0x02E;
static const uint32 kCharClassType       = 0x02F;
static const uint32 kCharAge             = 0x030; // u16 LE
static const uint32 kCharHitPointsMax    = 0x032;
static const uint32 kCharKnownSpells     = 0x033; // 56 bytes
static const uint32 kCharKnownSpellsSize = 56;
static const uint32 kCharAttackLevel     = 0x06B;
static const uint32 kCharIconDimension   = 0x06C;
static const uint32 kCharSavingThrows    = 0x06D; // 5 bytes
static const uint32 kCharMovementBase    = 0x072;
static const uint32 kCharThiefSkills     = 0x077; // 8 bytes
static const uint32 kCharEffectsAddress  = 0x07F; // POINTER PLACEHOLDER (4 bytes, loader skips to 0x84)
static const uint32 kCharNpc             = 0x084; // signed: bit7 NPC, bits6-0 morale
static const uint32 kCharModified        = 0x085;
static const uint32 kCharValuables       = 0x088; // VALUABLE_COUNT u16 LE each (coins/gems)
static const uint32 kCharClassLevels     = 0x096; // 8 bytes
static const uint32 kCharGender          = 0x09E;
static const uint32 kCharMonsterType     = 0x09F;
static const uint32 kCharAlignment       = 0x0A0;
static const uint32 kCharBaseAttacks     = 0x0A1; // 2 bytes (primary, secondary)
static const uint32 kCharBaseUnarmed     = 0x0A3; // 6 bytes dice/mod
static const uint32 kCharArmorClassBase  = 0x0A9;
static const uint32 kCharStrBonusAllowed = 0x0AA;
static const uint32 kCharCombatIcon      = 0x0AB; // CONFIRMED combatIcon (see poolrad_legacy_layout.cpp notes / GB13-034 B)
static const uint32 kCharExperience      = 0x0AC; // u32 LE
static const uint32 kCharItemsLimit      = 0x0B0;
static const uint32 kCharHitPointsRolled = 0x0B1;
static const uint32 kCharSpellSlots      = 0x0B2; // 6 bytes (cleric L1-3, mu L1-3)
static const uint32 kCharXpForDefeating  = 0x0B8; // u16 LE
static const uint32 kCharBonusXpPerHp    = 0x0BA;
static const uint32 kCharPortraitHead    = 0x0BB;
static const uint32 kCharPortraitBody    = 0x0BC;
static const uint32 kCharIconData        = 0x0BD; // 10 bytes custom icon (iconHead..weapon color)
static const uint32 kCharNumOfItems      = 0x0C7;
static const uint32 kCharItemsAddress    = 0x0C8; // POINTER PLACEHOLDER (u32)
static const uint32 kCharEquippedOffsets = 0x0CC; // POINTER PLACEHOLDER array (u32 x EQUIPMENT_SLOT_COUNT=13)
static const uint32 kCharEquippedSlots   = 13;
static const uint32 kCharHandsEquipped   = 0x100;
static const uint32 kCharSaveBonus       = 0x101;
static const uint32 kCharEncumbrance     = 0x102; // u16 LE
static const uint32 kCharNextCharAddr    = 0x104; // POINTER PLACEHOLDER (u32)
static const uint32 kCharCombatAddr      = 0x108; // POINTER PLACEHOLDER (u32)
static const uint32 kCharHealthStatus    = 0x10C;
static const uint32 kCharEnabled         = 0x10D;
static const uint32 kCharCombatSide      = 0x10E;
static const uint32 kCharAiControl       = 0x10F;
static const uint32 kCharThac0Current    = 0x110;
static const uint32 kCharAcCurrent       = 0x111;
static const uint32 kCharAcRear          = 0x112;
static const uint32 kCharCurAttacks      = 0x113; // 8 bytes current rolls
static const uint32 kCharHitPointsCur    = 0x11B;
static const uint32 kCharMovementCur     = 0x11C; // last byte (size - 1)

// Pointer-placeholder / normalized spans in the character record. A raw diff
// landing inside one of these ranges is NORMALIZED, never ERRONEOUS.
//   [offset, length]
static const uint32 kCharEffectsAddrLen  = 5;  // 0x07F-0x083: 4-byte far ptr + 1 unknown byte
static const uint32 kCharUnknown086Len   = 2;  // 0x086-0x087: unknown pad (saver writes 0)
static const uint32 kCharItemsAddrLen    = 4;
static const uint32 kCharEquippedLen     = 52; // 13 * 4 bytes of far pointers
static const uint32 kCharNextCombatLen   = 8;  // 0x104-0x10B: two far pointers

// --- 63-byte item record (.ITM, CharacterInventory::load/save) --------------
static const uint32 kItemRecordSize   = 63; // 0x3F
static const uint32 kItemName         = 0x00; // Pascal string, 42 bytes (1 len + 41)
static const uint32 kItemNameWidth    = 42;
static const uint32 kItemNextAddress  = 0x2A; // POINTER PLACEHOLDER (u32) - used as legacy equip key
static const uint32 kItemTypeIndex    = 0x2E;
static const uint32 kItemNameCode1    = 0x2F;
static const uint32 kItemNameCode2    = 0x30;
static const uint32 kItemNameCode3    = 0x31;
static const uint32 kItemBonus        = 0x32;
static const uint32 kItemSaveBonus    = 0x33;
static const uint32 kItemReadied      = 0x34;
static const uint32 kItemHidden       = 0x35;
static const uint32 kItemCursed       = 0x36;
static const uint32 kItemWeight       = 0x37; // u16 LE
static const uint32 kItemStackSize    = 0x39;
static const uint32 kItemValue        = 0x3A; // u16 LE
static const uint32 kItemEffect1      = 0x3C;
static const uint32 kItemEffect2      = 0x3D;
static const uint32 kItemEffect3      = 0x3E; // bit7 set => magic/ready effect

// --- 9-byte effect node (.SPC, Effect::load/save) ---------------------------
static const uint32 kEffectNodeSize   = 9;
static const uint32 kEffectId         = 0x0;
static const uint32 kEffectDuration   = 0x1; // u16 LE (minutes)
static const uint32 kEffectPower      = 0x3; // 0xFF = permanent
static const uint32 kEffectImmediate  = 0x4;
static const uint32 kEffectNextPtr    = 0x5; // POINTER PLACEHOLDER (far ptr, 4 bytes)
static const uint32 kEffectNextPtrLen = 4;

// --- CHARLIST.TXT roster index ---------------------------------------------
static const uint32 kCharListSize     = 69;

// --- SAVGAM?.DAT game-state container (PoolradEngine::saveGameSlotX86) -------
// NOTE: no genuine SAVGAM?.DAT fixture is available (GB13-034 OPEN item); the
// following describes the writer. Banks are WORD-EXPANDED on write: each VM
// byte is stored as [lo=value][hi=0], so on-disk width = 2 * word count.
static const uint32 kGameFavailByte       = 0;       // NORMALIZED: leading BYTE_FAVAIL, saver writes 3
static const uint16 kGameBank0WorldAddr   = 0x4900;  // VMBANK0_WORLD_STATE
static const uint16 kGameBank0WordCount   = 0x0400;  // -> 0x0800 bytes (word-expanded)
static const uint16 kGameBank1PartyAddr   = 0x6B00;  // VMBANK1_PARTY_STATE
static const uint16 kGameBank1WordCount   = 0x0400;  // -> 0x0800 bytes
static const uint16 kGameBank2CombatAddr  = 0x9700;  // VMBANK2_COMBAT_STATE
static const uint16 kGameBank2WordCount   = 0x0200;  // -> 0x0400 bytes
static const uint16 kGameBank3EclAddr     = 0x9900;  // VMBANK3_ECL_SCRIPT (byte-addressed)
static const uint32 kGameBank3ByteCount   = 0x1E00;
static const uint32 kGameCharTableStride  = 0x29;    // per-character roster entry stride in tail

// Size of the word-expanded leading section (FAVAIL + three word banks + ecl).
static const uint32 kGameHeaderSize =
	1 /* FAVAIL */ +
	(uint32)kGameBank0WordCount * 2 +
	(uint32)kGameBank1WordCount * 2 +
	(uint32)kGameBank2WordCount * 2 +
	kGameBank3ByteCount;

} // namespace LegacyLayout
} // namespace Data
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_DATA_POOLRAD_LEGACY_LAYOUT_H
