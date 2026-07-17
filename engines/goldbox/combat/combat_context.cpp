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

#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/combat/combat_params.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/combat_viewport.h"
#include "goldbox/data/player_character.h"

namespace Goldbox {
namespace Combat {

CombatContext::CombatContext(CombatGlobals &globals_,
                             CombatParams &params_,
                             CombatantTable &table_,
                             BattlefieldMap &map_,
                             CombatViewport &viewport_)
    : globals(globals_), params(params_), table(table_),
      map(map_), viewport(viewport_) {}

void CombatContext::updateSideCount() {
    globals.updateSideCount(params.roster);
}

void CombatContext::rebuildPlacementMap() {
    table.rebuildOccupancy();
}

void CombatContext::rebuildDistances() {
    table.setViewportOrigin(viewport.getTopLeftCol(), viewport.getTopLeftRow());
}

void CombatContext::removeMember(Data::PlayerCharacter *ch, bool keepPartyCount, bool freeIconSlot) {
    if (!ch)
        return;

    if (freeIconSlot)
        ch->iconData.iconSlotId = 0;

    // Remove from roster
    for (uint i = 0; i < params.roster.size(); i++) {
        if (params.roster[i] == ch) {
            params.roster.remove_at(i);
            break;
        }
    }

    if (!keepPartyCount && params.partyCount > 0)
        params.partyCount--;

    updateSideCount();
}

} // namespace Combat
} // namespace Goldbox
