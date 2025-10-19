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

#define BASE_CLASS_NUM 8

namespace Goldbox {
namespace Data {

// Grouping for equipment addresses; these fields hold the file–offset addresses for equipped items.
struct EquipmentAddresses {
    uint32 equippedWeapon;
    uint32 equippedShield;
    uint32 equippedArmor;
    uint32 equippedGauntlets;
    uint32 equippedHelm;
    uint32 equippedBelt;
    uint32 equippedRobe;
    uint32 equippedCloak;
    uint32 equippedBoots;
    uint32 equippedRing1;
    uint32 equippedRing2;
    uint32 equippedArrow;
    uint32 equippedBolt;
};

// The base class for player characters.
// This class defines the core (common) data and behavior shared among AD&D–based games.
class ADnDCharacter : public PlayerCharacter {
public:
    // Grouped data for clarity
    SpellData spells;              // Memorized spells or spell book data
    SavingThrows savingThrows;      // Saving throw values (protections)

    CombatData combat;             // Combat-related data (e.g., THAC0)

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

    EquipmentAddresses equipment;  // Addresses of equipped items
    uint8 handsEquiped;
    uint8 saveBonus;
    uint16 encumbrance;


    Goldbox::Data::Items::CharacterInventory inventory;

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
    virtual void calculateHitPoints() = 0;

    // Provide base forwarding hook for effects (optional override in concrete game classes)
    virtual void setEffect(uint8 /*type*/, uint16 /*durationMin*/, uint8 /*power*/, bool /*immediate*/) override {}

    // --------------------------------------------------------------------
    // Common utility methods

};

} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_ADND_CHARACTER_H
