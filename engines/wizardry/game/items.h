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

#ifndef WIZARDRY_GAME_ITEMS_H
#define WIZARDRY_GAME_ITEMS_H

#include "common/array.h"
#include "common/serializer.h"
#include "wizardry/game/character.h"
#include "wizardry/resources/scenario_file.h"

namespace Wizardry {

/**
 * An item, as kept in record type 4 of the scenario file of IBM-PC
 * Wizardry I (46 bytes). Only what the trading post needs is taken out.
 */
struct Item {
	enum {
		kRecordSize = 46,
		kUnlimited = -1   ///< stock that never runs out
	};

	int16 kind = 0;       ///< 0 weapon, 1 armour, 2 shield, 3 helmet, 4 gauntlets, 5 special, 6 other
	int16 alignment = 0;  ///< 0 for anyone; otherwise it turns on a wearer of another alignment
	bool cursed = false;
	int16 specialPower = 0;   ///< 1 to 27: what invoking it does; see Equip::invokePower()
	int16 changeTo = 0;       ///< the item it becomes when it breaks
	int16 changeChance = 0;   ///< how likely that is, in percent
	int16 healPoints = 0;
	uint16 protectVs = 0;
	uint16 resist = 0;
	int16 armorMod = 0;       ///< taken off the armour class
	int16 hitMod = 0;
	Dice3 damage = { 0, 0, 0 };
	int16 swings = 0;
	bool criticalHits = false;
	uint16 doubleDamageVs = 0;
	int64 price = 0;
	int16 stock = 0;      ///< pieces at the trading post; below -1: not on sale yet
	uint16 classUse = 0;  ///< a bit for each class that can use it

	bool usableBy(const Character &c) const {
		return (classUse >> c.classType) & 1;
	}
};

/**
 * The items of the scenario. The stock of the trading post changes as
 * things are bought and sold, so it belongs to the state of the game.
 */
class ItemList {
private:
	Common::Array<Item> _items;

public:
	/**
	 * @returns false if the item records are not the kind this class knows
	 */
	bool load(const ScenarioFile &scenario);

	/**
	 * Saves or loads what changes during a game: the stock of the
	 * trading post
	 * @returns false if what was loaded is for another list of items
	 */
	bool synchronize(Common::Serializer &s);

	uint size() const { return _items.size(); }
	Item &operator[](uint id) { return _items[id]; }
	const Item &operator[](uint id) const { return _items[id]; }

	/**
	 * Message number of an item's name: the true one, or the one shown
	 * while the item is not identified
	 */
	static uint nameMessage(uint id, bool identified) {
		return 14000 + id * 2 + (identified ? 1 : 0);
	}
};

} // End of namespace Wizardry

#endif
