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

    uint8 baseMovement = 0;
    uint8 hitDice = 0;
    uint8 drainedLevels = 0;
    uint8 drainedHp = 0;
    uint8 undeadResistance = 0;

    uint8 monsterType = 0;

    // No SpellBook abstraction; we use legacy arrays directly for compatibility
    // Legacy fields for binary compatibility
    struct {
        uint8 memorizedSpells[21];
        uint8 knownSpells[55];
    } spells;

    Equipment equipment;
    uint32 equippedOffsets[EQUIPMENT_SLOT_COUNT];

    // Combat detail
    uint8 primaryAttacks = 0, secondaryAttacks = 0;
    uint8 priDmgDiceNum = 0, secDmgDiceNum = 0;
    uint8 priDmgDiceSides = 0, secDmgDiceSides = 0;
    uint8 priDmgModifier = 0, secDmgModifier = 0;
    uint8 combatIcon = 0;

    uint8 hitPointsRolled = 0;
    Goldbox::Data::ClassSpellSlots spellSlots{};
    uint32 xpForDefeating = 0;
    uint8 bonusXpPerHp = 0;

    Goldbox::Data::Effects::CharacterEffects effects;

    uint8 itemsLimit;
    uint8 itemBytes[56] = {};

    uint8 handsUsed = 0;
    uint16 encumbrance = 0;

    uint32 actions = 0;

    Goldbox::Data::CombatStat acRear;

    // Attacks left
    uint8 priAttacksLeft = 0, secAttacksLeft = 0;

    // Damage dice (current)
    uint8 curPriDiceNum = 0, curSecDiceNum = 0;
    uint8 curPriDiceSides = 0, curSecDiceSides = 0;
    uint8 curPriBonus = 0, curSecBonus = 0;

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
    void calculateHitPoints() override;
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
        // Return aggregated Constitution HP modifier across active base classes filtered by flags.
        // Uses Rules::conHPModifier(CON) as base per-slot, with fighter bonuses: +1 at CON>=17,
        // additional +1 at CON>=18, applied per active slot. Aggregates like getRolledHP.

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

