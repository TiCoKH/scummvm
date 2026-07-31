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

#include "goldbox/combat/combat_setup.h"
#include "goldbox/combat/battlefield_map.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/combat/combat_params.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/combat_placement.h"
#include "goldbox/combat/combat_viewport.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/combat_state.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/gfx/battlefield_tilemap.h"
#include "goldbox/runtime/runtime_geo.h"
#include "goldbox/core/direction.h"
#include "goldbox/core/vm_layout.h"
#include "goldbox/ecl/ecl_memory.h"

namespace Goldbox {
namespace Combat {

void initCombatStates(Common::Array<Data::PlayerCharacter *> &combatants,
                         int partyCount, uint8 wayFlag,
                         uint8 moraleThreshold) {
    int combatantCount = 0;

    for (uint i = 0; i < combatants.size(); i++) {
        Data::PlayerCharacter *ch = combatants[i];
        if (!ch)
            continue;

        combatantCount++;

        // Allocate and zero-fill combat state
        delete ch->combatState;
        ch->combatState = new Data::CombatAction();

        // Mark non-hostile characters beyond the player party as not in team.
        // Hostile monsters are always in the enemy team regardless of roster index.
        if (partyCount < combatantCount && !ch->hostile)
            ch->combatState->notInTeam = true;

        // Original morale migration for neutral non-team NPCs:
        // npcClass = npc & 0x7F
        // if (!hostile && notInTeam && (npcClass == 0 || npcClass > 0x66))
        //     npc = moraleThreshold | 0x80
        if (Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(ch)) {
            uint8 moraleValue = static_cast<uint8>(adnd->npc) & 0x7F;
            if (!ch->hostile && ch->combatState->notInTeam &&
                ((moraleValue == 0) || (moraleValue > 0x66))) {
                adnd->npc = static_cast<int8>(moraleThreshold | 0x80);
            }
        }

        // Set initial facing from approach direction table
        uint8 dirIndex = (wayFlag >> 1) & 0x03;
        ch->combatState->direction = Data::kCombatDirectionTable[dirIndex];

        // Hostile characters face the opposite direction
        if (ch->hostile)
            ch->combatState->direction = dirReverse(ch->combatState->direction);
    }
}

void freeCombatStates(Common::Array<Data::PlayerCharacter *> &combatants) {
    for (uint i = 0; i < combatants.size(); i++) {
        if (combatants[i]) {
            delete combatants[i]->combatState;
            combatants[i]->combatState = nullptr;
        }
    }
}

void setupCombat(CombatParams &params,
                 CombatGlobals &globals,
                 BattlefieldMap &map,
                 CombatantTable &table,
                 CombatPlacement &placement,
                 CombatViewport &viewport,
                 Data::Effects::EffectRuntime *effectRuntime) {
    // Step 1: Reset combat globals (mirrors original zeroing of globals)
    globals.reset();

    // Step 2: Read and clamp morale threshold from VM global memory when
    // available (authoritative source). Fall back to params field otherwise.
    uint8 moraleThreshold = params.moraleThreshold;
    if (params.eclMemory && params.vmGlobalLayout) {
        const VmFieldLocation moraleField =
            params.vmGlobalLayout->field(kVmGlobalFieldMoraleThreshold);
        if (VmLayout::isValid(moraleField)) {
            moraleThreshold = params.eclMemory->read8(moraleField.vmAddr);
            if (moraleThreshold > 100) {
                moraleThreshold = 100;
                // Mirror original behavior: clamp global value in VM state.
                params.eclMemory->write8(moraleField.vmAddr, moraleThreshold);
            }
            params.moraleThreshold = moraleThreshold;
        }
    }
    if (moraleThreshold > 100)
        moraleThreshold = 100;
    params.moraleThreshold = moraleThreshold;

    // Step 3: Build playfield data (COMBAT_BuildPlayfield)
    map.build(*params.geo,
              params.mapCenterX, params.mapCenterY, params.playerY,
              params.isDungeon, params.eclScriptId,
              params.wildX, params.wildY,
              params.mapType, params.terrainOverride);

    // Step 4: Init combatant states (COMBAT_InitCombatantStates)
    initCombatStates(params.roster, params.partyCount,
                     params.mapDirection, moraleThreshold);

    // Step 5: Place all combatants (COMBAT_AssignBattlefieldPositions)
    placement.placeAll(params.roster, params.partyCount,
                       params.mapDirection, params.encounterDistance,
                       map, params.combatTrigger, table, globals);

    // Step 6: Center viewport on active character (PTR_NEXT_CHAR)
    // Original: _PTR_COMBAT_FIELD[2] = charX - 3; [3] = charY - 3
    if (params.nextChar) {
        viewport.centerOn(table.getCharacterCol(params.nextChar),
                          table.getCharacterRow(params.nextChar));
    }

    // Step 7: Apply combat aura effects (EFFECT_applyEffectSet(8, ch))
    if (effectRuntime) {
        for (uint i = 0; i < params.roster.size(); i++) {
            Data::PlayerCharacter *ch = params.roster[i];
            if (!ch)
                continue;
            Data::Effects::CharacterEffects *effects = ch->getEffects();
            if (effects) {
                effectRuntime->applyTriggerSet(
                    Data::Effects::ETS_COMBAT_AURA,
                    *effects, *ch, &globals);
            }
        }
    }

    // Step 8: Update hostile health percentages
    updateHostileHealthPercent(table);
}

void updateHostileHealthPercent(const CombatantTable &table) {
    for (int i = 0; i < table.getCount(); i++) {
        if (table.getSize(i) == 0)
            continue;
        Data::PlayerCharacter *ch = table.getCharacter(i);
        if (!ch || !ch->hostile || !ch->combatState)
            continue;
        if (ch->hitPoints.max == 0)
            continue;
        // Store health as percentage (0-100) in AI state byte for UI
        uint8 pct = (uint8)((ch->hitPoints.current * 100) / ch->hitPoints.max);
        ch->combatState->aiState = pct;
    }
}

} // namespace Combat
} // namespace Goldbox
