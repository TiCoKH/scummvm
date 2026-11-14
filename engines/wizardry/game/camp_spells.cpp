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

#include "wizardry/game/camp_spells.h"

namespace Wizardry {

bool CampSpells::isCampSpell(int spell) {
	switch (spell) {
	case DIOS: case DIAL: case DIALMA: case MADI:
	case MILWA: case LOMILWA: case LATUMAPIC: case MAPORFIC:
	case DIALKO: case LATUMOFIS: case DI: case KADORTO:
	case KANDI: case DUMAPIC: case MALOR:
		return true;
	default:
		return false;
	}
}

bool CampSpells::needsTarget(int spell) {
	switch (spell) {
	case DIOS: case DIAL: case DIALMA: case MADI:
	case DIALKO: case LATUMOFIS: case DI: case KADORTO:
		return true;
	default:
		return false;
	}
}

static void heal(Character &c, int dice, int sides, Dice &source) {
	int amount = 0;
	for (int i = 0; i < dice; ++i)
		amount += source.roll(sides) + 1;
	c.hp = MIN<int>(c.hp + amount, c.hpMax);
}

CampSpells::Result CampSpells::cast(Party &party, uint casterIndex, int spell, uint targetIndex,
		MazeState &maze, Dice &dice) {
	if (!isCampSpell(spell))
		return kNotACampSpell;

	// TODO: these need screens of their own: finding a body, the map
	// position, and choosing where to teleport
	if (spell == KANDI || spell == DUMAPIC || spell == MALOR)
		return kNotYet;

	Character &caster = party.member(casterIndex);
	Character &target = party.member(needsTarget(spell) ? targetIndex : casterIndex);

	if (spell == DI || spell == KADORTO) {
		// Asked before anything is used up
		if (target.status == kStatusLost)
			return kIsLost;
		if (target.status != kStatusDead && target.status != kStatusAshes)
			return kNotDead;
	}

	Spells::spend(caster, spell);
	if (maze.spellsFizzle)
		return kFizzled;

	switch (spell) {
	case DIOS:
		heal(target, 1, 8, dice);
		return kHealed;
	case DIAL:
		heal(target, 2, 8, dice);
		return kHealed;
	case DIALMA:
		heal(target, 3, 8, dice);
		return kHealed;
	case MADI:
		// All hit points, no poison, and any state short of death cured
		target.hp = target.hpMax;
		target.poison = 0;
		if (target.status < kStatusDead)
			target.status = kStatusOk;
		return kHealed;

	case MILWA:
		maze.lightDuration += 30 + dice.roll(15);
		return kDone;
	case LOMILWA:
		maze.lightDuration = 10000;
		return kDone;
	case LATUMAPIC:
		maze.monstersIdentified = 10000;
		return kDone;
	case MAPORFIC:
		maze.acBonus = 2;
		return kDone;

	case DIALKO:
		if (target.status == kStatusAsleep || target.status == kStatusParalysed)
			target.status = kStatusOk;
		return kDone;
	case LATUMOFIS:
		target.poison = 0;
		return kDone;

	default:
		break;
	}

	// DI and KADORTO. DI cannot raise ashes.
	if (spell == DI && target.status == kStatusAshes)
		return kTryKadorto;

	const int vitality = target.attributes[kVitality];
	if ((int)dice.roll(100) <= 4 * vitality) {
		target.hp = (spell == DI) ? 1 : target.hpMax;

		// It costs a point of vitality; one too frail does not come back.
		// (The original leaves that one in a state past "lost".)
		if (vitality == 3) {
			target.status = kStatusLost;
			return kRaiseFailed;
		}
		target.attributes[kVitality] = vitality - 1;
		target.status = kStatusOk;
		return kRaised;
	}

	// Dead turns to ashes, ashes are lost
	++target.status;
	return kRaiseFailed;
}

} // End of namespace Wizardry
