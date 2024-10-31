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

#ifndef GOLDBOX_POOLRAD_DATA_CHARACTER_H
#define GOLDBOX_POOLRAD_DATA_CHARACTER_H

#include "common/str.h"
#include "common/array.h"
#include "common/hashmap.h"
#include "goldbox/core/file.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {

struct Stat {
    uint8 _base;
    uint8 _current;

	operator uint8() const { return _current; }
	Stat &operator=(byte v) {
		_base = _current = v;
		return *this;
	}
	Stat &operator++() {
		if (_base < 255)
			_current = ++_base;
		return *this;
	}
	Stat &operator--() {
		if (_base > 0)
			_current = --_base;
		return *this;
	}
    void set(byte v) { _current = v; }
    uint8 get() { return _current; }
	void clear() { _current = _base = 0; }
	void reset() { _current = _base; }
};

struct CombatStat {
    uint8 _raw_base;
    uint8 _raw_current;

    // Calculate the base value based on the raw base
    int8 calculatedBase() const { return 60 - _raw_base; }

    // Calculate the current value based on the raw current
    int8 calculatedCurrent() const { return 60 - _raw_current; }

    // Implicit conversion to uint8 (returns the raw current value)
    operator uint8() const { return _raw_current; }

    // Assignment operator
    CombatStat &operator=(byte v) {
        _raw_base = _raw_current = v;
        return *this;
    }

    // Pre-increment operator
    CombatStat &operator++() {
        if (_raw_base < 255) {
            _raw_current = ++_raw_base;
        }
        return *this;
    }

    // Pre-decrement operator
    CombatStat &operator--() {
        if (_raw_base > 0) {
            _raw_current = --_raw_base;
        }
        return *this;
    }

    // Set the current value
    void set(byte v) { 
        _raw_current = v; 
    }

    // Get the calculated current value
    int8 get() const { return calculatedCurrent(); }

    // Clear all values to 0
    void clear() { 
        _raw_current = _raw_base = 0; 
    }

    // Reset _current to _base
    void reset() { 
        _raw_current = _raw_base;
    }
};

class Class {
public:
    Class() {
        // Initialize the map with byte values and their corresponding class names
        _classMap.setVal(0x01, "Cleric");
        _classMap.setVal(0x02, "Knight");
        _classMap.setVal(0x03, "Fighter");
        _classMap.setVal(0x04, "Paladin");
        _classMap.setVal(0x05, "Ranger");
        _classMap.setVal(0x06, "Magic-User");
        _classMap.setVal(0x07, "Thief");
        _classMap.setVal(0x08, "Monk");
        _classMap.setVal(0x09, "Cleric/Fighter");
        _classMap.setVal(0x0A, "Cleric/Fighter/Magic-User");
        _classMap.setVal(0x0B, "Cleric/Ranger");
        _classMap.setVal(0x0C, "Cleric/Magic-User");
        _classMap.setVal(0x0D, "Cleric/Thief");
        _classMap.setVal(0x0E, "Fighter/Magic-User");
        _classMap.setVal(0x0F, "Fighter/Thief");
        _classMap.setVal(0x10, "Fighter/Magic-User/Thief");
        _classMap.setVal(0x11, "Magic-User/Thief");
    }

    // Method to get the class name from a byte value
    Common::String getClassName(uint8 key) const {
        return _classMap.getValOrDefault(key, "Unknown Class");
    }

private:
    Common::HashMap<uint8, Common::String> _classMap;
};

class Race {
public:
    Race() {
        // Initialize the map with byte values and their corresponding race names
        _raceMap.setVal(0x00, "Monster");
        _raceMap.setVal(0x01, "Dwarf");
        _raceMap.setVal(0x02, "Elf");
        _raceMap.setVal(0x03, "Gnome");
        _raceMap.setVal(0x04, "Half-Elf");
        _raceMap.setVal(0x05, "Halfling");
        _raceMap.setVal(0x06, "Human");
        _raceMap.setVal(0x07, "Monster");
    }

    // Method to get the race name from a byte value
    Common::String getRaceName(uint8 key) const {
        return _raceMap.getValOrDefault(key, "Unknown Race");
    }

private:
    Common::HashMap<uint8, Common::String> _raceMap;
};

struct Character {

