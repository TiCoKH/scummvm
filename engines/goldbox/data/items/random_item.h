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

#ifndef GOLDBOX_DATA_ITEMS_RANDOM_ITEM_H
#define GOLDBOX_DATA_ITEMS_RANDOM_ITEM_H

#include "common/scummsys.h"
#include "goldbox/data/items/base_items.h"
#include "goldbox/data/items/character_item.h"

// GB13-032: pure, testable port of the random treasure item generator
// (GenerateRandomItem / ITEM_createRandomMagicItem + RandomBonus,
// poolrad_engine_host_impl.cpp:1970). None of these functions touch g_engine
// or the global RNG directly: the category roll and the bonus/charge rolls are
// driven through an injected IItemRollSource so the whole decision tree and the
// bonus boundaries are unit-provable without a live engine. The live host wires
// the source to VmInterface::rollDice.
//
// The generator returns an OWNED CANONICAL CharacterItem (every field set), NOT
// a stub record: category roll -> typeIndex, then base-item properties from the
// Storage (gItemProps) template, then RandomBonus (plus / charges), then the
// identification / curse / payload fields. Generated treasure is a fresh owned
// instance kept distinct from immutable shop stock templates.

namespace Goldbox {
namespace Data {
namespace Items {

/**
 * Injectable dice source for the generator, mirroring the encounter_rules
 * ISurpriseRollSource pattern. rollDice(number, sides) returns the sum of
 * @p number d@p sides in 1..number*sides. The live host forwards to
 * VmInterface::rollDice; tests inject a scripted sequence so each reachable
 * category and each bonus boundary is deterministic.
 */
class IItemRollSource {
public:
	virtual ~IItemRollSource() {}

	/** Roll @p number dice of @p sides each and return the sum. */
	virtual int rollDice(int number, int sides) = 0;
};

/**
 * Port of the random magic-item CATEGORY roll (rollRandomMagicItemType). This
 * is the first half of GenerateRandomItem: a 1d100 primary roll feeds nested
 * sub-rolls that select a base item typeIndex. The roll ORDER and the exact
 * threshold tables are preserved verbatim from the DOS reference; only the RNG
 * is abstracted behind @p source.
 *
 * @return a base-item typeIndex (index into Storage::all() / gItemProps).
 */
uint8 rollRandomMagicItemType(IItemRollSource &source);

/**
 * Port of the random enchantment roll (RandomBonus). Fills the plus
 * (bonus/saveBonus) and, for charged items, the charge count. See random_item.cpp
 * for the exact boundary table.
 *
 * NOTE (OPEN): the precise DOS plus-distribution thresholds and the per-item
 * charge counts are reference data that could not be byte-confirmed from the
 * sources available in-repo. The implemented boundaries follow the standard
 * Gold Box 1d100 plus distribution and are documented inline; they are applied
 * deterministically so tests pin the behaviour. Treat the specific numeric
 * boundaries as REVIEW-OPEN.
 */
struct BonusRoll {
	uint8 bonus;     ///< enchantment plus (0 for mundane / charge items)
	uint8 saveBonus; ///< saving-throw plus (mirrors bonus for wearables)
	uint8 charges;   ///< charges for wands/rods/staves/scrolls (0 otherwise)

	BonusRoll() : bonus(0), saveBonus(0), charges(0) {}
};

BonusRoll rollRandomBonus(IItemRollSource &source);

/**
 * Generate a fully-populated owned canonical magic item.
 *
 * @param typeIndex  base-item typeIndex (from rollRandomMagicItemType, or a
 *                   caller-supplied value in tests to target a category).
 * @param source     injected dice source for the bonus/charge/payload rolls.
 * @param props      the base-item property storage (Engine::gItemProps). Used
 *                   to read the base template (slot/hands/weapon flags) so the
 *                   generated instance is equippable/usable; never mutated.
 *
 * Every CharacterItem field is initialised. Magic properties start HIDDEN
 * (unidentified) via the `hidden` mask, matching a freshly-dropped magic item;
 * Detect/Identify later reveals them without this generator rerolling.
 */
CharacterItem generateRandomItem(uint8 typeIndex, IItemRollSource &source,
		const Storage &props);

/**
 * Convenience: roll a category and generate the item in one call.
 */
CharacterItem generateRandomItem(IItemRollSource &source, const Storage &props);

} // namespace Items
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_ITEMS_RANDOM_ITEM_H
