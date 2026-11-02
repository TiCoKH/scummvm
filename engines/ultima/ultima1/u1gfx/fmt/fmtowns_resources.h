/* ScummVM - Graphic Adventure Engine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef ULTIMA_ULTIMA1_GFX_FMT_FMTOWNS_RESOURCES_H
#define ULTIMA_ULTIMA1_GFX_FMT_FMTOWNS_RESOURCES_H

#include "common/config-manager.h"
#include "common/language.h"
#include "common/str.h"
#include "common/array.h"

namespace Ultima {
namespace Ultima1 {

/**
 * FM Towns (DARKNESS) localized string table.
 *
 * Strings are hardcoded for now (pre-datafile stage). Each member that has
 * both an English and Japanese variant is stored as an _EN / _JA pair.
 * Use the static str() helper to pick the correct pointer at runtime.
 *
 * Custom font glyphs (original 0xEB 0xNN two-byte sequences) will be stored
 * as Unicode PUA codepoints U+E000..U+E0FF once the devtool pipeline is wired.
 */
class FMTownsResources {
public:
	/**
	 * Returns the en or ja string based on the active ScummVM language setting.
	 */
	static const char *str(const char *en, const char *ja) {
		if (Common::parseLanguage(ConfMan.get("language")) == Common::JA_JPN && ja && *ja)
			return ja;
		return en;
	}

	// --- Intro ---
	const char *INTRO_HEADER_EN;
	const char *INTRO_HEADER_JA;

	// --- Intro slide text (UTF-8; use toSjis() before rendering) ---
	// Indexed as: [0]=slide1_1 .. [5]=slide1_6, [6]=slide2_1 .. [12]=slide2_7+
	Common::String INTRO_SLIDE_EN[13];
	Common::String INTRO_SLIDE_JA[13];

	// --- Combat / battle ---
	const char *ATTACKED_BY;
	const char *ATTACKS;
	const char *HIT;
	const char *MISSED;
	const char *DAMAGE;
	const char *KILLED;
	const char *ARMOR_DESTROYED;
	const char *MENTAL_ATTACK;
	const char *MISS;
	const char *HIT_STATS_REDUCED;

	// --- Movement / navigation ---
	const char *NORTH;
	const char *SOUTH;
	const char *EAST;
	const char *WEST;
	const char *BLOCKED;
	const char *FORWARD;
	const char *LEFT_TURN;
	const char *RIGHT_TURN;
	const char *BACK_TURN;
	const char *PATH_BLOCKED;
	const char *WALL_BLOCKS;

	// --- World / location ---
	const char *ENTERING;
	const char *THE_CITY_OF;
	const char *YOU_ARE_AT_SEA;
	const char *YOU_ARE_IN_WOODS;
	const char *YOU_ARE_IN_LANDS;
	const char *HEAD;

	// --- Dungeon ---
	const char *DUNGEON_LEVEL;
	const char *GO_UPSTAIRS;
	const char *GO_DOWNSTAIRS;
	const char *UP_TO_LEVEL;
	const char *DOWN_TO_LEVEL;
	const char *YOU_EXITED_DUNGEON;
	const char *LADDER_CREATED;
	const char *FORCE_FIELD;

	// --- Items / inventory ---
	const char *INVENTORY;
	const char *GET;
	const char *DROP;
	const char *FOUND_KEY;
	const char *NO_KEY;
	const char *INCORRECT_KEY;
	const char *DOOR_IS_OPEN;
	const char *THOU_DOST_FIND;
	const char *THOU_DOST_GAIN;
	const char *OPEN;
	const char *OPENED;
	const char *CHEST;
	const char *COFFIN;
	const char *SET_OFF_TRAP;
	const char *CANT_LEAVE_IT_HERE;

	// --- Weapons / armour ---
	const char *WEAPON;
	const char *ARMOUR;
	const char *READY;
	const char *WEAP_LABEL;
	const char *ARMR_LABEL;

	// --- Spells ---
	const char *CAST;
	const char *SHAZAM;
	const char *ALAKAZOT;
	const char *NO_EFFECT;
	const char *FAILED;
	const char *TELEPORTED;
	const char *FIELD_CREATED;
	const char *FIELD_DESTROYED;
	const char *MONSTER_REMOVED;
	const char *USED_UP_SPELL;
	const char *DUNGEON_SPELL_ONLY;
	const char *SPELL_LABEL;

	// --- Character stats ---
	const char *NAME_LABEL;
	const char *A_LEVEL;
	const char *SEX_LABEL;
	const char *RACE_LABEL;
	const char *TYPE_LABEL;
	const char *STRENGTH;
	const char *AGILITY;
	const char *STAMINA;
	const char *CHARISMA;
	const char *WISDOM;
	const char *INTELLIGENCE;
	const char *HIT_POINTS;
	const char *EXPERIENCE;
	const char *FOOD_LABEL;
	const char *GOLD_LABEL;
	const char *HP_EX_LABEL;
	const char *FD_CN_LABEL;

	// --- Currency ---
	const char *COPPER_PENCE;
	const char *SILVER_PIECES;
	const char *GOLD_CROWNS;
	const char *ENEMY_VESSELS;

	// --- Race / sex / class names ---
	const char *RACE_HUMAN;
	const char *RACE_ELF;
	const char *RACE_DWARF;
	const char *RACE_BOBBIT;
	const char *SEX_MALE;
	const char *SEX_FEMALE;
	const char *CLASS_FIGHTER;
	const char *CLASS_CLERIC;
	const char *CLASS_WIZARD;
	const char *CLASS_THIEF;

