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

#ifndef GOLDBOX_POOLRAD_DATA_POOLRAD_CHARACTER_H
#define GOLDBOX_POOLRAD_DATA_POOLRAD_CHARACTER_H

#include "common/str.h"
#include "common/array.h"
#include "goldbox/core/file.h"

#include "goldbox/data/adnd_character.h"
#include "goldbox/data/items/base_items.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/data/spells/spell.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {



constexpr int EQUIPMENT_SLOT_COUNT = static_cast<int>(Goldbox::Data::Items::Slot::SLOT_COUNT);
constexpr uint8 CLASS_COUNT = 8;
constexpr uint8 MAX_CLASSES_PER_RACE = 11;

struct Equipment {
    int slots[EQUIPMENT_SLOT_COUNT];
};

class PoolradCharacter : public Goldbox::Data::ADnDCharacter {
public:

    static constexpr uint8 BASE_ICON_VALUES[7] = {0x0A, 0x01, 0x02, 0x03, 0x04, 0x06, 0x07};

    uint8 highestLevel = 0;
    uint8 creatureSize = 1;

    uint8 hitDice = 0;
    uint8 undeadResistance = 0;

    uint8 monsterType = 0;
    uint8 combatIcon = 0; // probably used in monsters

    // No SpellBook abstraction; we use legacy arrays directly for compatibility
    // Legacy fields for binary compatibility
    struct {
        uint8 memorizedSpells[21];
        uint8 knownSpells[55];
    } spells;

    Equipment equipment;
    uint32 equippedOffsets[EQUIPMENT_SLOT_COUNT];

    uint8 hitPointsRolled = 0;
    Goldbox::Data::ClassSpellSlots spellSlots{};
    uint32 xpForDefeating = 0;
    uint8 bonusXpPerHp = 0;

    Goldbox::Data::Effects::CharacterEffects effects;

    uint8 itemsLimit;
    uint8 itemBytes[56] = {};

    uint32 actions = 0;

    Goldbox::Data::CombatStat acRear;

    PoolradCharacter();

    // === I/O and core methods ===

    void load(Common::SeekableReadStream &stream) override;
    void save(Common::WriteStream &stream) override;

    void initialize();

    bool meetsClassRequirements() const;
    bool haveMemorizedSpell() const;
    void finalizeName();

    // Returns true if the character knows the given spell (by Spells enum id).
    // Primary source is the legacy knownSpells[55] buffer (ids 1..55 map to [0..54]).
    // For ids outside that range, falls back to the SpellBook known flag.
    bool isSpellKnown(Goldbox::Data::Spells::Spells spell) const;

    // Marks the given spell (by Spells enum id) as known.
    // Updates both the legacy knownSpells buffer (for ids 1..55) and the
    // SpellBook entry, preserving existing memorized count when present.
    void setSpellKnown(Goldbox::Data::Spells::Spells spell);

    void rollAbilityScores() override;
    // Aggregate Constitution HP modifier across active base-class slots (0..7).
    // Uses base modifier from rules for the character's Constitution. If the character
    // has any Fighter level, adds +1 at CON>=17 and an additional +1 at CON>=18 to the
    // per-slot modifier before summing. Returns the total across active slots.
    int8 getConHPModifier() const;
    // Roll additional HP for levels beyond 1 for allowed base classes (bitmask in ClassFlag).
    // For each base class 0..7: if levels[base] > 1 and the corresponding bit in flags is set,
    // roll (levels[base] - 1) dice using the class's hit die and add the sum to hitPointsRolled.
    // Also increases hitPoints.max and hitPoints.current by the same amount (clamped to uint8).
    uint8 getRolledHP(Goldbox::Data::ClassFlag flags);
    // Return number of active base classes (levels > 0) across 0..7.
    uint8 countActiveBaseClasses() const;

    // Compute and apply final saving throws from base-class contributions
    // using the best (lowest) value for each category. Handles dual-class quirks
    // via highestLevel when applicable.
    void computeSavingThrows();

    // Compute and apply THAC0 using the best progression among active base classes.
    // Also refreshes itemsLimit bitmask.
    void computeThac0();

    // Roll and apply initial age using rules for class/race and multiclass mapping.
    void rollInitialAge();

    // Apply cumulative ageing effects to ability scores based on current age and race.
    void applyAgeingEffects();

    // Compute spell slots and initial known spells appropriate for current levels
    // for Cleric and Magic-User according to Pool of Radiance rules.
    void computeSpellSlots();

    static uint8 getBaseIconColor(int index);
    /**
     * Set platform-dependent default values for a freshly generated character.
     *
     * This was split out from the general initialization so that future
     * platform specific variants (e.g. Amiga / PC-98) can override the logic
     * without duplicating the whole constructor or load path. The DOS values
     * (current default) are implemented in the base version here.
     */
    virtual void initializeNewCharacter();

    void resolveEquippedItems();
    void recalcCombatStats();

    // Set current damage and to-hit based on equipped weapon and stats.
    // Sets current primary dice/sides/modifier and applies STR/DEX, ammo enchantment and racial adjustments.
    void setDamage();

    void setEffect(uint8 type, uint16 durationMin, uint8 power, bool immediate) override {
        effects.addEffect(type, durationMin, power, immediate ? 1 : 0);
    }

    // draw methods
    byte getNameColor();
    //void drawCombatStats();

    // Implementing pure virtual functions from PlayerCharacter
    const char *getRaceName() const override {
        // Provide implementation
        return "RaceName";
    }

    const char *getClassName() const override {
        // Provide implementation
        return "ClassName";
    }

    const char *getGenderName() const override {
        // Provide implementation
        return "GenderName";
    }

    const char *getAlignmentName() const override {
        // Provide implementation
        return "AlignmentName";
    }

    const char *getStatusName() const override {
        // Provide implementation
        return "StatusName";
    }
};

} // namespace Data
} // namespace Poolrad
} // namespace Goldbox

    #endif // GOLDBOX_POOLRAD_DATA_POOLRAD_CHARACTER_H

