/* ScummVM - Graphic Adventure Engine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "ultima/ultima1/u1gfx/fmt/fmtowns_resources.h"
#include "common/file.h"
#include "common/hashmap.h"
#include "common/str.h"

namespace Ultima {
namespace Ultima1 {

static const char *const TODO = "TODO";

// ---------------------------------------------------------------------------
// Minimal line-by-line parser for strings_fmt.yml.
// Handles the three-level structure:
//   section:        (indent 0)
//     key:          (indent 2)
//       en: "..."   (indent 4)
//       ja: "..."   (indent 4)
// ---------------------------------------------------------------------------
void FMTownsResources::loadFromYml() {
	Common::File f;
	if (!f.open(Common::Path("strings_fmt.yml")))
		return;

	struct Entry { Common::String en, ja; };
	Common::HashMap<Common::String, Entry> table;

	Common::String section, key;
	Entry cur;
	bool inEntry = false;

	while (!f.eos()) {
		Common::String line;
		char c;
		while (!f.eos()) {
			c = (char)f.readByte();
			if (c == '\n') break;
			if (c != '\r') line += c;
		}

		// Strip inline comment
		int commentPos = line.findFirstOf('#');
		if (commentPos != -1)
			line = Common::String(line.c_str(), commentPos);

		if (line.empty()) continue;

		uint indent = 0;
		while (indent < line.size() && line[indent] == ' ') indent++;
		Common::String trimmed(line.c_str() + indent);

		if (indent == 0) {
			if (inEntry) { table[section + "." + key] = cur; inEntry = false; }
			if (trimmed.hasSuffix(":"))
				section = Common::String(trimmed.c_str(), trimmed.size() - 1);
		} else if (indent == 2) {
			if (inEntry) table[section + "." + key] = cur;
			if (trimmed.hasSuffix(":")) {
				key = Common::String(trimmed.c_str(), trimmed.size() - 1);
				cur = Entry();
				inEntry = true;
			}
		} else if (indent == 4 && inEntry) {
			auto extractVal = [](const Common::String &s) -> Common::String {
				int q1 = s.findFirstOf('"');
				if (q1 == -1) return "";
				int q2 = s.findLastOf('"');
				if (q2 <= q1) return "";
				return Common::String(s.c_str() + q1 + 1, q2 - q1 - 1);
			};
			if (trimmed.hasPrefix("en:"))
				cur.en = extractVal(trimmed);
			else if (trimmed.hasPrefix("ja:"))
				cur.ja = extractVal(trimmed);
		}
	}
	if (inEntry)
		table[section + "." + key] = cur;
	f.close();

	// Intro header: prefer intro.header, fall back to common.header
	for (const char *hk : {"common.header", "intro.header"}) {
		if (table.contains(hk)) {
			_introHeaderEn = table[hk].en;
			_introHeaderJa = table[hk].ja;
			INTRO_HEADER_EN = _introHeaderEn.c_str();
			INTRO_HEADER_JA = _introHeaderJa.c_str();
		}
	}

	// Intro slide text (13 entries matching TITLE_MESSAGES indices)
	static const char *const slideKeys[13] = {
		"intro.slide1_1", "intro.slide1_2", "intro.slide1_3",
		"intro.slide1_4", "intro.slide1_5", "intro.slide1_6",
		"intro.slide2_1", "intro.slide2_2", "intro.slide2_3",
		"intro.slide2_4", "intro.slide2_5", "intro.slide2_6",
		"intro.slide2_7"
	};
	for (int i = 0; i < 13; ++i) {
		if (table.contains(slideKeys[i])) {
			INTRO_SLIDE_EN[i] = table[slideKeys[i]].en;
			INTRO_SLIDE_JA[i] = table[slideKeys[i]].ja;
		}
	}
}

FMTownsResources::FMTownsResources() {
	INTRO_HEADER_EN = "";
	INTRO_HEADER_JA = "";
	loadFromYml();

	ATTACKED_BY = TODO; ATTACKS = TODO; HIT = TODO; MISSED = TODO;
	DAMAGE = TODO; KILLED = TODO; ARMOR_DESTROYED = TODO;
	MENTAL_ATTACK = TODO; MISS = TODO; HIT_STATS_REDUCED = TODO;
	NORTH = TODO; SOUTH = TODO; EAST = TODO; WEST = TODO;
	BLOCKED = TODO; FORWARD = TODO; LEFT_TURN = TODO;
	RIGHT_TURN = TODO; BACK_TURN = TODO; PATH_BLOCKED = TODO;
	WALL_BLOCKS = TODO; ENTERING = TODO; THE_CITY_OF = TODO;
	YOU_ARE_AT_SEA = TODO; YOU_ARE_IN_WOODS = TODO;
	YOU_ARE_IN_LANDS = TODO; HEAD = TODO;
	DUNGEON_LEVEL = TODO; GO_UPSTAIRS = TODO; GO_DOWNSTAIRS = TODO;
	UP_TO_LEVEL = TODO; DOWN_TO_LEVEL = TODO;
	YOU_EXITED_DUNGEON = TODO; LADDER_CREATED = TODO; FORCE_FIELD = TODO;
	INVENTORY = TODO; GET = TODO; DROP = TODO; FOUND_KEY = TODO;
	NO_KEY = TODO; INCORRECT_KEY = TODO; DOOR_IS_OPEN = TODO;
	THOU_DOST_FIND = TODO; THOU_DOST_GAIN = TODO; OPEN = TODO;
	OPENED = TODO; CHEST = TODO; COFFIN = TODO;
	SET_OFF_TRAP = TODO; CANT_LEAVE_IT_HERE = TODO;
	WEAPON = TODO; ARMOUR = TODO; READY = TODO;
	WEAP_LABEL = TODO; ARMR_LABEL = TODO;
	CAST = TODO; SHAZAM = TODO; ALAKAZOT = TODO; NO_EFFECT = TODO;
	FAILED = TODO; TELEPORTED = TODO; FIELD_CREATED = TODO;
	FIELD_DESTROYED = TODO; MONSTER_REMOVED = TODO;
	USED_UP_SPELL = TODO; DUNGEON_SPELL_ONLY = TODO; SPELL_LABEL = TODO;
	NAME_LABEL = TODO; A_LEVEL = TODO; SEX_LABEL = TODO;
	RACE_LABEL = TODO; TYPE_LABEL = TODO; STRENGTH = TODO;
	AGILITY = TODO; STAMINA = TODO; CHARISMA = TODO;
	WISDOM = TODO; INTELLIGENCE = TODO; HIT_POINTS = TODO;
	EXPERIENCE = TODO; FOOD_LABEL = TODO; GOLD_LABEL = TODO;
	HP_EX_LABEL = TODO; FD_CN_LABEL = TODO;
	COPPER_PENCE = TODO; SILVER_PIECES = TODO;
	GOLD_CROWNS = TODO; ENEMY_VESSELS = TODO;
	RACE_HUMAN = TODO; RACE_ELF = TODO; RACE_DWARF = TODO;
	RACE_BOBBIT = TODO; SEX_MALE = TODO; SEX_FEMALE = TODO;
	CLASS_FIGHTER = TODO; CLASS_CLERIC = TODO;
	CLASS_WIZARD = TODO; CLASS_THIEF = TODO;
	BUY_SELL = TODO; BUY = TODO; SELL = TODO; SOLD = TODO;
	NOTHING = TODO; NONE_HERE = TODO; NOTHING_HERE = TODO;
	CANT_AFFORD = TODO; NOT_BY_COUNTER = TODO; TRANSACT = TODO;
	PENCE = TODO; COPPER = TODO; SILVER = TODO; GOLD = TODO;
	FOOD_PACKS = TODO; FOOD_COST = TODO; HOW_MANY = TODO;
	HOW_MUCH = TODO; SORRY_NO_SPELLS = TODO;
	NO_WEAPONRY_SELL = TODO; NO_ARMOUR_SELL = TODO;
	COME_BACK_WHEN = TODO; MONEY_TO_SPEND = TODO;
	WITH_KING = TODO; HE_IS_NOT_HERE = TODO;
	HE_REJECTS_OFFER = TODO; NO_KINGS_PERMISSION = TODO;
	QUEST_COMPLETED = TODO; FOR_THIS_GIVE_THEE = TODO;
	HIT_POINTS_REWARD = TODO; POINT = TODO;
	NONE_WILL_TALK = TODO; NOISE = TODO; YELL = TODO; WORD = TODO;
	BARD_SINGS = TODO; BARD_SONG = TODO;
	JESTER_SINGS = TODO; JESTER_SONG = TODO;
	BARD_STOLE = TODO; JESTER_STOLE = TODO;
	THIEF_STOLE = TODO; GREMLIN_STOLE = TODO;
	BOARD = TODO; MOUNT = TODO; EXIT_CRAFT = TODO;
	NOTHING_TO_BOARD = TODO; CANNOT_OPERATE = TODO;
	FIRE = TODO; SPEED = TODO; SECTOR_SCAN = TODO;
	HYPERJUMP = TODO; LIGHTSPEED = TODO; COMPLETED = TODO;
	CENTER = TODO; PASS = TODO; NO_FUEL = TODO;
	NOT_ENOUGH_FUEL = TODO; DOCKED_WELCOME = TODO; TO_BASE = TODO;
	SHIELD_LABEL = TODO; FUEL_LABEL = TODO; EXP_LABEL = TODO;
	COIN_LABEL = TODO; ENEMY_LABEL = TODO; SPACE_ACE = TODO;
	RANK_OF = TODO; ACHIEVED_RANK = TODO;
	REGISTER_LABEL = TODO; EMPTY = TODO;
	GENERATE_CHARACTER = TODO; CREATE = TODO; CREATED = TODO;
	TERMINATE = TODO; TERMINATED = TODO;
	LOOK_AT_CHARACTER = TODO; SELECT_PLAYER = TODO;
	SELECT_WITH_ARROWS = TODO; SELECTED = TODO;
	NO_ONE_THERE = TODO; NOT_EMPTY = TODO; POINTS_LABEL = TODO;
	CANCELED = TODO; OPTION = TODO; MORE = TODO; ZTATS = TODO;
	HUH = TODO; WHAT = TODO; OK = TODO; YES = TODO; NO = TODO;
	DONE = TODO; PRESS_SPACE = TODO; PRESS_SPACE_CONTINUE = TODO;
	PUSH_Y_N = TODO; SAVE_GAME = TODO; NOW_SAVING = TODO;
	SAVED = TODO; ABORTED = TODO; WAIT_SAVING_ROSTER = TODO;
	MODE_ENGLISH = TODO; BGM_ON = TODO; BGM_OFF = TODO;
	SE_ON = TODO; SE_OFF = TODO; VOLUME = TODO;
	MUSIC_TEST = TODO; SE_TEST = TODO;
	SHORT_OF_MEMORY = TODO; CD_NOT_READY = TODO;
	CD_DISCONNECTED = TODO; CD_HARDWARE_ERROR = TODO;
	CD_SYSTEM_ERROR = TODO; MEDIA_ERROR = TODO;
	WRITE_PROTECTED = TODO; WARNING_PROTECTED = TODO;
	DRIVE_NOT_READY = TODO; SOUND_INIT_ERROR = TODO;
	PCM_LOAD_ERROR = TODO; DATA_SAVE_ERROR = TODO;
	MONDAIN = TODO; MONDAIN_IS_DEAD = TODO; MONDAIN_CASTS = TODO;
	MAGIC_MISSILE = TODO; MIND_BLASTER = TODO; PSIONIC_SHOCK = TODO;
	THOU_ART_VICTORIOUS = TODO; THOU_ART_DEAD = TODO;
	ATTEMPTING_RESURRECTION = TODO;
	PRINCESS = TODO; THOU_HAST_SAVED = TODO; THE_PRINCESS = TODO;
	SHE_GIVETH_THEE = TODO; HIT_POINTS_4II = TODO;
}

} // End of namespace Ultima1
} // End of namespace Ultima
