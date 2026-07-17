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

#ifndef GOLDBOX_COMBAT_COMBAT_CONTEXT_H
#define GOLDBOX_COMBAT_COMBAT_CONTEXT_H

#include "common/scummsys.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}

namespace Combat {

struct CombatGlobals;
struct CombatParams;
class CombatantTable;
class BattlefieldMap;
class CombatViewport;

/**
 * Non-owning bundle of live combat object references.
 *
 * Passed to any combat subsystem (AI, movement, spells, etc.) that needs
 * to read or mutate global combat state during the combat phase.
 * CombatView owns all members; this struct just provides access.
 *
 * Mirrors the original pattern where COMBAT_* functions accessed scattered
 * globals (ARRAY_HOSTILITY, C_FIELD_PLACEMENT_MAP, BYTE_ARRAY_COL_DIST, etc.)
 * directly. Here those are reached through the owning objects.
 */
struct CombatContext {
    CombatGlobals  &globals;
    CombatParams   &params;
    CombatantTable &table;
    BattlefieldMap &map;
    CombatViewport &viewport;

    CombatContext(CombatGlobals &globals_,
                  CombatParams &params_,
                  CombatantTable &table_,
                  BattlefieldMap &map_,
                  CombatViewport &viewport_);

    /** Mirrors COMBAT_updateSideCount — call after any roster change. */
    void updateSideCount();

    /** Rebuild occupancy grid — call after any combatant moves or is removed. */
    void rebuildPlacementMap();

    /** Rebuild viewport distances — call after viewport scrolls or combatant moves. */
    void rebuildDistances();

    /**
     * Remove a character from the combat roster.
     *
     * Mirrors PARTY_removeMember — never touches engine _party.
     *   freeIconSlot=true   → release the character's icon resource
     *   keepPartyCount=true → remove from roster but do not shrink partyCount
     *                         (e.g. character fled, slot stays reserved)
     *   keepPartyCount=false → remove and decrement partyCount
     *                         (e.g. permanent removal from encounter)
     *
     * Always calls updateSideCount() after removal.
     */
    void removeMember(Data::PlayerCharacter *ch, bool keepPartyCount, bool freeIconSlot);
};

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_CONTEXT_H
