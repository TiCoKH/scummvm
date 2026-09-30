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

#ifndef GOLDBOX_POOLRAD_DATA_POOLRAD_RULES_PROVIDER_H
#define GOLDBOX_POOLRAD_DATA_POOLRAD_RULES_PROVIDER_H

#include "goldbox/data/rules/rules.h"

namespace Goldbox {
namespace Poolrad {

/**
 * Pool of Radiance ruleset provider.
 *
 * Owns the race/class/alignment/spell tables specific to Pool of Radiance.
 * Other Gold Box games will provide their own RulesProvider implementations.
 */
class PoolradRulesProvider : public Goldbox::Data::Rules::RulesProvider {
public:
    bool isClassAllowed(uint8 race, uint8 classId) const override;
    bool isAlignmentAllowed(uint8 classId, uint8 alignmentId) const override;
    int thac0AtLevel(uint8 classId, uint8 level) const override;
    const Goldbox::Data::Rules::AgeDefEntry &getAgeDef(uint8 race, uint8 baseClassIndex) const override;
    const Goldbox::Data::Rules::ClassAlignmentDef *getAlignmentTable() const override;
    const Goldbox::Data::Rules::RaceClassDef *getRaceClassTable() const override;
    const Goldbox::Data::Rules::thac0Bases *getThac0Table() const override;
    const Common::Array< Common::Array<Goldbox::Data::Rules::AgeDefEntry> > &getAgeDefs() const override;
    const Common::Array<Goldbox::Data::Spells::SpellEntry> &getSpellEntries() const override;
    const Goldbox::Data::DiceRoll &getInitGoldRoll(uint8 baseClassIndex) const override;
    const Goldbox::Data::DiceRoll &getHPRoll(uint8 baseClassIndex) const override;
    int8 conHPModifier(uint8 constitution) const override;
    const Goldbox::Data::AgeCategories &getAgeCategoriesForRace(uint8 race) const override;
    const Common::Array<Goldbox::Data::AgeingEffects> &getStatAgeingEffects() const override;
    const Goldbox::Data::RaceStatMinMax &getRaceStatMinMaxForRace(uint8 race) const override;
    const Goldbox::Data::ClassMinStats &getClassMinStats(uint8 classId) const override;
    uint8 classEnumCount() const override;
    uint8 alignmentEnumCount() const override;
    const Goldbox::Data::ThiefSkills &getThiefSkillsForLevel(uint8 level) const override;
    Goldbox::Data::ThiefSkills computeThiefSkills(uint8 race, uint8 dexterity, uint8 thiefLevel) const override;
    uint8 classItemLimitBit(uint8 baseClassIndex) const override;
    uint8 computeItemLimitMask(const Common::Array<uint8> &levels) const override;
    const Goldbox::Data::SavingThrows &savingThrowsAt(uint8 baseClassIndex, uint8 level) const override;
    const Goldbox::Data::SpellSlots &getSpellSlotsForClassAtRow(uint8 baseClassIndex, uint8 row) const override;
    int32 xpForClassAtLevel(uint8 baseClassIndex, uint8 level) const override;
    uint8 forcedBaseIndexForMulticlass(uint8 classId) const override;
    uint16 rollInitialGold(const Goldbox::Data::LevelData &levels) const override;
    void applyStatMinMax(uint8 race, uint8 gender, uint8 classType,
            const Goldbox::Data::LevelData &levels, Goldbox::Data::AbilityScores &abilities) const override;
    uint8 getBlurEffectId() const override;
    uint8 getEndlessRegenEffectId() const override;

    /** Singleton accessor for convenience. */
    static PoolradRulesProvider &instance();
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_DATA_POOLRAD_RULES_PROVIDER_H
