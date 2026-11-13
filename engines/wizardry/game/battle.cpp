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

#include "wizardry/game/battle.h"

namespace Wizardry {

enum {
	kMsgDrainedOne = 1100,
	kMsgDrainedMany = 1101,
	kMsgIs = 1102,
	kMsgPoisoned = 1103,
	kMsgParalyzed = 1104,
	kMsgStoned = 1105,
	kMsgMisses = 1107,
	kMsgHitsOnce = 1108,
	kMsgHitsTimes = 1109,
	kMsgIsKilled = 1110,
	kMsgDecapitated = 1111,
	kMsgNoHelpComes = 1112,
	kMsgHelpAppears = 1113,
	kMsgRunsAway = 1114,
	kMsgDispelFails = 1115,
	kMsgDispelsOne = 1116,
	kMsgDispelsMany = 1117,
	kMsgResists = 1118,
	kMsgSpellKilled = 1119,
	kMsgTakesDamage = 1120,
	kMsgIsWord = 1121,         // "^ IS &"
	kMsgIsNotWord = 1122,
	kMsgRestored = 1123,
	kMsgCompletelyHealed = 1124,
	kMsgHeld = 1125,
	kMsgSilenced = 1126,
	kMsgDoesNotWork = 1127,
	kMsgSlain = 1128,
	kMsgSlept = 1129,
	kMsgNothingHappens = 1130,
	kMsgCured = 1131,
	kMsgNotCured = 1132,
	kMsgCannotSpeak = 1133,
	kMsgFizzles = 1134,
	kMsgCasts = 1136,
	kMsgBreathes = 1135,
	kMsgMonsterVerbs = 1150,   // five of them
	kMsgPartyVerbs = 1155,     // five of them
	kMsgCountAndName = 1300,   // "# ^"

	kClassUndead = 10,

	// Saving throws
	kSaveDeath = 0,
	kSaveStoning = 1,
	kSaveBreath = 3,

	// Bits of a character's resistances
	kResistPoison = 3,
	kResistDrain = 4,
	kResistStoning = 5,
	kResistSpells = 6,    // spells aimed at this one are wasted

