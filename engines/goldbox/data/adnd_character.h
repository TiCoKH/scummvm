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
#include "goldbox/data/items/character_inventory.h"

namespace Goldbox {
namespace Data {

enum RaceADnD {
    R_MONSTER     = 0,
    R_DWARF       = 1,
    R_ELF         = 2,
    R_GNOME       = 3,
    R_HALF_ELF    = 4,
    R_HALFLING    = 5,
    R_HALF_ORC    = 6,
    R_HUMAN       = 7
};

enum ClassADnD {
    C_CLERIC              = 0x00,
    C_DRUID               = 0x01,
    C_FIGHTER             = 0x02,
    C_PALADIN             = 0x03,
    C_RANGER              = 0x04,
    C_MAGE                = 0x05,
    C_THIEF               = 0x06,
    C_MONK                = 0x07,
    C_CLERIC_FIGHTER      = 0x08,
    C_CLERIC_FIGHTER_MAGE = 0x09,
    C_CLERIC_RANGER       = 0x0A,
    C_CLERIC_MAGE         = 0x0B,
    C_CLERIC_THIEF        = 0x0C,
    C_FIGHTER_MAGE        = 0x0D,
    C_FIGHTER_THIEF       = 0x0E,
    C_FIGHTER_MAGE_THIEF  = 0x0F,
    C_MAGE_THIEF          = 0x10,
    C_MONSTER             = 0x11
};

enum AlignmentADnD {
    A_LAWFUL_GOOD       = 0,
    A_LAWFUL_NEUTRAL    = 1,
    A_LAWFUL_EVIL       = 2,
    A_NEUTRAL_GOOD      = 3,
    A_TRUE_NEUTRAL      = 4,
    A_NEUTRAL_EVIL      = 5,
    A_CHAOTIC_GOOD      = 6,
    A_CHAOTIC_NEUTRAL   = 7,
    A_CHAOTIC_EVIL      = 8
};

enum StatusADnD {
    S_OKAY        = 0,
    S_ANIMATED    = 1,
    S_TEMPGONE    = 2,
    S_RUNNING     = 3,
    S_UNCONSCIOUS = 4,
    S_DYING       = 5,
    S_DEAD        = 6,
    S_STONED      = 7,
    S_GONE        = 8
};

struct SpellData {
    uint8 memorizedSpells[22];  // Array for memorized spells; layout may differ by game.
    uint8 knownSpells[62];      // Array for known spells; layout may differ by game.
};

// Grouping for saving throw protections (commonly, versus paralysis, petrification, rods/staves/wands, breath weapon, spells)
struct SavingThrows {
    uint8 vsParalysis;      // Save vs. paralysis, poison or death magic
    uint8 vsPetrification;  // Save vs. petrification or polymorph
    uint8 vsRodStaffWand;   // Save vs. rod, staff, or wand
    uint8 vsBreathWeapon;   // Save vs. breath weapon
    uint8 vsSpell;          // Save vs. spell
};

// Grouping for thief skills (typically eight skills in AD&D 1st edition)
struct ThiefSkills {
    uint8 pickPockets;
    uint8 openLocks;
    uint8 findRemoveTraps;
    uint8 moveSilently;
    uint8 hideInShadows;
    uint8 hearNoise;
    uint8 climbWalls;
    uint8 readLanguages;
};

// Grouping for level data; one field per class type supported.
struct LevelData {
    uint8 cleric;
    uint8 druid;
    uint8 fighter;
    uint8 paladin;
    uint8 ranger;
    uint8 mage;
    uint8 thief;
    uint8 monk;
};

// Grouping for combat–related numbers; here we include THAC0 and attack level.
struct CombatData {
    uint8 thac0Base;   // THAC0 base value (attack metric)
    uint8 attackLevel; // Overall attack level indicator
    // Additional combat fields (e.g., unarmed attack values) could be added here.
};

struct CombatRolls {
    uint8 attacks;
    uint8 rolls;
    uint8 dice;
    uint8 modifier;
};

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

struct ValuableItems {
    uint16 coinsCopper;
    uint16 coinsSilver;
    uint16 coinsElectrum;
    uint16 coinsGold;
    uint16 coinsPlatinum;
    uint16 gems;
    uint16 jewelry;
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
    uint8 npc;
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


    // Virtual destructor to ensure proper cleanup in derived classes.
    virtual ~ADnDCharacter() {}

    // --------------------------------------------------------------------
    // Pure virtual functions to be implemented by game–specific derived classes.
    // These methods handle file–format specifics and game–rule calculations.
    virtual void load(Common::SeekableReadStream &stream) = 0;
    virtual void save(Common::WriteStream &stream) = 0;
    virtual void rollAbilityScores() = 0;
    virtual void applyRacialAdjustments() = 0;
    virtual void calculateHitPoints() = 0;

    // --------------------------------------------------------------------
    // Common utility methods

};

} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_ADND_CHARACTER_H
