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

#ifndef GOLDBOX_DATA_ADND_CHARACTER_H
#define GOLDBOX_DATA_ADND_CHARACTER_H

#include "common/str.h"
#include "common/array.h"
#include "common/stream.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/rules/rules_types.h"  // Centralized shared AD&D type
#include "goldbox/data/items/character_inventory.h"
#include "goldbox/data/items/base_items.h"

#define BASE_CLASS_NUM 8

namespace Goldbox {
namespace Data {

// The base class for player characters.
// This class defines the core (common) data and behavior shared among AD&D–based games.
class ADnDCharacter : public PlayerCharacter {
public:
	// Grouped data for clarity
	SpellData spells;              // Memorized spells or spell book data
	SavingThrows savingThrows;      // Saving throw values (protections)

	uint8 attackLevel;
	uint8 drainedLevels;
	uint8 drainedHPs;
	uint8 levelUndead;
	ThiefSkills thiefSkills;       // Thief skills
	int8 npc;
	uint8 modified;

	ValuableItems valuableItems;
	LevelData levels;              // Class level information

	int8 numOfItems;
	uint32 itemsAddress;

	uint8 handsEquipped;
	uint8 saveBonus;

	Goldbox::Data::Items::CharacterInventory inventory;

	// --------------------------------------------------------------------
	// Unified combat rolls model (base + current), common to AD&D engines.
	// Maps legacy per-hand fields into a portable representation.
	// 'attacks' = attacks per round; 'rolls' = number of damage dice;
	// 'dice' = sides per die; 'modifier' = flat damage modifier (signed).
	Goldbox::Data::CombatRoll basePrimaryRoll;
	Goldbox::Data::CombatRoll baseSecondaryRoll;
	Goldbox::Data::CombatRoll curPrimaryRoll;
	Goldbox::Data::CombatRoll curSecondaryRoll;
	// --------------------------------------------------------------------
	// New, engine-friendly equipped items mapping (slot -> inventory index)
	// Kept alongside the legacy 32-bit pointer addresses for savefile
	// compatibility. Use this in runtime logic; convert to/from legacy
	// addresses when loading/saving.
	enum { EQUIPPED_SLOT_COUNT = (int)Goldbox::Data::Items::Slot::SLOT_COUNT };

	struct EquippedItems {
		Common::Array<Goldbox::Data::Items::CharacterItem *> slots; // index by Slot enum; nullptr means empty

		EquippedItems() {
			slots.resize(EQUIPPED_SLOT_COUNT);
			for (int i = 0; i < EQUIPPED_SLOT_COUNT; ++i)
				slots[i] = nullptr;
		}

		void clear() {
			if (slots.size() != (uint)EQUIPPED_SLOT_COUNT)
				slots.resize(EQUIPPED_SLOT_COUNT);
			for (int i = 0; i < EQUIPPED_SLOT_COUNT; ++i)
				slots[i] = nullptr;
		}

		// Iterator support
		Goldbox::Data::Items::CharacterItem **begin() { return slots.begin(); }
		Goldbox::Data::Items::CharacterItem **end() { return slots.end(); }
		const Goldbox::Data::Items::CharacterItem *const *begin() const { return slots.begin(); }
		const Goldbox::Data::Items::CharacterItem *const *end() const { return slots.end(); }
	} equippedItems;

	// Clear equippedItems to nullptrs
	void clearEquippedItems();

	// Populate equippedItems from legacy offsets table and current inventory.
	// Any unmatched offsets result in -1 for the corresponding slot.
	void resolveEquippedFromLegacyOffsets(const uint32 *offsets);

	// Build legacy offsets for saving from equippedItems (or, if empty,
	// infer from items' readied flags). Writes 0 when no item.
	void buildLegacyOffsetsFromEquipped(uint32 *offsetsOut) const;

	// Accessors for modern equipped items
	Goldbox::Data::Items::CharacterItem *getEquippedItem(Goldbox::Data::Items::Slot slot);
	const Goldbox::Data::Items::CharacterItem *getEquippedItem(Goldbox::Data::Items::Slot slot) const;
	const Goldbox::Data::Items::ItemProperty *getEquippedProp(Goldbox::Data::Items::Slot slot) const;
	// Debug validation helper: logs mismatches between equippedItems pointers and readied flags
	void debugValidateEquipped() const;