    Common::String name;            // Offset: 0x01, Length: 15 bytes //On Amiga C string 14 char long with x0000 on the end. ON x86 Pascal string 15 char long
    Stat strength;                   // Offset: 0x10, Length: 1 byte
    Stat intelligence;               // Offset: 0x11, Length: 1 byte
    Stat wisdom;                     // Offset: 0x12, Length: 1 byte
    Stat dexterity;                  // Offset: 0x13, Length: 1 byte
    Stat constitution;               // Offset: 0x14, Length: 1 byte
    Stat charisma;                   // Offset: 0x15, Length: 1 byte
    Stat ext_strength;               // Offset: 0x16, Length: 1 byte
    char memorized_spells[22];       // Offset: 0x17, Length: 22 bytes
    CombatStat thac0;                // Offset: 0x2D, Length: 1 byte, Default: 40
    uint8 race;                      // Offset: 0x2E, Length: 1 byte
    uint8 class_type;                // Offset: 0x2F, Length: 1 byte
    uint16 age;                      // Offset: 0x30, Length: 2 bytes
    Stat hit_points;                 // Offset: 0x32, Length: 1 byte
    uint8 highest_level;             // Offset: 0x6B, Length: 1 byte
    uint8 creature_size = 1;         // Offset: 0x6C, Length: 1 byte, Default: 1
    uint8 save_vs_paralysis;         // Offset: 0x6D, Length: 1 byte
    uint8 save_vs_petrification;     // Offset: 0x6E, Length: 1 byte
    uint8 save_vs_rod_staff_wand;    // Offset: 0x6F, Length: 1 byte
    uint8 save_vs_breath_weapon;     // Offset: 0x70, Length: 1 byte
    uint8 save_vs_spell;             // Offset: 0x71, Length: 1 byte
    uint8 base_movement_speed;       // Offset: 0x72, Length: 1 byte
    uint8 hit_dice;                  // Offset: 0x73, Length: 1 byte
    uint8 drained_levels;            // Offset: 0x74, Length: 1 byte
    uint8 drained_hp;                // Offset: 0x75, Length: 1 byte
    uint8 undead_resistance;         // Offset: 0x76, Length: 1 byte
    uint8 thief_skills_pick_pockets; // Offset: 0x77, Length: 1 byte
    uint8 thief_skills_open_locks;   // Offset: 0x78, Length: 1 byte
    uint8 thief_skills_find_remove_traps; // Offset: 0x79, Length: 1 byte
    uint8 thief_skills_move_silently; // Offset: 0x7A, Length: 1 byte
    uint8 thief_skills_hide_in_shadows; // Offset: 0x7B, Length: 1 byte
    uint8 thief_skills_hear_noise; // Offset: 0x7C, Length: 1 byte
    uint8 thief_skills_climb_walls; // Offset: 0x7D, Length: 1 byte
    uint8 thief_skills_read_languages; // Offset: 0x7E, Length: 1 byte !!!!ON Amiga add more 1 byte to padding every offset+1 on Amiga
    uint32 effects_ptr;            // Offset: 0x7F, Length: 4 bytes !!!! Real memory address should be handled in a savefile reading
    uint8 unknown_byte_1;          // Offset: 0x83, Length: 1 byte
    uint8 npc_status;              // Offset: 0x84, Length: 1 byte
    uint8 modified;                // Offset: 0x85, Length: 1 byte
    uint8 unknown_byte_2;          // Offset: 0x86, Length: 1 byte
    uint8 unknown_byte_3;          // Offset: 0x87, Length: 1 byte
    uint8 unknown_byte_4;          // Offset: 0x88, Length: 1 byte
    uint16 copper;                 // Offset: 0x89, Length: 2 bytes !!!!ON Amiga add more 1 byte to padding every offset+2 on Amiga
    uint16 silver;                 // Offset: 0x8B, Length: 2 bytes
    uint16 electrum;               // Offset: 0x8D, Length: 2 bytes
    uint16 gold;                   // Offset: 0x8E, Length: 2 bytes
    uint16 platinum;               // Offset: 0x90, Length: 2 bytes
    uint16 gems;                   // Offset: 0x92, Length: 2 bytes
    uint16 jewels;                 // Offset: 0x94, Length: 2 bytes
    uint8 cleric_levels;           // Offset: 0x96, Length: 1 byte
    uint8 druid_levels;            // Offset: 0x97, Length: 1 byte
    uint8 fighter_levels;          // Offset: 0x98, Length: 1 byte
    uint8 paladin_levels;          // Offset: 0x99, Length: 1 byte
    uint8 ranger_levels;           // Offset: 0x9A, Length: 1 byte
    uint8 magic_user_levels;       // Offset: 0x9B, Length: 1 byte
    uint8 thief_levels;            // Offset: 0x9C, Length: 1 byte
    uint8 monk_levels;             // Offset: 0x9D, Length: 1 byte
    uint8 gender;                  // Offset: 0x9E, Length: 1 byte
    uint8 monster_type;            // Offset: 0x9F, Length: 1 byte
    uint8 alignment;               // Offset: 0xA0, Length: 1 byte
    uint8 primary_attacks_x2;      // Offset: 0xA1, Length: 1 byte
    uint8 secondary_attacks_x2;    // Offset: 0xA2, Length: 1 byte
    uint8 pri_attack_damage_dice_number; // Offset: 0xA3, Length: 1 byte
    uint8 sec_attack_damage_dice_number; // Offset: 0xA4, Length: 1 byte
    uint8 pri_attack_damage_dice_sides;   // Offset: 0xA5, Length: 1 byte
    uint8 sec_attack_damage_dice_sides; // Offset: 0xA6, Length: 1 byte
    uint8 pri_attack_damage_dice_modifier; // Offset: 0xA7, Length: 1 byte
    uint8 sec_attack_damage_dice_modifier; // Offset: 0xA8, Length: 1 byte
    CombatStat armor_class;         // Offset: 0xA9, Length: 1 byte, Default: 50
    uint8 strength_bonus_allowed;  // Offset: 0xAA, Length: 1 byte
    uint8 combat_icon;             // Offset: 0xAB, Length: 1 byte Default: Rand(256);
    uint32 experience_points;      // Offset: 0xAC, Length: 4 bytes
    uint8 class_item_usage_flags;  // Offset: 0xB0, Length: 1 byte
    uint8 hit_points_rolled;       // Offset: 0xB1, Length: 1 byte
    uint8 cleric_level1_spell_slots; // Offset: 0xB2, Length: 1 byte
    uint8 cleric_level2_spell_slots; // Offset: 0xB3, Length: 1 byte
    uint8 cleric_level3_spell_slots; // Offset: 0xB4, Length: 1 byte
    uint8 magic_user_level1_spell_slots; // Offset: 0xB5, Length: 1 byte
    uint8 magic_user_level2_spell_slots; // Offset: 0xB6, Length: 1 byte
    uint8 magic_user_level3_spell_slots; // Offset: 0xB7, Length: 1 byte
    uint16 base_xp_for_defeating_monster; // Offset: 0xB8, Length: 2 bytes
    uint8 bonus_xp_per_monster_hp;  // Offset: 0xBA, Length: 1 byte
    uint8 head_portrait = 1;            // Offset: 0xBB, Length: 1 byte, Default: 1
    uint8 body_portrait = 1;            // Offset: 0xBC, Length: 1 byte, Default: 1
    uint8 head_icon;                // Offset: 0xBD, Length: 1 byte
    uint8 weapon_icon;              // Offset: 0xBE, Length: 1 byte
    uint8 unknown_byte_5;           // Offset: 0xBF, Length: 1 byte
    uint8 icon_size;                // Offset: 0xC0, Length: 1 byte
    uint8 body_color;               // Offset: 0xC1, Length: 1 byte Default: 91 Created from {1,2,3,4,6,7}
    uint8 arm_color;                // Offset: 0xC2, Length: 1 byte Default: A2 and the complementer color
    uint8 leg_color;                // Offset: 0xC3, Length: 1 byte Default: B3
    uint8 head_color;               // Offset: 0xC4, Length: 1 byte Default: C4
    uint8 shield_color;             // Offset: 0xC5, Length: 1 byte Default: E6
    uint8 weapon_color;             // Offset: 0xC6, Length: 1 byte Default: F7
    uint8 spec_vulnerability_flags; // Offset: 0xC7, Length: 1 byte
    uint8 items[56];                // Offset: 0xC8, Length: 56 bytes
    uint8 hands_used;               // Offset: 0x100, Length: 1 byte
    uint8 unknown_byte_6;           // Offset: 0x101, Length: 1 byte
    uint16 encumbrance;             // Offset: 0x102, Length: 2 bytes
    uint8 unknown_byte_7;           // Offset: 0x104, Length: 1 byte
    uint8 unknown_byte_8;           // Offset: 0x105, Length: 1 byte
    uint8 unknown_byte_9;           // Offset: 0x106, Length: 1 byte
    uint32 actions;                 // Offset: 0x107, Length: 4 bytes
    uint8 unknown_byte_10;          // Offset: 0x10B, Length: 1 byte
    uint8 health_status = 0;            // Offset: 0x10C, Length: 1 byte
    uint8 in_combat = 1;                // Offset: 0x10D, Length: 1 byte
    uint8 side_in_combat;           // Offset: 0x10E, Length: 1 byte
    uint8 quick_fight_flag;         // Offset: 0x10F, Length: 1 byte
    CombatStat ac_for_rear_attacks; // Offset: 0x112, Length: 1 byte
    uint8 pri_attacks_left;         // Offset: 0x113, Length: 1 byte
    uint8 sec_attacks_left;         // Offset: 0x114, Length: 1 byte
    uint8 current_pri_attack_damage_dice_number; // Offset: 0x115, Length: 1 byte
    uint8 current_sec_attack_damage_dice_number; // Offset: 0x116, Length: 1 byte
    uint8 current_pri_attack_damage_dice_sides; // Offset: 0x117, Length: 1 byte
    uint8 current_sec_attack_damage_dice_sides; // Offset: 0x118, Length: 1 byte
    uint8 current_pri_attack_bonus; // Offset: 0x119, Length: 1 byte
    uint8 current_sec_attack_bonus; // Offset: 0x11A, Length: 1 byte
    uint8 current_movement;         // Offset: 0x11C, Length: 1 byte !!!!ON Amiga add more 1 byte on the end now +3

    void loadFrom(Common::SeekableReadStream &stream);
};

} // namespace Data
} // namespace Poolrad
} // namespace Goldbox

#endif