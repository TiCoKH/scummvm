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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_ITEM_USE_RESULT_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_ITEM_USE_RESULT_H

#include "common/scummsys.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * GB13-013 — typed item-dialog outcome (Requirement 24).
 *
 * The original DIALOG_Items returned a single bool (the b_ptr / resultFlag the
 * combat caller looped on). That conflates "dialog closed" with "turn used",
 * so the port maps the original per-command semantics into an explicit result
 * that flows ItemsMenu -> CharacterProfile/View -> CombatView. A Use that cast
 * a wand/scroll/potion consumes the action (ACTION_UseSpell -> COMBAT_endActorTurn);
 * View/Identify/Ready with no use returns without acting (DIALOG_CombatMain
 * loops on a false result and reopens the menu); Escape cancels; an application
 * quit tears down without advancing.
 */
enum ItemActionResult {
	IAR_CANCELLED = 0,      // Escape/exit with no state change (menu dismissed)
	IAR_RETURNED,           // Returned without an action: turn continues, menu reopens
	IAR_ACTION_CONSUMED,    // A use cast a spell/charge: the actor's turn ended
	IAR_QUIT                // Application quit: tear down, do not advance the turn
};

/** Where the item dialog was entered from (controls charging + targeting). */
enum ItemUseOrigin {
	IUO_EXPLORATION = 0,    // Camp / wilderness / dungeon (no combat turn to consume)
	IUO_COMBAT              // Opened from the combat Use action (DIALOG_CombatMain)
};

/**
 * True only when the combat scheduler should advance the actor for @p result.
 * Exactly IAR_ACTION_CONSUMED advances; everything else (cancel, returned,
 * quit) leaves the activation where it is. This is the single decision that
 * stops "dialog closed == turn end": direct Use and View->Items both route
 * through here, so they cannot double-advance.
 */
inline bool itemResultConsumesTurn(ItemActionResult result) {
	return result == IAR_ACTION_CONSUMED;
}

/**
 * Map a completed Use attempt to the typed result.
 *
 * @p castSucceeded is true only at the source-defined success point (a
 * confirmed ACTION_UseSpell cast: CAST_OK). @p turnEnded is true when that
 * success also consumed the combat action (a timed cast called endTurn()).
 * A cancelled target selection reaches this with castSucceeded == false and so
 * returns IAR_RETURNED, preserving the charge/payload (the caller must not have
 * committed it).
 */
inline ItemActionResult itemUseResult(bool castSucceeded, bool turnEnded) {
	if (!castSucceeded)
		return IAR_RETURNED;
	return turnEnded ? IAR_ACTION_CONSUMED : IAR_RETURNED;
}

/**
 * Independent restriction gates (Requirement 24.3). The two flags are read from
 * authoritative PoR state and never collapsed into one:
 *   - @p noMagic  (G_NoMagicFlag / kVmFieldNoMagicFlag) forbids spell-origin
 *     item use (wands/scrolls/potions cast a spell).
 *   - @p noCombatItems (zone1[NO_COMBAT_ITEMS] / kVmGlobalFieldNoItemCombatFlag)
 *     forbids item use entirely while in combat.
 * A spell-origin use needs BOTH to be clear; a non-spell item use only needs
 * the no-items gate to be clear.
 */
inline bool spellOriginUseAllowed(bool noMagic, bool noCombatItems) {
	return !noMagic && !noCombatItems;
}

inline bool itemUseAllowed(bool noCombatItems) {
	return !noCombatItems;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_ITEM_USE_RESULT_H