	// Convenience helpers
	bool hasShieldEquipped() const;     // Interprets OFF_HAND item with protect>0 as shield
	bool isDualWielding() const;        // Main hand + off hand weapon (off hand not a shield)
	const Goldbox::Data::Items::ItemProperty *mainWeaponProp() const; // Shorthand

	// AC component structure for cumulative AC calculation
	struct AcComponents {
		int8 dexAdj;     // Dex AC adjustment (DexAcBonus)
		uint8 shield;    // Shield AC magic bonus (OFF_HAND protect hi-bit)
		uint8 misc;      // Sum of misc magical AC (non-armor, non-ring, non-shield)
		uint8 ring;      // Best ring/protection bonus (cleared if magic armor worn)
		uint8 armorBase; // Base armor AC (base + armor magic)

		AcComponents() : dexAdj(0), shield(0), misc(0), ring(0), armorBase(0) {}
		
		int getTotalAC() const {
			int total = armorBase + misc + ring + shield + dexAdj;
			return CLIP<int>(total, 0, 255);
		}

		int getRearAC() const {
			int rearAc = misc + ring + armorBase - 2;
			return CLIP<int>(rearAc, 0, 255);
		}


	};

	// Apply an item's defensive bonuses and protections to character.
	// Returns true if the item provides protection (protection flag is set).
	void setItemProtection(const Goldbox::Data::Items::CharacterItem *item, 
						  AcComponents *acComponents,
						  bool *magicArmorWorn);

	// Apply movement modifier based on equipped body armor weight and magical bonus.
	// Uses `movement.base` as input and writes the result to `movement.current`.
	void armorMovementEffect(const Goldbox::Data::Items::CharacterItem *armorItem);

	// Inventory accessors (generic for all AD&D-based characters)
	Goldbox::Data::Items::CharacterInventory &getInventory() { return inventory; }
	const Goldbox::Data::Items::CharacterInventory &getInventory() const { return inventory; }
	void clearInventory() { inventory.clear(); }


	// Virtual destructor to ensure proper cleanup in derived classes.
	virtual ~ADnDCharacter() {}

	// --------------------------------------------------------------------
	// Pure virtual functions to be implemented by game–specific derived classes.
	// These methods handle file–format specifics and game–rule calculations.
	virtual void load(Common::SeekableReadStream &stream) = 0;
	virtual void save(Common::WriteStream &stream) = 0;
	virtual void rollAbilityScores() = 0;

	// Provide base forwarding hook for effects (optional override in concrete game classes)
	virtual void setEffect(uint8 /*type*/, uint16 /*durationMin*/, uint8 /*power*/, bool /*immediate*/) override {}

	// --------------------------------------------------------------------
	// Common utility methods

	// ---- Combat rolls helpers ----
	// Accessors for combat rolls
	const Goldbox::Data::CombatRoll &getBasePrimaryRoll() const { return basePrimaryRoll; }
	const Goldbox::Data::CombatRoll &getBaseSecondaryRoll() const { return baseSecondaryRoll; }
	const Goldbox::Data::CombatRoll &getCurrentPrimaryRoll() const { return curPrimaryRoll; }
	const Goldbox::Data::CombatRoll &getCurrentSecondaryRoll() const { return curSecondaryRoll; }

	void setBasePrimaryRoll(const Goldbox::Data::CombatRoll &roll) { basePrimaryRoll = roll; }
	void setBaseSecondaryRoll(const Goldbox::Data::CombatRoll &roll) { baseSecondaryRoll = roll; }
	void setCurrentPrimaryRoll(const Goldbox::Data::CombatRoll &roll) { curPrimaryRoll = roll; }
	void setCurrentSecondaryRoll(const Goldbox::Data::CombatRoll &roll) { curSecondaryRoll = roll; }

	// Reset current rolls back to base values (used at combat start / rest etc.).
	void resetCurrentRollsFromBase();

	// ---- Carrying capacity helpers ----
	// Calculate the carrying capacity modifier based on strength tier.
	// Returns a value that adjusts the base carrying capacity.
	int getCapacityModifier() const;

	// Apply encumbrance-based movement reduction.
	// Reduces movement.current based on effective encumbrance (encumbrance - capacity modifier).
	// Uses thresholds: >1024 -> cap at 3, >=769 -> cap at 6, >=513 -> cap at 9.
	void setMovement();

};

} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_ADND_CHARACTER_H
