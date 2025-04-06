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

namespace Goldbox {
namespace Poolrad {
namespace Data {



class PoolradCharacter : public Goldbox::Data::ADnDCharacter {
public:

    uint8 highestLevel = 0;
    uint8 creatureSize = 1;

    uint8 baseMovement = 0;
    uint8 hitDice = 0;
    uint8 drainedLevels = 0;
    uint8 drainedHp = 0;
    uint8 undeadResistance = 0;

    uint8 monsterType = 0;

    // Combat detail
    uint8 primaryAttacks = 0, secondaryAttacks = 0;
    uint8 priDmgDiceNum = 0, secDmgDiceNum = 0;
    uint8 priDmgDiceSides = 0, secDmgDiceSides = 0;
    uint8 priDmgModifier = 0, secDmgModifier = 0;
    uint8 strengthBonusAllowed = 0;
    uint8 combatIcon = 0;

    uint8 hitPointsRolled = 0;
    uint8 spellSlots[6] = {};
    uint32 xpForDefeating = 0;
    uint8 bonusXpPerHp = 0;

    uint8 itemsLimit;
    uint8 itemBytes[56] = {};

    uint8 handsUsed = 0;
    uint16 encumbrance = 0;

    uint32 actions = 0;
    uint8 sideInCombat = 0;
    uint8 quickFightFlag = 0;

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
    void save(Common::WriteStream &stream) const override;

    void initialize();
    bool meetsClassRequirements() const;
    void finalizeName();

    void rollAbilityScores() override;
    void applyRacialAdjustments() override;
    void calculateHitPoints() override;

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

private:
    // Helper methods
    void adjustAbilityForRace(Goldbox::Data::Stat &ability, int adjustment, int minValue, int maxValue);
};

} // namespace Data
} // namespace Poolrad
} // namespace Goldbox

    #endif // GOLDBOX_POOLRAD_DATA_POOLRAD_CHARACTER_H

