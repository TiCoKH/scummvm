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

// Free-function forwarders to the currently active RulesProvider. This keeps
// existing call sites (Goldbox::Data::Rules::xxx(...)) working unchanged
// while the actual ruleset tables/logic live behind a per-game provider
// (see PoolradRulesProvider), mirroring how Combat::TilePropertyProvider is
// injected per game.

#include "goldbox/data/rules/rules.h"
#include "goldbox/engine.h"

namespace Goldbox {
namespace Data {
namespace Rules {

namespace {
RulesProvider *g_rulesProvider = nullptr;
}

RulesProvider &getRulesProvider() {
	if (!g_rulesProvider)
		error("Goldbox::Data::Rules: no RulesProvider registered");
	return *g_rulesProvider;
}

void setRulesProvider(RulesProvider *provider) {
	g_rulesProvider = provider;
}

bool isClassAllowed(uint8 race, uint8 classId) {
	return getRulesProvider().isClassAllowed(race, classId);
}

bool isAlignmentAllowed(uint8 classId, uint8 alignmentId) {
	return getRulesProvider().isAlignmentAllowed(classId, alignmentId);
}

int thac0AtLevel(uint8 classId, uint8 level) {
	return getRulesProvider().thac0AtLevel(classId, level);
}

const AgeDefEntry &getAgeDef(uint8 race, uint8 baseClassIndex) {
	return getRulesProvider().getAgeDef(race, baseClassIndex);
}

const ClassAlignmentDef *getAlignmentTable() {
	return getRulesProvider().getAlignmentTable();
}

const RaceClassDef *getRaceClassTable() {
	return getRulesProvider().getRaceClassTable();
}

const thac0Bases *getThac0Table() {
	return getRulesProvider().getThac0Table();
}

const Common::Array< Common::Array<AgeDefEntry> > &getAgeDefs() {
	return getRulesProvider().getAgeDefs();
}

const Common::Array<Spells::SpellEntry> &getSpellEntries() {
	return getRulesProvider().getSpellEntries();
}

const DiceRoll &getInitGoldRoll(uint8 baseClassIndex) {
	return getRulesProvider().getInitGoldRoll(baseClassIndex);
}

const DiceRoll &getHPRoll(uint8 baseClassIndex) {
	return getRulesProvider().getHPRoll(baseClassIndex);
}

int8 conHPModifier(uint8 constitution) {
	return getRulesProvider().conHPModifier(constitution);
}

const AgeCategories &getAgeCategoriesForRace(uint8 race) {
	return getRulesProvider().getAgeCategoriesForRace(race);
}

const Common::Array<AgeingEffects> &getStatAgeingEffects() {
	return getRulesProvider().getStatAgeingEffects();
}

const RaceStatMinMax &getRaceStatMinMaxForRace(uint8 race) {
	return getRulesProvider().getRaceStatMinMaxForRace(race);
}

const ClassMinStats &getClassMinStats(uint8 classId) {
	return getRulesProvider().getClassMinStats(classId);
}

uint8 classEnumCount() {
	return getRulesProvider().classEnumCount();
}

uint8 alignmentEnumCount() {
	return getRulesProvider().alignmentEnumCount();
}

const ThiefSkills &getThiefSkillsForLevel(uint8 level) {
	return getRulesProvider().getThiefSkillsForLevel(level);
}

ThiefSkills computeThiefSkills(uint8 race, uint8 dexterity, uint8 thiefLevel) {
	return getRulesProvider().computeThiefSkills(race, dexterity, thiefLevel);
}

uint8 classItemLimitBit(uint8 baseClassIndex) {
	return getRulesProvider().classItemLimitBit(baseClassIndex);
}

uint8 computeItemLimitMask(const Common::Array<uint8> &levels) {
	return getRulesProvider().computeItemLimitMask(levels);
}

const SavingThrows &savingThrowsAt(uint8 baseClassIndex, uint8 level) {
	return getRulesProvider().savingThrowsAt(baseClassIndex, level);
}

const SpellSlots &getSpellSlotsForClassAtRow(uint8 baseClassIndex, uint8 row) {
	return getRulesProvider().getSpellSlotsForClassAtRow(baseClassIndex, row);
}

int32 xpForClassAtLevel(uint8 baseClassIndex, uint8 level) {
	return getRulesProvider().xpForClassAtLevel(baseClassIndex, level);
}

uint8 forcedBaseIndexForMulticlass(uint8 classId) {
	return getRulesProvider().forcedBaseIndexForMulticlass(classId);
}

uint16 rollInitialGold(const LevelData &levels) {
	return getRulesProvider().rollInitialGold(levels);
}

void applyStatMinMax(uint8 race, uint8 gender, uint8 classType,
		const LevelData &levels, AbilityScores &abilities) {
	getRulesProvider().applyStatMinMax(race, gender, classType, levels, abilities);
}

uint8 getBlurEffectId() {
	return getRulesProvider().getBlurEffectId();
}

uint8 getEndlessRegenEffectId() {
	return getRulesProvider().getEndlessRegenEffectId();
}

} // namespace Rules
} // namespace Data
} // namespace Goldbox
