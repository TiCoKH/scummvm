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

#ifndef WIZARDRY_GAME_EQUIP_H
#define WIZARDRY_GAME_EQUIP_H

#include "wizardry/game/items.h"
#include "wizardry/game/party.h"
#include "wizardry/game/random.h"

namespace Wizardry {

/**
 * A character's battle figures and what equipment does to them, as in
 * Wizardry I. Nothing here is kept from one time to the next: saving
 * throws, hit bonus, damage, armour class and number of attacks are worked
 * out afresh from level, class, race and the items equipped.
 */
class Equip {
public:
	enum {
		kKindCount = 6,
		kPowerCount = 27
	};

	/** The kinds of equipment, in the order they are asked for */
	static const int16 KINDS[kKindCount];

	/**
	 * The figures of a character with nothing equipped, and what comes
	 * from items merely carried (healing, protection, resistance).
	 * @param unequip  Whether to take everything off, as before equipping
	 */
	static void begin(Character &c, const ItemList &items, bool unequip);

	/**
	 * Adds what an equipped item gives. An item made for another
	 * alignment turns on its owner and counts as cursed.
	 */
	static void applyItem(Character &c, uint slot, const ItemList &items);

	/**
	 * Ends the working out
	 * @param anyApplied  Whether applyItem() was called at all: a ninja
	 *                    with nothing on is harder to hit
	 */
	static void end(Character &c, bool anyApplied);

	/**
	 * Works out the figures from what is equipped now
	 */
	static void recalc(Character &c, const ItemList &items);

	/**
	 * The places in the pack with items of a kind that the class can use
	 */
	static Common::Array<uint> candidates(const Character &c, int kind, const ItemList &items);

	/**
	 * True if the item in a place of the pack has a power the character
	 * can invoke
	 */
	static bool hasPower(const Character &c, uint slot, const ItemList &items);

	/**
	 * Invokes the special power of an item: it may break into another
	 * item first, then the power takes effect.
	 * @param member  Place of the character in the party
	 */
	static void invokePower(Party &party, uint member, uint slot, const ItemList &items, Dice &dice);
};

} // End of namespace Wizardry

#endif