	// --- Trade / shops ---
	const char *BUY_SELL;
	const char *BUY;
	const char *SELL;
	const char *SOLD;
	const char *NOTHING;
	const char *NONE_HERE;
	const char *NOTHING_HERE;
	const char *CANT_AFFORD;
	const char *NOT_BY_COUNTER;
	const char *TRANSACT;
	const char *PENCE;
	const char *COPPER;
	const char *SILVER;
	const char *GOLD;
	const char *FOOD_PACKS;
	const char *FOOD_COST;
	const char *HOW_MANY;
	const char *HOW_MUCH;
	const char *SORRY_NO_SPELLS;
	const char *NO_WEAPONRY_SELL;
	const char *NO_ARMOUR_SELL;
	const char *COME_BACK_WHEN;
	const char *MONEY_TO_SPEND;

	// --- King / quest ---
	const char *WITH_KING;
	const char *HE_IS_NOT_HERE;
	const char *HE_REJECTS_OFFER;
	const char *NO_KINGS_PERMISSION;
	const char *QUEST_COMPLETED;
	const char *FOR_THIS_GIVE_THEE;
	const char *HIT_POINTS_REWARD;
	const char *POINT;

	// --- NPC / social ---
	const char *NONE_WILL_TALK;
	const char *NOISE;
	const char *YELL;
	const char *WORD;
	const char *BARD_SINGS;
	const char *BARD_SONG;
	const char *JESTER_SINGS;
	const char *JESTER_SONG;
	const char *BARD_STOLE;
	const char *JESTER_STOLE;
	const char *THIEF_STOLE;
	const char *GREMLIN_STOLE;

	// --- Transport / vehicle ---
	const char *BOARD;
	const char *MOUNT;
	const char *EXIT_CRAFT;
	const char *NOTHING_TO_BOARD;
	const char *CANNOT_OPERATE;

	// --- Space combat ---
	const char *FIRE;
	const char *SPEED;
	const char *SECTOR_SCAN;
	const char *HYPERJUMP;
	const char *LIGHTSPEED;
	const char *COMPLETED;
	const char *CENTER;
	const char *PASS;
	const char *NO_FUEL;
	const char *NOT_ENOUGH_FUEL;
	const char *DOCKED_WELCOME;
	const char *TO_BASE;
	const char *SHIELD_LABEL;
	const char *FUEL_LABEL;
	const char *EXP_LABEL;
	const char *COIN_LABEL;
	const char *ENEMY_LABEL;
	const char *SPACE_ACE;
	const char *RANK_OF;
	const char *ACHIEVED_RANK;

	// --- Roster / character generation ---
	const char *REGISTER_LABEL;
	const char *EMPTY;
	const char *GENERATE_CHARACTER;
	const char *CREATE;
	const char *CREATED;
	const char *TERMINATE;
	const char *TERMINATED;
	const char *LOOK_AT_CHARACTER;
	const char *SELECT_PLAYER;
	const char *SELECT_WITH_ARROWS;
	const char *SELECTED;
	const char *NO_ONE_THERE;
	const char *NOT_EMPTY;
	const char *POINTS_LABEL;
	const char *CANCELED;
	const char *OPTION;
	const char *MORE;
	const char *ZTATS;

	// --- UI / system ---
	const char *HUH;
	const char *WHAT;
	const char *OK;
	const char *YES;
	const char *NO;
	const char *DONE;
	const char *PRESS_SPACE;
	const char *PRESS_SPACE_CONTINUE;
	const char *PUSH_Y_N;
	const char *SAVE_GAME;
	const char *NOW_SAVING;
	const char *SAVED;
	const char *ABORTED;
	const char *WAIT_SAVING_ROSTER;
	const char *MODE_ENGLISH;
	const char *BGM_ON;
	const char *BGM_OFF;
	const char *SE_ON;
	const char *SE_OFF;
	const char *VOLUME;
	const char *MUSIC_TEST;
	const char *SE_TEST;

	// --- Error / disk ---
	const char *SHORT_OF_MEMORY;
	const char *CD_NOT_READY;
	const char *CD_DISCONNECTED;
	const char *CD_HARDWARE_ERROR;
	const char *CD_SYSTEM_ERROR;
	const char *MEDIA_ERROR;
	const char *WRITE_PROTECTED;
	const char *WARNING_PROTECTED;
	const char *DRIVE_NOT_READY;
	const char *SOUND_INIT_ERROR;
	const char *PCM_LOAD_ERROR;
	const char *DATA_SAVE_ERROR;

	// --- Mondain ending ---
	const char *MONDAIN;
	const char *MONDAIN_IS_DEAD;
	const char *MONDAIN_CASTS;
	const char *MAGIC_MISSILE;
	const char *MIND_BLASTER;
	const char *PSIONIC_SHOCK;
	const char *THOU_ART_VICTORIOUS;
	const char *THOU_ART_DEAD;
	const char *ATTEMPTING_RESURRECTION;

	// --- Princess ---
	const char *PRINCESS;
	const char *THOU_HAST_SAVED;
	const char *THE_PRINCESS;
	const char *SHE_GIVETH_THEE;
	const char *HIT_POINTS_4II;

	FMTownsResources();

private:
	// Backing storage for INTRO_HEADER_EN/JA pointers
	Common::String _introHeaderEn;
	Common::String _introHeaderJa;
	void loadFromYml();
};

} // End of namespace Ultima1
} // End of namespace Ultima

#endif