	// Kinds of damage a spell can do
	kElementNone = 0,
	kElementFire = 1,
	kElementCold = 2
};

// What a spell does to a mind that does not resist
enum StatusEffect {
	kEffectSleep, kEffectSilence, kEffectDeath
};

enum Affliction {
	kAfflictPoison, kAfflictParalysis, kAfflictStoning, kAfflictDeath
};

// "^ IS &" with a name and a word
static Common::String isWord(const Common::String &text, const Common::String &name, const Common::String &word) {
	Common::String result;
	for (uint i = 0; i < text.size(); ++i) {
		if (text[i] == '^')
			result += name;
		else if (text[i] == '&')
			result += word;
		else
			result += text[i];
	}
	return result;
}

Battle::Battle(Party &party, const MonsterList &monsters, const MessageFile &messages,
		MazeState &maze, Dice &dice) :
		_party(party), _monsters(monsters), _messages(messages), _maze(maze), _dice(dice) {
	for (int g = 0; g < kGroupCount; ++g) {
		_groups[g].identified = false;
		_groups[g].aliveCount = _groups[g].startCount = 0;
		_groups[g].monsterId = -1;
		memset(_groups[g].members, 0, sizeof(_groups[g].members));
	}
	for (uint i = 0; i < Party::kMaxMembers; ++i)
		_drained[i] = false;
}

int Battle::rollDice(const Dice3 &dice) {
	int result = dice.bonus;
	for (int i = 0; i < dice.count; ++i) {
		if (dice.sides > 0)
			result += _dice.roll(dice.sides) + 1;
	}
	return result;
}

void Battle::say(const Common::String &line) {
	if (_lines)
		_lines->push_back(line);
}

Common::String Battle::format(uint message, const Common::String &name, const Common::String &verb,
		const Common::String &victim, int hits, int damage) const {
	// ^ who, ? what is done, % to whom, # how often, & how much
	const Common::String &text = _messages.get(message);
	Common::String result;

	for (uint i = 0; i < text.size(); ++i) {
		switch (text[i]) {
		case '^':
			result += name;
			break;
		case '?':
			result += verb;
			break;
		case '%':
			result += victim;
			break;
		case '#':
			result += Common::String::format("%d", hits);
			break;
		case '&':
			result += Common::String::format("%d", damage);
			break;
		default:
			result += text[i];
			break;
		}
	}

	return result;
}

Common::String Battle::monsterName(int group) const {
	const Group &g = _groups[group];
	return _messages.get(MonsterList::nameMessage(g.monsterId, g.identified, false));
}

void Battle::loadEnemyGroup(int monsterId, int group) {
	if (monsterId < 0 || monsterId >= (int)_monsters.size())
		return;

	Group &g = _groups[group];
	g.monsterId = monsterId;
	g.enemy = _monsters[monsterId];

	// A monster may bring another kind along, and that one another;
	// the deeper the level, the more groups there can be
	if (group < 4 && group - 1 < _maze.level && g.enemy.partnerId > 0 &&
			(int)_dice.roll(100) < g.enemy.partnerChance)
		loadEnemyGroup(g.enemy.partnerId, group + 1);
}

void Battle::begin(int monsterId, bool canRun) {
	_canRun = canRun;
	_outcome = kUndecided;

	loadEnemyGroup(monsterId, 1);

	for (int gi = 1; gi < kGroupCount; ++gi) {
		Group &g = _groups[gi];
		if (g.monsterId < 0)
			continue;

		int count = rollDice(g.enemy.groupSize);
		count = MIN(count, _maze.level + 4);
		count = CLIP(count, 1, (int)kMaxInGroup);
		g.startCount = g.aliveCount = count;
		g.identified = _maze.monstersIdentified > 0;

		for (int i = 0; i < count; ++i) {
			Fighter &f = g.members[i];
			f.level = g.enemy.hitDice.count;
			f.hp = rollDice(g.enemy.hitDice);
			f.status = kStatusOk;
			f.acModifier = 0;
			f.silencedRounds = 0;
			f.action = kParry;
			f.initiative = -1;
		}
	}

	Group &party = _groups[0];
	party.aliveCount = party.startCount = _party.size();
	for (uint i = 0; i < _party.size(); ++i) {
		const Character &c = _party.member(i);
		Fighter &f = party.members[i];
		f.level = c.level;
		f.hp = c.hp;
		f.status = c.status;
		f.acModifier = 0;
		f.silencedRounds = 0;
		f.action = kNotChosen;
		f.initiative = -1;
	}

	// About two fights in three start evenly
	if (_dice.roll(100) < 81)
		_surprise = (_dice.roll(100) < 81) ? kNoSurprise : kMonstersSurprise;
	else
		_surprise = kPartySurprises;

	// TODO: monsters that turn out to be friendly
}

int Battle::enemyGroups() const {
	int count = 0;
	for (int g = 1; g < kGroupCount; ++g)
		count += _groups[g].aliveCount > 0;
	return count;
}

Common::String Battle::groupLine(int group, int &able) const {
	const Group &g = _groups[group];

	able = 0;
	for (int i = 0; i < g.aliveCount; ++i)
		able += g.members[i].status == kStatusOk;

	const Common::String &name = _messages.get(
		MonsterList::nameMessage(g.monsterId, g.identified, g.aliveCount > 1));
	return format(kMsgCountAndName, name, Common::String(), Common::String(), g.aliveCount);
}

void Battle::writeBack() {
	for (uint i = 0; i < _party.size(); ++i) {
		Character &c = _party.member(i);
		c.hp = _groups[0].members[i].hp;
		c.status = _groups[0].members[i].status;
	}
}

void Battle::updateEnemies() {
	for (int gi = 1; gi < kGroupCount; ++gi) {
		Group &g = _groups[gi];

		// The fallen are cleared away and the others close up
		int kept = 0;
		for (int i = 0; i < g.aliveCount; ++i) {
			if (g.members[i].status < kStatusDead && g.members[i].hp > 0)
				g.members[kept++] = g.members[i];
		}
		g.aliveCount = kept;

		for (int i = 0; i < kept; ++i) {
			Fighter &f = g.members[i];
			int chance = 0;
			if (f.status == kStatusAfraid)
				chance = g.enemy.hitDice.count * 10;
			else if (f.status == kStatusAsleep)
				chance = g.enemy.hitDice.count * 20;
			else if (f.status == kStatusParalysed)
				chance = g.enemy.hitDice.count * 7;

			if (chance && (int)_dice.roll(100) <= MIN(chance, 50))
				f.status = kStatusOk;
			f.hp += g.enemy.regeneration;
		}
	}

	// Groups with nobody left move behind the others
	for (int gi = 1; gi < kGroupCount - 1; ++gi) {
		if (_groups[gi].aliveCount > 0)
			continue;
		for (int next = gi + 1; next < kGroupCount; ++next) {
			if (_groups[next].aliveCount > 0) {
				SWAP(_groups[gi], _groups[next]);
				break;
			}
		}
	}

	if (enemyGroups() == 0 && _outcome == kUndecided)
		_outcome = kVictory;
}

void Battle::updateParty() {
	for (uint i = 0; i < _party.size(); ++i) {
		Character &c = _party.member(i);
		Fighter &f = _groups[0].members[i];
		if (f.status >= kStatusDead)
			continue;

		if (c.poison > 0)
			f.hp -= (f.hp < 51) ? c.poison : f.hp / 10;
		f.hp = MIN<int>(f.hp + c.healPoints, c.hpMax);
		if (f.hp < 1) {
			f.status = kStatusDead;
			f.hp = 0;
			continue;
		}

		int chance = 0;
		if (f.status == kStatusAfraid)
			chance = c.level * 5;
		else if (f.status == kStatusAsleep)
			chance = c.level * 10;
		if (chance && (int)_dice.roll(100) <= MIN(chance, 50))
			f.status = kStatusOk;
	}

	writeBack();
}

void Battle::sortParty() {
	// The healthiest go to the front, where the monsters reach them;
	// those of the same health keep their order. (The original first
	// puts everyone back in the order of the start of the fight.)
	const uint count = _party.size();
	Common::Array<uint> order;
	for (int status = kStatusOk; status <= kStatusLost; ++status) {
		for (uint i = 0; i < count; ++i) {
			if (_groups[0].members[i].status == status)
				order.push_back(i);
		}
	}
	if (order.size() != count)
		return;

	Fighter fighters[kMaxInGroup];
	bool drained[Party::kMaxMembers];
	for (uint i = 0; i < count; ++i) {
		fighters[i] = _groups[0].members[order[i]];
		drained[i] = _drained[order[i]];
	}
	for (uint i = 0; i < count; ++i) {
		_groups[0].members[i] = fighters[i];
		_drained[i] = drained[i];
	}

	_party.reorder(order);
}

void Battle::prepareRound() {
	updateEnemies();
	updateParty();
	sortParty();

	for (int g = 0; g < kGroupCount; ++g) {
		for (int i = 0; i < _groups[g].aliveCount; ++i) {
			if (_groups[g].members[i].silencedRounds > 0)
				--_groups[g].members[i].silencedRounds;
		}
	}

	// The strength of each side, and who recognises the monsters
	_partyStrength = _enemyStrength = 0;
	bool anyoneAble = false;
	for (uint i = 0; i < _party.size(); ++i) {
		const Character &c = _party.member(i);
		const Fighter &f = _groups[0].members[i];
		if (f.status == kStatusOk)
			_partyStrength += c.level;
		anyoneAble |= f.status < kStatusParalysed;

		if ((int)_dice.roll(99) < c.attributes[kIQ] + c.attributes[kPiety] + c.level)
			_groups[_dice.roll(4) + 1].identified = true;
	}

	for (int g = 1; g < kGroupCount; ++g) {
		for (int i = 0; i < _groups[g].aliveCount; ++i) {
			if (_groups[g].members[i].status == kStatusOk)
				_enemyStrength += _groups[g].enemy.hitDice.count;
		}
	}

	if (_outcome == kUndecided && !anyoneAble)
		_outcome = kDefeat;
}

bool Battle::canOrder(uint member) const {
	return member < _party.size() && _groups[0].members[member].status == kStatusOk &&
		_surprise != kMonstersSurprise;
}

bool Battle::mayDispel(uint member) const {
	const Character &c = _party.member(member);
	return c.classType == kClassPriest || (c.classType == kClassBishop && c.level >= 4) ||
		(c.classType == kClassLord && c.level >= 9);
}

void Battle::startOrders() {
	for (uint i = 0; i < _party.size(); ++i) {
		Fighter &f = _groups[0].members[i];
		f.action = kNotChosen;
		f.target = 0;

		if (!canOrder(i)) {
			f.initiative = -1;
			continue;
		}

		// The lower, the sooner
		int initiative = _dice.roll(10);
		switch (_party.member(i).attributes[kAgility]) {
		case 3:
			initiative += 3;
			break;
		case 4:
		case 5:
			initiative += 2;
			break;
		case 6:
		case 7:
			initiative += 1;
			break;
		case 15:
			initiative -= 1;
			break;
		case 16:
			initiative -= 2;
			break;
		case 17:
			initiative -= 3;
			break;
		case 18:
			initiative -= 4;
			break;
		default:
			break;
		}
		f.initiative = CLIP(initiative, 1, 10);
	}
}

void Battle::orderFight(uint member, int group) {
	_groups[0].members[member].action = kFight;
	_groups[0].members[member].target = group;
}

void Battle::orderDispel(uint member, int group) {
	_groups[0].members[member].action = kDispel;
	_groups[0].members[member].target = group;
}

void Battle::orderParry(uint member) {
	_groups[0].members[member].action = kParry;
}

void Battle::orderCast(uint member, int spell, int target) {
	_groups[0].members[member].action = spell;
	_groups[0].members[member].target = target;
}

bool Battle::tryRun() {
	int chance = 75;
	if (_party.size() < 4)
		chance = 95 - 5 * _party.size();
	if (_enemyStrength < _partyStrength)
		chance += 20;

	if (!_canRun || (int)_dice.roll(100) > chance) {
		// Caught: nobody in the party does anything this round
		for (uint i = 0; i < _party.size(); ++i) {
			_groups[0].members[i].initiative = -1;
			_groups[0].members[i].action = kParry;
		}
		return false;
	}

	for (int g = 1; g < kGroupCount; ++g)
		_groups[g].aliveCount = 0;
	_outcome = kRanAway;
	return true;
}

bool Battle::enemyCanAffect(int group, int target) {
	// A member protected against this kind of monster is left alone half the time
	const Character &c = _party.member(target);
	if (!((c.protectVs >> _groups[group].enemy.monsterClass) & 1))
		return true;
	return _dice.roll(100) < 50;
}

void Battle::enemiesChoose() {
	// Those of the party not dead, counted from the front
	int standing = 0;
	for (uint i = 0; i < _party.size(); ++i)
		standing += _groups[0].members[i].status < kStatusDead;

	for (int gi = 1; gi < kGroupCount; ++gi) {
		Group &g = _groups[gi];

		for (int i = 0; i < g.aliveCount; ++i) {
			Fighter &f = g.members[i];
			if (f.status != kStatusOk || _surprise == kPartySurprises || standing == 0) {
				f.initiative = -1;
				continue;
			}

			f.initiative = _dice.roll(8) + 2;
			f.target = _dice.roll(standing);
			f.action = kParry;

			if (!enemyCanAffect(gi, f.target))
				continue;

			// Spells come first, but not in a round of surprise
			if (_surprise == kNoSurprise)
				pickSpell(gi, i);

			if (f.action == kParry && g.enemy.breathType > 0 && _dice.roll(100) < 60)
				f.action = kBreath;
			if (f.action == kParry && (g.enemy.flags & Monster::kMayCallHelp) && g.aliveCount < 8 &&
					_dice.roll(100) < 75)
				f.action = kCallHelp;
			if (f.action == kParry && (g.enemy.flags & Monster::kMayRun) && _enemyStrength < _partyStrength &&
					_dice.roll(100) < 65)
				f.action = kRunAway;

			// A spell at someone whom spells cannot touch is wasted
			if (f.action > 0 && ((_party.member(f.target).resist >> kResistSpells) & 1))
				f.initiative = -1;

			// Otherwise fight, but only from near the front
			if (f.action == kParry && (i <= 4 - gi || (int)_dice.roll(100) >= 60 - 10 * gi)) {
				f.target %= 3;
				if (f.target < (int)_party.size() && enemyCanAffect(gi, f.target))
					f.action = kFight;
				else
					f.initiative = -1;
			}
		}
	}

	// Surprise lasts one round
	_surprise = kNoSurprise;
}

void Battle::memberAttacks(int member) {
	const Character &c = _party.member(member);
	const int gi = _groups[0].members[member].target;
	Group &g = _groups[gi];
	if (gi < 1 || gi >= kGroupCount || g.aliveCount == 0)
		return;

	Fighter &victim = g.members[member % g.aliveCount];
	if (victim.status >= kStatusDead)
		return;

	// Groups further back are harder to reach
	int need = 21 - g.enemy.armorClass - c.hitBonus + victim.acModifier - 3 * gi;
	need = CLIP(need, 1, 19);

	int hits = 0, damage = 0;
	for (int i = 0; i < c.swingCount; ++i) {
		if ((int)_dice.roll(20) >= need) {
			damage += rollDice(c.damage);
			++hits;
		}
	}
	if (victim.status == kStatusAsleep)
		damage *= 2;
	if ((c.doubleDamageVs >> g.enemy.monsterClass) & 1)
		damage *= 2;

	const Common::String verb = _messages.get(kMsgPartyVerbs + _dice.roll(5));
	const uint message = !hits ? kMsgMisses : (hits == 1 ? kMsgHitsOnce : kMsgHitsTimes);
	say(format(message, c.name, verb, monsterName(gi), hits, damage));

	victim.hp -= damage;

	// Ninjas and certain weapons may kill outright
	if (c.criticalHits && damage > 0 && (int)_dice.roll(100) < MIN(c.level * 2, 50) &&
			(int)_dice.roll(35) > g.enemy.hitDice.count + 10) {
		victim.hp = 0;
		say(format(kMsgDecapitated, monsterName(gi)));
		victim.status = kStatusDead;
		return;
	}

	if (victim.hp < 1) {
		victim.hp = 0;
		victim.status = kStatusDead;
		say(format(kMsgIsKilled, monsterName(gi)));
	}
}

void Battle::killMember(int victim, uint message) {
	Fighter &f = _groups[0].members[victim];
	f.hp = 0;
	f.status = kStatusDead;
	say(format(message, _party.member(victim).name));
}

void Battle::monsterAttacks(int gi, int index) {
	Group &g = _groups[gi];
	const int target = g.members[index].target;
	if (target < 0 || target >= (int)_party.size())
		return;

	Fighter &victim = _groups[0].members[target];
	const Character &c = _party.member(target);
	if (victim.status >= kStatusDead)
		return;

	int need = 20 - c.armorClass - g.enemy.hitDice.count + _maze.acBonus + victim.acModifier;
	if (victim.action == kParry)
		need += 2;
	need = CLIP(need, 1, 19);

	int hits = 0, damage = 0;
	for (int i = 0; i < MIN<int>(g.enemy.attackCount, Monster::kMaxAttacks); ++i) {
		if ((int)_dice.roll(20) >= need) {
			damage += rollDice(g.enemy.attackDamage[i]);
			++hits;
		}
	}
	if (victim.status == kStatusAsleep)
		damage *= 2;

	const Common::String verb = _messages.get(kMsgMonsterVerbs + _dice.roll(5));
	const uint message = !hits ? kMsgMisses : (hits == 1 ? kMsgHitsOnce : kMsgHitsTimes);
	say(format(message, monsterName(gi), verb, c.name, hits, damage));

	if (damage > 0) {
		hitSideEffects(gi, target);
		if (victim.status < kStatusDead) {
			victim.hp -= damage;
			if (victim.hp < 1)
				killMember(target, kMsgIsKilled);
		}
	}
}

void Battle::hitSideEffects(int gi, int victim) {
	const Monster &enemy = _groups[gi].enemy;

	if (enemy.flags & Monster::kPoisons)
		inflict(gi, victim, kAfflictPoison, kSaveDeath, kResistPoison);
	if (enemy.flags & Monster::kParalyses)
		inflict(gi, victim, kAfflictParalysis, kSaveDeath, -1);
	if (enemy.flags & Monster::kStones)
		inflict(gi, victim, kAfflictStoning, kSaveStoning, kResistStoning);
	if (enemy.levelDrain > 0)
		drainLevels(gi, victim);
	if (enemy.flags & Monster::kKillsOutright)
		inflict(gi, victim, kAfflictDeath, kSaveDeath, -1);
}

bool Battle::inflict(int gi, int victim, int kind, int save, int resistBit) {
	Character &c = _party.member(victim);
	Fighter &f = _groups[0].members[victim];

	if ((int)_dice.roll(20) <= c.savingThrows[save])
		return false;
	if (kind == kAfflictDeath && (int)_dice.roll(100) > MIN(_groups[gi].enemy.hitDice.count * 2, 50))
		return false;
	if (resistBit >= 0 && ((c.resist >> resistBit) & 1))
		return false;
	if (f.status >= kStatusDead)
		return false;

	switch (kind) {
	case kAfflictPoison:
		c.poison = 1;
		say(isWord(_messages.get(kMsgIs), c.name, _messages.get(kMsgPoisoned)));
		break;
	case kAfflictParalysis:
		f.status = MAX<int>(f.status, kStatusParalysed);
		say(isWord(_messages.get(kMsgIs), c.name, _messages.get(kMsgParalyzed)));
		break;
	case kAfflictStoning:
		f.status = MAX<int>(f.status, kStatusStoned);
		say(isWord(_messages.get(kMsgIs), c.name, _messages.get(kMsgStoned)));
		break;
	default:
		killMember(victim, kMsgDecapitated);
		break;
	}

	return true;
}

void Battle::drainLevels(int gi, int victim) {
	Character &c = _party.member(victim);
	Fighter &f = _groups[0].members[victim];
	const int levels = _groups[gi].enemy.levelDrain;

	// Once in a fight, and not with the resistance
	if (_drained[victim] || ((c.resist >> kResistDrain) & 1))
		return;

	say(format(levels == 1 ? kMsgDrainedOne : kMsgDrainedMany, c.name, Common::String(), Common::String(), levels));

	c.level -= levels;
	f.level = c.level;
	if (c.level < 1) {
		c.level = 0;
		f.hp = 0;
		f.status = kStatusLost;
	} else {
		c.hpMax = MAX((c.hpMax / MAX<int>(c.maxLevel, 1)) * c.level, 1);
		c.maxLevel = c.level;
		f.hp = MIN(f.hp, c.hpMax);
		_drained[victim] = true;
	}
}

void Battle::dispel(int member) {
	const Character &c = _party.member(member);
	const int gi = _groups[0].members[member].target;
	Group &g = _groups[gi];

	int chance = 50 + 5 * c.level - 10 * g.enemy.hitDice.count;
	if (c.classType == kClassBishop)
		chance -= 20;
	else if (c.classType != kClassPriest)
		chance -= 40;
	chance = MAX(chance, 5);

	// Only the undead can be dispelled. Those that are give no
	// experience, which is paid by the count at the start.
	int count = 0;
	for (int i = 0; i < g.aliveCount; ++i) {
		Fighter &f = g.members[i];
		if (f.status == kStatusOk && (int)_dice.roll(100) < chance && g.enemy.monsterClass == kClassUndead) {
			f.status = kStatusDead;
			f.hp = 0;
			--g.startCount;
			++count;
		}
	}

	const uint message = !count ? kMsgDispelFails : (count == 1 ? kMsgDispelsOne : kMsgDispelsMany);
	say(format(message, c.name, Common::String(), Common::String(), count));
}

void Battle::callForHelp(int gi, int index) {
	Group &g = _groups[gi];

	if (g.aliveCount >= kMaxInGroup || (int)_dice.roll(200) > g.enemy.hitDice.count * 10) {
		say(format(kMsgNoHelpComes, monsterName(gi)));
		return;
	}

	// One more joins; it does nothing this round, and it pays experience too
	Fighter &f = g.members[g.aliveCount++];
	f = g.members[index];
	f.hp = rollDice(g.enemy.hitDice);
	f.status = kStatusOk;
	f.acModifier = 0;
	f.initiative = -1;
	f.action = kParry;
	++g.startCount;

	say(format(kMsgHelpAppears, monsterName(gi)));
}

void Battle::monsterRuns(int gi, int index) {
	Group &g = _groups[gi];
	say(format(kMsgRunsAway, monsterName(gi)));

	--g.startCount;
	g.members[index].status = kStatusDead;   // gone; cleared away with the fallen
	g.members[index].hp = 0;
}

void Battle::breathe(int gi, int index) {
	Group &g = _groups[gi];
	say(format(kMsgBreathes, monsterName(gi)));

	for (uint i = 0; i < _party.size(); ++i) {
		const Character &c = _party.member(i);
		Fighter &f = _groups[0].members[i];
		if (f.status >= kStatusDead)
			continue;

		// Half the breather's hit points; halved by a saving throw and by resistance
		int damage = g.members[index].hp / 2;
		if ((int)_dice.roll(20) >= c.savingThrows[kSaveBreath])
			damage = (damage + 1) / 2;
		if ((c.resist >> g.enemy.breathType) & 1)
			damage = (damage + 1) / 2;
		damage = MAX(damage, 1);

		say(format(kMsgTakesDamage, c.name, Common::String(), Common::String(), damage));
		f.hp -= damage;
		if (f.hp < 1)
			killMember(i, kMsgIsKilled);
	}
}

void Battle::runRound(Common::Array<Common::String> &lines) {
	_lines = &lines;

	// The spells ordered are paid for now, cast or not
	for (uint i = 0; i < _party.size(); ++i) {
		if (_groups[0].members[i].action > 0)
			Spells::spend(_party.member(i), _groups[0].members[i].action);
	}

	enemiesChoose();

	for (int step = 1; step <= 10; ++step) {
		for (int gi = 0; gi < kGroupCount; ++gi) {
			Group &g = _groups[gi];

			for (int i = 0; i < g.aliveCount; ++i) {
				const Fighter &f = g.members[i];
				if (f.status != kStatusOk || f.initiative != step)
					continue;

				if (_outcome != kUndecided)
					continue;

				if (f.action > 0) {
					cast(gi, i);
				} else if (gi == 0) {
					if (f.action == kFight)
						memberAttacks(i);
					else if (f.action == kDispel)
						dispel(i);
				} else {
					switch (f.action) {
					case kFight:
						monsterAttacks(gi, i);
						break;
					case kBreath:
						breathe(gi, i);
						break;
					case kCallHelp:
						callForHelp(gi, i);
						break;
					case kRunAway:
						monsterRuns(gi, i);
						break;
					default:
						break;
					}
				}
			}
		}
	}

	writeBack();
	_lines = nullptr;
}

Common::String Battle::fighterName(int group, int index) const {
	return group == 0 ? _party.member(index).name : monsterName(group);
}

void Battle::pickSpell(int gi, int index) {
	Group &g = _groups[gi];
	Fighter &f = g.members[index];

	if (g.enemy.mageSpellLevel > 0 && (g.enemy.monsterClass == 1 || _dice.roll(100) < 75)) {
		// Often a spell of a lower level than the highest
		int level = g.enemy.mageSpellLevel;
		while (level > 1 && _dice.roll(100) > 70)
			--level;

		// Now and then the group loses its highest spell level
		// (the place of this roll among the others is not certain)
		if (_dice.roll(g.aliveCount + 2) == 0)
			--g.enemy.mageSpellLevel;

		// One time in three the first of the two, else the second
		static const uint8 CHOICES[7][2] = {
			{ HALITO, KATINO }, { HALITO, DILTO }, { MAHALITO, MOLITO }, { LAHALITO, DALTO },
			{ MADALTO, LAHALITO }, { ZILWAN, MADALTO }, { TILTOWAIT, TILTOWAIT }
		};
		f.action = CHOICES[CLIP(level, 1, 7) - 1][_dice.roll(100) < 34 ? 0 : 1];
	}

	if (f.action == kParry && g.enemy.priestSpellLevel > 0 &&
			(g.enemy.monsterClass == 2 || _dice.roll(100) < 75)) {
		static const uint8 CHOICES[7][2] = {
			{ BADIOS, BADIOS }, { MONTINO, MONTINO }, { BADIAL, BADIOS }, { BADIAL, BADIAL },
			{ BADI, BADIALMA }, { MABADI, LORTO }, { MABADI, MABADI }
		};
		f.action = CHOICES[CLIP<int>(g.enemy.priestSpellLevel, 1, 7) - 1][_dice.roll(100) < 34 ? 0 : 1];
	}
}

void Battle::cast(int gi, int index) {
	Fighter &f = _groups[gi].members[index];
	const int spell = f.action;
	const Common::String who = fighterName(gi, index);
	const Common::String what = Spells::name(_messages, spell);

	if (f.silencedRounds > 0) {
		say(format(kMsgCannotSpeak, who, what));
		return;
	}
	if (_maze.spellsFizzle) {
		say(format(kMsgFizzles, who, what));
		return;
	}
	say(format(kMsgCasts, who, what));

	// The party aims at a group, and each member at the monster of it
	// that stands opposite; a monster aims at one of the party
	int targetGroup = 0, targetMember = f.target;
	if (gi == 0) {
		targetGroup = f.target;
		targetMember = 0;
		if (targetGroup > 0 && targetGroup < kGroupCount && _groups[targetGroup].aliveCount > 0)
			targetMember = index % _groups[targetGroup].aliveCount;
	}

	applyMage(spell, gi, index, targetGroup, targetMember, f.level);
	applyPriest(spell, gi, index, targetGroup, targetMember, f.level);
}

bool Battle::groupResists(int group) {
	if (group < 1)
		return false;

	if ((int)_dice.roll(100) < _groups[group].enemy.magicResistance) {
		say(format(kMsgResists, monsterName(group)));
		return true;
	}
	return false;
}

void Battle::spellKill(int group, int index) {
	Fighter &f = _groups[group].members[index];
	f.hp = 0;
	if (f.status < kStatusDead)
		f.status = kStatusDead;
}

void Battle::spellDamage(int group, int index, int damage) {
	Fighter &f = _groups[group].members[index];

	damage = (damage < 0) ? 0 : MAX(damage, 1);
	if (f.status >= kStatusDead || groupResists(group))
		return;

	f.hp -= damage;
	if (f.hp < 1)
		spellKill(group, index);

	say(format(f.hp < 1 ? kMsgSpellKilled : kMsgTakesDamage, fighterName(group, index),
		Common::String(), Common::String(), damage));
}

void Battle::spellDamageDice(int group, int index, int count, int sides) {
	const Dice3 dice = { (int16)count, (int16)sides, 0 };
	spellDamage(group, index, rollDice(dice));
}

void Battle::spellDamageGroup(int group, int count, int sides, int element) {
	Group &g = _groups[group];

	for (int i = 0; i < g.aliveCount; ++i) {
		// Half the dice against one who withstands this kind of damage
		const uint resist = (group == 0) ? _party.member(i).resist : g.enemy.elementResist;
		if (element != kElementNone && ((resist >> element) & 1))
			spellDamageDice(group, i, count / 2 + 1, sides);
		else
			spellDamageDice(group, i, count, sides);
	}
}

void Battle::statusAttack(int group, int index, int resistChance, uint word, int effect) {
	if (groupResists(group))
		return;

	Fighter &f = _groups[group].members[index];
	resistChance = MAX(resistChance, 5);

	uint message = kMsgIsNotWord;
	if ((int)_dice.roll(100) >= resistChance) {
		message = kMsgIsWord;
		switch (effect) {
		case kEffectSleep:
			f.status = kStatusAsleep;
			break;
		case kEffectSilence:
			f.silencedRounds = _dice.roll(4) + 2;
			break;
		default:
			spellKill(group, index);
			break;
		}
	}

	say(isWord(_messages.get(message), fighterName(group, index), _messages.get(word)));
}

void Battle::adjustAC(int group, int by, int first, int last) {
	for (int i = first; i <= last && i < kMaxInGroup; ++i)
		_groups[group].members[i].acModifier += by;
}

void Battle::heal(int member, int count, int sides) {
	if (member < 0 || member >= (int)_party.size())
		return;

	const Dice3 dice = { (int16)count, (int16)sides, 0 };
	const int amount = rollDice(dice);
	Fighter &f = _groups[0].members[member];
	const Character &c = _party.member(member);

	f.hp = MIN<int>(f.hp + amount, c.hpMax);
	say(format(f.hp == c.hpMax ? kMsgCompletelyHealed : kMsgRestored, c.name,
		Common::String(), Common::String(), amount));
}

void Battle::applyMage(int spell, int gi, int index, int targetGroup, int targetMember, int casterLevel) {
	const int lastMember = (int)_party.size() - 1;

	switch (spell) {
	case HALITO:
		spellDamageDice(targetGroup, targetMember, 1, 8);
		break;
	case MOGREF:
		adjustAC(0, 2, index, index);
		break;
	case KATINO: {
		Group &g = _groups[targetGroup];
		for (int i = 0; i < g.aliveCount; ++i) {
			if (g.members[i].status >= kStatusAsleep)
				continue;
			int chance = (g.members[i].level - casterLevel) * 5 + 50;
			// Monsters that cannot be put to sleep never are
			if (targetGroup != 0 && !(g.enemy.flags & Monster::kCanSleep))
				chance += 100;
			statusAttack(targetGroup, i, chance, kMsgSlept, kEffectSleep);
		}
		break;
	}
	case DILTO:
		adjustAC(targetGroup, -2, 0, _groups[targetGroup].aliveCount - 1);
		break;
	case SOPIC:
		adjustAC(0, 4, index, index);
		break;
	case MAHALITO:
		spellDamageGroup(targetGroup, 4, 6, kElementFire);
		break;
	case MOLITO:
		spellDamageGroup(targetGroup, 3, 6, kElementNone);
		break;
	case MORLIS:
		adjustAC(targetGroup, -3, 0, _groups[targetGroup].aliveCount - 1);
		break;
	case DALTO:
		spellDamageGroup(targetGroup, 6, 6, kElementCold);
		break;
	case LAHALITO:
		spellDamageGroup(targetGroup, 6, 6, kElementFire);
		break;
	case MAMORLIS:
		for (int g = 1; g < kGroupCount; ++g)
			adjustAC(g, -3, 0, _groups[g].aliveCount - 1);
		break;
	case MAKANITO:
		// Kills every group that is not undead and below level 8
		for (int g = 1; g < kGroupCount; ++g) {
			if (_groups[g].aliveCount > 0 && _groups[g].enemy.monsterClass != kClassUndead &&
					_groups[g].enemy.hitDice.count < 8) {
				for (int i = 0; i < _groups[g].aliveCount; ++i)
					spellDamage(g, i, _groups[g].members[i].hp);
			}
		}
		break;
	case MADALTO:
		spellDamageGroup(targetGroup, 8, 8, kElementCold);
		break;
	case LAKANITO:
		// Smothers the living
		if (_groups[targetGroup].enemy.monsterClass != kClassUndead) {
			for (int i = 0; i < _groups[targetGroup].aliveCount; ++i) {
				if (_groups[targetGroup].members[i].status < kStatusDead)
					statusAttack(targetGroup, i, _groups[targetGroup].enemy.hitDice.count * 6,
						kMsgSlain, kEffectDeath);
			}
		}
		break;
	case ZILWAN:
		// Dissolves one of the undead
		if (_groups[targetGroup].enemy.monsterClass == kClassUndead)
			spellDamageDice(targetGroup, targetMember, 10, 200);
		break;
	case MASOPIC:
		adjustAC(0, 4, 0, lastMember);
		break;
	case HAMAN:
	case MAHAMAN:
		// TODO: the boons of the gods
		say(_messages.get(kMsgDoesNotWork));
		break;
	case MALOR:
		// Out of the fight to a place at random, perhaps some levels down
		_maze.x = _dice.roll(MazeLevel::kSize);
		_maze.y = _dice.roll(MazeLevel::kSize);
		while (_dice.roll(100) <= 29)
			++_maze.level;
		while (_dice.roll(100) <= 9)
			++_maze.level;
		// TODO: the original turns back to level 1 at the level of the last fight
		if (_maze.level > 10)
			_maze.level = 1;
		_outcome = kTeleported;
		break;
	case TILTOWAIT:
		if (gi == 0) {
			for (int g = 1; g < kGroupCount; ++g)
				spellDamageGroup(g, 10, 15, kElementNone);
		} else {
			spellDamageGroup(0, 10, 15, kElementNone);
		}
		break;
	default:
		break;
	}
}

void Battle::applyPriest(int spell, int gi, int index, int targetGroup, int targetMember, int casterLevel) {
	const int lastMember = (int)_party.size() - 1;
	// A spell for one of the party carries the member where the others carry the group
	const int member = targetGroup;
	const bool validMember = member >= 0 && member < (int)_party.size();

	switch (spell) {
	case KALKI:
		adjustAC(0, 1, 0, lastMember);
		break;
	case DIOS:
		heal(member, 1, 8);
		break;
	case BADIOS:
		spellDamageDice(targetGroup, targetMember, 1, 8);
		break;
	case MILWA:
		_maze.lightDuration += 30 + _dice.roll(15);
		break;
	case PORFIC:
		adjustAC(0, 4, index, index);
		break;
	case MATU:
		adjustAC(0, 2, 0, lastMember);
		break;
	case MANIFO: {
		Group &g = _groups[targetGroup];
		for (int i = 0; i < g.aliveCount; ++i) {
			if (g.members[i].status < kStatusParalysed)
				statusAttack(targetGroup, i, (g.members[i].level - casterLevel) * 5 + 50, kMsgHeld, kEffectSleep);
		}
		break;
	}
	case MONTINO: {
		Group &g = _groups[targetGroup];
		for (int i = 0; i < g.aliveCount; ++i)
			statusAttack(targetGroup, i, (g.members[i].level - casterLevel) * 5 + 40, kMsgSilenced, kEffectSilence);
		break;
	}
	case LOMILWA:
		_maze.lightDuration = 10000;
		break;
	case DIALKO:
		if (validMember) {
			Fighter &f = _groups[0].members[member];
			const bool cured = f.status == kStatusParalysed || f.status == kStatusAsleep;
			if (cured)
				f.status = kStatusOk;
			say(format(cured ? kMsgCured : kMsgNotCured, _party.member(member).name));
		}
		break;
	case LATUMAPIC:
		for (int g = 1; g < kGroupCount; ++g)
			_groups[g].identified = true;
		_maze.monstersIdentified = 0x7fff;
		break;
	case BAMATU:
		adjustAC(0, 4, 0, lastMember);
		break;
	case DIAL:
		heal(member, 2, 8);
		break;
	case BADIAL:
		spellDamageDice(targetGroup, targetMember, 2, 8);
		break;
	case LATUMOFIS:
		if (validMember) {
			say(format(kMsgCured, _party.member(member).name));
			_party.member(member).poison = 0;
		}
		break;
	case MAPORFIC:
		_maze.acBonus = 2;
		break;
	case DIALMA:
		heal(member, 3, 8);
		break;
	case BADIALMA:
		spellDamageDice(targetGroup, targetMember, 3, 8);
		break;
	case LITOKAN:
		spellDamageGroup(targetGroup, 3, 8, kElementFire);
		break;
	case KANDI:
	case DI:
	case KADORTO:
		// These are for the camp
		say(_messages.get(kMsgDoesNotWork));
		break;
	case BADI: {
		const int chance = (_groups[targetGroup].members[targetMember].level - casterLevel) * 5 + 50;
		statusAttack(targetGroup, targetMember, MAX(chance, 25), kMsgSlain, kEffectDeath);
		break;
	}
	case LORTO:
		spellDamageGroup(targetGroup, 6, 6, kElementNone);
		break;
	case MADI:
		if (validMember) {
			Fighter &f = _groups[0].members[member];
			f.hp = _party.member(member).hpMax;
			if (f.status < kStatusDead)
				f.status = kStatusOk;
			_party.member(member).poison = 0;
			heal(member, 1, 1);
		}
		break;
	case MABADI:
		// Leaves the victim with one to eight hit points
		spellDamage(targetGroup, targetMember,
			_groups[targetGroup].members[targetMember].hp - (int)_dice.roll(8) - 1);
		break;
	case LOKTOFEIT: {
		// Back to the castle, if it works: at the price of all items and most gold
		if (MIN(casterLevel + 65, 95) < (int)_dice.roll(100)) {
			say(_messages.get(kMsgNothingHappens));
			break;
		}
		for (uint i = 0; i < _party.size(); ++i) {
			Character &c = _party.member(i);
			c.itemCount = 0;
			c.gold %= 10000;
		}
		_outcome = kRecalled;
		break;
	}
	case MALIKTO:
		for (int g = 1; g < kGroupCount; ++g)
			spellDamageGroup(g, 12, 6, kElementNone);
		break;
	default:
		break;
	}
}

int64 Battle::awardExperience() {
	uint survivors = 0;
	for (uint i = 0; i < _party.size(); ++i)
		survivors += _party.member(i).status == kStatusOk;
	if (!survivors)
		return 0;

	// TODO: those drained in the fight have their experience set to the
	// least for their new level

	int64 total = 0;
	for (int g = 1; g < kGroupCount; ++g) {
		if (_groups[g].monsterId >= 0)
			total += _groups[g].enemy.experience() * MAX<int>(_groups[g].startCount, 0);
	}

	const int64 share = total / survivors;
	for (uint i = 0; i < _party.size(); ++i) {
		Character &c = _party.member(i);
		if (c.status == kStatusOk)
			c.experience = MIN<int64>(c.experience + share, 999999999999LL);
	}

	return share;
}

} // End of namespace Wizardry
