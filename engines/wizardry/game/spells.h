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

#ifndef WIZARDRY_GAME_SPELLS_H
#define WIZARDRY_GAME_SPELLS_H

#include "common/array.h"
#include "wizardry/game/character.h"
#include "wizardry/resources/messages.h"

namespace Wizardry {

/**
 * The spells of Wizardry I, by the numbers the game gives them:
 * 1 to 21 are the mage's, 22 to 50 the priest's.
 */
enum Spell {
	kSpellNone,
	HALITO, MOGREF, KATINO, DUMAPIC, DILTO, SOPIC, MAHALITO, MOLITO,
	MORLIS, DALTO, LAHALITO, MAMORLIS, MAKANITO, MADALTO, LAKANITO, ZILWAN,
	MASOPIC, HAMAN, MALOR, MAHAMAN, TILTOWAIT,
	KALKI, DIOS, BADIOS, MILWA, PORFIC, MATU, CALFO, MANIFO, MONTINO,
	LOMILWA, DIALKO, LATUMAPIC, BAMATU, DIAL, BADIAL, LATUMOFIS, MAPORFIC,
	DIALMA, BADIALMA, LITOKAN, KANDI, DI, BADI, LORTO, MADI, MABADI,
	LOKTOFEIT, MALIKTO, KADORTO,
	kSpellCount = KADORTO
};

class Spells {
public:
	enum Target {
		kTargetNone,
		kTargetMember,   ///< someone of the party
		kTargetGroup     ///< a group of monsters
	};

	static bool isMage(int spell) { return spell >= HALITO && spell <= TILTOWAIT; }

	/** Spell level, 1 to 7 */
	static int level(int spell);
	static Target target(int spell);

	/** The name, without the mark that starts a spell level in the list */
	static Common::String name(const MessageFile &messages, int spell);

	/** True if the character knows the spell and can still cast one of its level */
	static bool canCast(const Character &c, int spell);

	/** Uses up one casting of the spell's level */
	static void spend(Character &c, int spell);

	/**
	 * Finds the spells a typed name can mean: the one with exactly that
	 * name, or else all those the character knows that begin with it
	 */
	static Common::Array<int> find(const MessageFile &messages, const Character &c, const Common::String &typed);
};

} // End of namespace Wizardry

#endif
