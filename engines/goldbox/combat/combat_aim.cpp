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

#include "goldbox/combat/combat_aim.h"
#include "goldbox/combat/combat_session.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/tile_property_provider.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/items/character_item.h"

namespace Goldbox {
namespace Combat {

namespace {

const uint8 kTileDownedMember = 0x1F;

} // namespace

uint8 resolveAimRange(const Data::PlayerCharacter *attacker, uint8 range) {
    uint8 result = range;
    if (result == 0xFF) {
        result = 1;
        const Data::ADnDCharacter *adnd =
            dynamic_cast<const Data::ADnDCharacter *>(attacker);
        const Data::Items::CharacterItem *weapon = adnd
            ? adnd->getEquippedItem(Data::Items::Slot::S_MAIN_HAND) : nullptr;
        if (weapon)
            result = static_cast<uint8>(weapon->prop().range - 1);
    }
    if (result == 0 || result == 0xFF)
        result = 1;
    return result;
}

bool rangedAttackAllowed(CombatSession &session,
                         Data::PlayerCharacter *attacker) {
    Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(attacker);
    if (!adnd || !adnd->hasRangedWeapon())
        return true;

    Data::Items::CharacterItem *ammo = nullptr;
    if (!adnd->getRangedAttackItem(&ammo))
        return false;

    // COMBAT_BuildTargetList(1): enemies adjacent to the attacker.
    CombatContext *ctx = session.getContext();
    if (!ctx)
        return false;
    ctx->buildTargetList(attacker, 1);
    if (ctx->targetList.targetOrder.empty())
        return true;
    return adnd->isEquippedRangedWeapon();
}

bool aimTargetOffered(CombatSession &session, Data::PlayerCharacter *attacker,
                      Data::PlayerCharacter *target, uint8 range,
                      bool weaponMode) {
    CombatContext *ctx = session.getContext();
    if (!ctx || !attacker || !target)
        return false;
    if (ctx->getTargetRange(attacker, target) > range)
        return false;
    if (!weaponMode)
        return true;
    if (attacker == target)
        return false;
    return rangedAttackAllowed(session, attacker);
}

AimCell evaluateAimCell(CombatSession &session, const AimRequest &request,
                        uint8 range, TilePos cell) {
    AimCell result;
    CombatContext *ctx = session.getContext();
    Data::PlayerCharacter *attacker = request.attacker;
    if (!ctx || !attacker)
        return result;

    const CombatantTable &table = session.getTable();
    const TilePos origin = table.getCharacterPos(attacker);

    uint16 rawRange = 0xFF;
    result.lineOfSight = ctx->lineOfSightCheck(origin, cell, rawRange);
    result.range = static_cast<uint8>(MIN<uint16>(rawRange >> 1, 0xFF));

    const CombatContext::CombatCell contents = ctx->getTileAndOccupantAt(cell);
    const bool corpseCell = contents.occupantId == 0 &&
        contents.tileId == kTileDownedMember;

    if (result.lineOfSight) {
        if (contents.occupantId != 0) {
            result.occupant = table.getCharacter(contents.occupantId - 1);
        } else if (corpseCell) {
            // The last matching record wins, as in the original scan.
            const Common::Array<CombatGlobals::DownedMemberRecord> &downed =
                session.getGlobals().downedMembers;
            for (uint i = 0; i < downed.size(); ++i) {
                if (downed[i].pos == cell)
                    result.occupant = downed[i].character;
            }
        }
    }

    bool valid = result.lineOfSight;
    const TilePropertyProvider *props =
        session.getBattlefieldMap().getTilePropertyProvider();
    if (result.range > range || (props && props->isImpassable(contents.tileId)))
        valid = false;

    if (!result.occupant) {
        if (!request.groundOk)
            valid = false;
    } else {
        if (!ctx->canEngageTarget(attacker, result.occupant))
            valid = false;
        if (request.weaponMode) {
            if (result.occupant == attacker || corpseCell)
                valid = false;
            else if (!rangedAttackAllowed(session, attacker))
                valid = false;
        }
    }

    result.valid = valid;
    return result;
}

} // namespace Combat
} // namespace Goldbox
