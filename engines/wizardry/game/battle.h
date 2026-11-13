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

#ifndef WIZARDRY_GAME_BATTLE_H
#define WIZARDRY_GAME_BATTLE_H

#include "wizardry/game/maze.h"
#include "wizardry/game/monsters.h"
#include "wizardry/game/spells.h"
#include "wizardry/resources/messages.h"

namespace Wizardry {

/**
 * A fight, by the rules of Wizardry I.
 *
 * A round has ten steps. Everyone able to act has an initiative from 1 to
 * 10 and acts in that step, the party before the monsters. The party gives
 * its orders before the round; the monsters choose when it starts.
 *
 * An action above zero is the casting of the spell of that number.
 *
 * Not in yet: the use of items, friendly monsters, and the two spells that
 * ask the gods for a boon (HAMAN and MAHAMAN).
 */
class Battle {
public:
	enum {
		kGroupCount = 5,      ///< group 0 is the party
		kMaxInGroup = 9
	};

	enum Action {
		kParry = 0,
		kFight = -1,
		kRunAway = -2,
		kBreath = -3,
		kCallHelp = -4,
		kDispel = -5,
		kNotChosen = -999
	};

	enum Surprise {
		kNoSurprise,
		kPartySurprises,     ///< the monsters do nothing in the first round
		kMonstersSurprise    ///< the party gives no orders in the first round
	};

	enum Outcome {
		kUndecided,
		kVictory,
		kRanAway,
		kDefeat,      ///< nobody in the party can act any more
		kRecalled,    ///< LOKTOFEIT: back in the castle, without items and most gold
		kTeleported   ///< MALOR: somewhere else in the maze
	};

	struct Fighter {
		int16 target;
		int16 action;
		int16 initiative;   ///< step of the round, or -1 for nothing
		int16 hp;
		int16 acModifier;
		int16 silencedRounds;
		int16 level;
		int16 status;
	};

	struct Group {
		bool identified;
		int16 aliveCount;
		int16 startCount;   ///< what experience is paid for
		int16 monsterId;
		Fighter members[kMaxInGroup];
		Monster enemy;
	};

private:
	Party &_party;
	const MonsterList &_monsters;
	const MessageFile &_messages;
	MazeState &_maze;
	Dice &_dice;

	Group _groups[kGroupCount];
	int _surprise = kNoSurprise;
	Outcome _outcome = kUndecided;
	bool _canRun = true;
	bool _drained[Party::kMaxMembers];
	int _partyStrength = 0, _enemyStrength = 0;
	Common::Array<Common::String> *_lines = nullptr;

	int rollDice(const Dice3 &dice);
	void loadEnemyGroup(int monsterId, int group);
	void say(const Common::String &line);
	Common::String format(uint message, const Common::String &name, const Common::String &verb = Common::String(),
		const Common::String &victim = Common::String(), int hits = 0, int damage = 0) const;
	Common::String monsterName(int group) const;
	void writeBack();

	void updateEnemies();
	void updateParty();
	void sortParty();
	void enemiesChoose();
	bool enemyCanAffect(int group, int target);

	void memberAttacks(int member);
	void monsterAttacks(int group, int index);
	void hitSideEffects(int group, int victim);
	bool inflict(int group, int victim, int kind, int save, int resistBit);
	void drainLevels(int group, int victim);
	void killMember(int victim, uint message);
	void dispel(int member);
	void callForHelp(int group, int index);
	void monsterRuns(int group, int index);
	void breathe(int group, int index);

	// ---- spells
	void pickSpell(int group, int index);
	void cast(int group, int index);
	void applyMage(int spell, int group, int index, int targetGroup, int targetMember, int casterLevel);
	void applyPriest(int spell, int group, int index, int targetGroup, int targetMember, int casterLevel);
	Common::String fighterName(int group, int index) const;
	bool groupResists(int group);
	void spellKill(int group, int index);
	void spellDamage(int group, int index, int damage);
	void spellDamageDice(int group, int index, int count, int sides);
	void spellDamageGroup(int group, int count, int sides, int element);
	void statusAttack(int group, int index, int resistChance, uint word, int effect);
	void adjustAC(int group, int by, int first, int last);
	void heal(int member, int count, int sides);

public:
	Battle(Party &party, const MonsterList &monsters, const MessageFile &messages,
		MazeState &maze, Dice &dice);

	/**
	 * Sets up the fight
	 * @param canRun  False for the fixed encounters, which hold the party
	 */
	void begin(int monsterId, bool canRun);

	/**
	 * What happens before the orders of a round: the fallen are cleared
	 * away, poison and healing work, some wake up. Decides the fight if
	 * one side is finished.
	 */
	void prepareRound();

	Outcome outcome() const { return _outcome; }
	int surprise() const { return _surprise; }
	const Group &group(int index) const { return _groups[index]; }

	/** Number of groups of monsters with anyone left */
	int enemyGroups() const;

	/**
	 * One line of the list of monsters: "3 SLIMES", and how many of them
	 * can act
	 */
	Common::String groupLine(int group, int &able) const;

	// ---- the party's orders

	/** True if a member gets to give an order this round */
	bool canOrder(uint member) const;
	/** Only the first three stand close enough to fight */
	bool mayFight(uint member) const { return member < 3; }
	/** Priests, bishops from level 4, lords from level 9 */
	bool mayDispel(uint member) const;

	/** Clears all orders and rolls everyone's initiative */
	void startOrders();
	void orderFight(uint member, int group);
	void orderDispel(uint member, int group);
	void orderParry(uint member);

	/**
	 * Orders a spell
	 * @param target  A member or a group, as the spell needs; else ignored
	 */
	void orderCast(uint member, int spell, int target);

	/**
	 * Tries to run. If it works the fight is over; if not, nobody in the
	 * party does anything this round.
	 */
	bool tryRun();

	/**
	 * Plays the round
	 * @param lines  Receives what is told of it, in order
	 */
	void runRound(Common::Array<Common::String> &lines);

	/**
	 * After a victory: shares the experience among those still standing
	 * @returns what each of them got
	 */
	int64 awardExperience();
};

} // End of namespace Wizardry

#endif
