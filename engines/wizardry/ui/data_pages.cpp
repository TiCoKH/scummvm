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

#include "wizardry/ui/data_pages.h"
#include "wizardry/game/spells.h"

namespace Wizardry {

namespace {

const char *const ITEM_KINDS[] = {
	"WEAPON", "ARMOR", "SHIELD", "HELMET", "GAUNTLETS", "SPECIAL", "MISC"
};

const char *const ALIGNMENTS[] = { "ANY", "GOOD", "NEUTRAL", "EVIL" };

const char *const MONSTER_CLASSES[] = {
	"FIGHTER", "MAGE", "PRIEST", "THIEF", "MIDGET", "GIANT", "MYTHICAL",
	"DRAGON", "ANIMAL", "WERE", "UNDEAD", "DEMON", "INSECT", "ENCHANTED"
};

const char *const MONSTER_FLAGS[] = {
	"STONE", "POISON", "PARALYZE", "CRITICAL", "SLEEPS", "RUNS", "CALLS"
};

const char *const WALLS[] = { "OPEN", "WALL", "DOOR", "HIDDEN DOOR" };

const char *const SQUARE_NAMES[] = {
	"NORMAL", "STAIRS", "PIT", "CHUTE", "SPINNER", "DARK", "TRANSFER",
	"DAMAGE", "ELEVATOR", "ROCK", "FIZZLE", "SPECIAL", "ENCOUNTER"
};

/** The character for each kind of square on the map */
const char SQUARE_CHARS[] = ".SPCODTHERF!M";

/** The classes by their first letters, in the order of the class bits */
const char CLASS_LETTERS[] = "FMPTBSLN";

Common::String dice(const Dice3 &d) {
	if (d.count == 0 && d.bonus == 0)
		return "-";
	Common::String s = Common::String::format("%dD%d", d.count, d.sides);
	if (d.bonus)
		s += Common::String::format("%+d", d.bonus);
	return s;
}

Common::String named(const char *const *names, uint count, int value) {
	if (value >= 0 && (uint)value < count)
		return names[value];
	return Common::String::format("%d", value);
}

/** The bits that are set, as their numbers */
Common::String bits(uint value) {
	Common::String s;
	for (int b = 0; b < 16; ++b) {
		if ((value >> b) & 1)
			s += Common::String::format(s.empty() ? "%d" : ",%d", b);
	}
	return s.empty() ? "-" : s;
}

/** Prints lines from the third row on, each cut to the window */
void printLines(TextWindow &window, const Common::Array<Common::String> &lines) {
	for (uint i = 0; i < lines.size() && (int)i + 2 < window.height() - 1; ++i)
		window.PRINTSTR(0, 2 + i, Common::String(lines[i].c_str(), MIN<uint>(lines[i].size(), window.width())));
}

} // End of anonymous namespace

void DataPages::drawItem(TextWindow &window, const MessageFile &messages, const ItemList &items, uint id) {
	if (!items.size()) {
		window.printCentered(1, "NO ITEMS IN THIS SCENARIO");
		return;
	}
	const Item &item = items[id];
	window.printCentered(0, Common::String::format("ITEM %u OF %u", id, items.size() - 1));

	Common::String classes;
	for (int c = 0; c < 8; ++c)
		classes += ((item.classUse >> c) & 1) ? CLASS_LETTERS[c] : '-';

	Common::Array<Common::String> lines;
	lines.push_back(messages.get(ItemList::nameMessage(id, true)));
	lines.push_back("UNKNOWN AS " + messages.get(ItemList::nameMessage(id, false)));
	lines.push_back("KIND      " + named(ITEM_KINDS, ARRAYSIZE(ITEM_KINDS), item.kind));
	lines.push_back("ALIGNMENT " + named(ALIGNMENTS, ARRAYSIZE(ALIGNMENTS), item.alignment) +
		(item.cursed ? "  CURSED" : ""));
	lines.push_back("CLASSES   " + classes);
	lines.push_back(Common::String::format("PRICE     %lld", (long long)item.price));
	lines.push_back(Common::String::format("STOCK     %d", item.stock));
	lines.push_back(Common::String::format("AC        %d   HIT %+d", item.armorMod, item.hitMod));
	lines.push_back("DAMAGE    " + dice(item.damage) + Common::String::format("   SWINGS %d", item.swings) +
		(item.criticalHits ? "  CRIT" : ""));
	lines.push_back("SPELL     " + (item.spellPower > 0 ?
		Spells::name(messages, item.spellPower) : Common::String("-")));
	lines.push_back(Common::String::format("POWER     %d", item.specialPower));
	lines.push_back(Common::String::format("BREAKS    %d%% INTO %d", item.changeChance, item.changeTo));
	lines.push_back(Common::String::format("HEALS     %d", item.healPoints));
	lines.push_back("PROTECTS  " + bits(item.protectVs));
	lines.push_back("RESISTS   " + bits(item.resist));
	lines.push_back("DOUBLE VS " + bits(item.doubleDamageVs));
	printLines(window, lines);
}

void DataPages::drawMonster(TextWindow &window, const MessageFile &messages, const MonsterList &monsters, uint id) {
	if (!monsters.size()) {
		window.printCentered(1, "NO MONSTERS IN THIS SCENARIO");
		return;
	}
	const Monster &m = monsters[id];
	window.printCentered(0, Common::String::format("MONSTER %u OF %u", id, monsters.size() - 1));

	Common::String attacks;
	for (int i = 0; i < m.attackCount && i < Monster::kMaxAttacks; ++i)
		attacks += (i ? " " : "") + dice(m.attackDamage[i]);

	Common::String flags;
	for (int b = 0; b < (int)ARRAYSIZE(MONSTER_FLAGS); ++b) {
		if ((m.flags >> b) & 1)
			flags += Common::String(flags.empty() ? "" : " ") + MONSTER_FLAGS[b];
	}

	Common::Array<Common::String> lines;
	lines.push_back(messages.get(MonsterList::nameMessage(id, true, false)) + " / " +
		messages.get(MonsterList::nameMessage(id, true, true)));
	lines.push_back(messages.get(MonsterList::nameMessage(id, false, false)) + " / " +
		messages.get(MonsterList::nameMessage(id, false, true)));
	lines.push_back("CLASS     " + named(MONSTER_CLASSES, ARRAYSIZE(MONSTER_CLASSES), m.monsterClass) +
		Common::String::format("   PICTURE %d", m.pictureId & 0xff));
	lines.push_back("GROUP     " + dice(m.groupSize) + "   HIT DICE " + dice(m.hitDice));
	lines.push_back(Common::String::format("AC        %d   EXP %lld", m.armorClass, (long long)m.experience()));
	lines.push_back("ATTACKS   " + (attacks.empty() ? Common::String("-") : attacks));
	lines.push_back(Common::String::format("DRAIN     %d   REGEN %d", m.levelDrain, m.regeneration));
	lines.push_back(Common::String::format("SPELLS    MAGE %d  PRIEST %d", m.mageSpellLevel, m.priestSpellLevel));
	lines.push_back(Common::String::format("BREATH    %d   MAGIC RES %d%%", m.breathType, m.magicResistance));
	lines.push_back("RESISTS   " + bits(m.elementResist));
	lines.push_back("FLAGS     " + (flags.empty() ? Common::String("-") : flags));
	lines.push_back(Common::String::format("REWARDS   %d / %d IN A ROOM", m.rewardWandering, m.rewardLair));
	if (m.partnerId > 0)
		lines.push_back(Common::String::format("PARTNER   %d (%d%%) ", m.partnerId, m.partnerChance) +
			messages.get(MonsterList::nameMessage(m.partnerId, true, false)));
	else
		lines.push_back("PARTNER   -");
	printLines(window, lines);
}

int DataPages::spellsPerPage(const TextWindow &window) {
	return (window.height() - 3) * 2;
}

void DataPages::drawSpells(TextWindow &window, const MessageFile &messages, int first) {
	window.printCentered(0, Common::String::format("SPELLS %d TO %d", first,
		MIN<int>(first + spellsPerPage(window) - 1, kSpellCount)));

	static const char TARGETS[] = "-PG";
	const int rows = window.height() - 3;
	const int columnWidth = window.width() / 2;

	for (int i = 0; i < spellsPerPage(window); ++i) {
		const int spell = first + i;
		if (spell > kSpellCount)
			break;

		// Number, name, M or P with the level, and whom it is for
		Common::String line = Common::String::format("%2d ", spell) + Spells::name(messages, spell);
		while ((int)line.size() < columnWidth - 5)
			line += ' ';
		line += Common::String::format("%c%d %c", Spells::isMage(spell) ? 'M' : 'P', Spells::level(spell),
			TARGETS[Spells::target(spell)]);
		window.PRINTSTR((i / rows) * columnWidth, 2 + i % rows,
			Common::String(line.c_str(), MIN<uint>(line.size(), columnWidth)));
	}
}

void DataPages::drawMaze(TextWindow &window, const MazeLevel &maze, int level, int levels, int x, int y) {
	window.printCentered(0, Common::String::format("MAZE LEVEL %d OF %d", level, levels));

	// The map, north at the top: a room with guardians shows '*'
	for (int my = 0; my < MazeLevel::kSize; ++my) {
		Common::String row;
		for (int mx = 0; mx < MazeLevel::kSize; ++mx) {
			const uint type = maze.squareTypeAt(mx, my);
			char c = (type < sizeof(SQUARE_CHARS) - 1) ? SQUARE_CHARS[type] : '?';
			if (c == '.' && maze.isFightSquare(mx, my))
				c = '*';
			if (mx == x && my == y)
				c = '@';
			row += c;
		}
		window.PRINTSTR(0, 1 + (MazeLevel::kSize - 1 - my), row);
	}

	// The square at the cursor
	const int col = MazeLevel::kSize + 1;
	const uint index = maze.squareIndex(x, y);
	const uint type = maze.squareType(index);
	Common::Array<Common::String> lines;
	lines.push_back(Common::String::format("%d EAST %d NORTH", x, y));
	lines.push_back(Common::String::format("%u ", index) + named(SQUARE_NAMES, ARRAYSIZE(SQUARE_NAMES), type));
	lines.push_back(Common::String::format("AUX %d %d %d", maze.aux(0, index), maze.aux(1, index), maze.aux(2, index)));
	static const char SIDES[] = "NESW";
	for (int side = 0; side < 4; ++side)
		lines.push_back(Common::String::format("%c ", SIDES[side]) +
			named(WALLS, ARRAYSIZE(WALLS), maze.wall(x, y, side)));
	lines.push_back(maze.isFightSquare(x, y) ? "ROOM" : "");

	// The kinds of square, two to a line
	for (uint t = 0; t + 1 < sizeof(SQUARE_CHARS); t += 2) {
		Common::String line = Common::String::format("%c %-6.6s", SQUARE_CHARS[t], SQUARE_NAMES[t]);
		if (t + 2 < sizeof(SQUARE_CHARS))
			line += Common::String::format(" %c %-6.6s", SQUARE_CHARS[t + 1], SQUARE_NAMES[t + 1]);
		lines.push_back(line);
	}
	lines.push_back("* ROOM   @ HERE");

	const int width = window.width() - col;
	for (uint i = 0; i < lines.size() && (int)i + 1 < window.height() - 1; ++i)
		window.PRINTSTR(col, 1 + i, Common::String(lines[i].c_str(), MIN<uint>(lines[i].size(), MAX(width, 0))));
}

} // End of namespace Wizardry
