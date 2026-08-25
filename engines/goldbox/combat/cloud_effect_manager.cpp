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

#include "goldbox/combat/cloud_effect_manager.h"

#include "goldbox/combat/battlefield_map.h"
#include "goldbox/combat/tile_property_provider.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/core/direction.h"
#include "goldbox/runtime/effect_host_bridge.h"
#include "goldbox/engine.h"

namespace Goldbox {
namespace Combat {

// Wire-format directions for the four cloud cells: NoMove(8), E(2), SE(3), S(4).
const uint8 CloudEffectManager::kCloudDirections[4] = { 8, 2, 3, 4 };

void CloudEffectManager::reset() {
    _clouds.clear();
}

void CloudEffectManager::getCellPos(const CloudEffect &c, int dir,
                                    uint8 &x, uint8 &y) const {
    const uint8 wireDir = kCloudDirections[dir];
    x = (uint8)(c.centerX + kDirDeltaX[wireDir]);
    y = (uint8)(c.centerY + kDirDeltaY[wireDir]);
}

uint8 CloudEffectManager::create(Data::PlayerCharacter *owner,
                                 uint8 centerX, uint8 centerY) {
    CombatContext *ctx = Goldbox::g_engine->getCombatContext();
    if (!ctx)
        return 0;

    BattlefieldMap &map = ctx->map;
    const CombatantTable &table = ctx->table;

    CloudEffect cloud;
    cloud.owner      = owner;
    cloud.centerX    = centerX;
    cloud.centerY    = centerY;
    cloud.cloudIndex = countOwnedBy(owner);

    const TilePropertyProvider *props = map.getTilePropertyProvider();

    for (int dir = 0; dir < 4; ++dir) {
        uint8 x, y;
        getCellPos(cloud, dir, x, y);

        const uint8 rawTile = map.getRawTile(x, y);
        cloud.savedTile[dir] = rawTile;

        if (props && props->isImpassable(rawTile)) {
            cloud.activeTile[dir]   = false;
            cloud.occupantIcon[dir] = 0;
            continue;
        }

        cloud.activeTile[dir] = true;

        // Resolve the icon to save under this cell.
        uint8 icon = rawTile;

        if (rawTile == kTileCloud) {
            // Another cloud already occupies this cell — inherit its saved icon.
            const CloudEffect *other = findAtTile(x, y, nullptr);
            if (other) {
                for (int od = 0; od < 4; ++od) {
                    if (!other->activeTile[od])
                        continue;
                    uint8 ox, oy;
                    getCellPos(*other, od, ox, oy);
                    if (ox == x && oy == y &&
                            other->occupantIcon[od] != kTileCloud) {
                        icon = other->occupantIcon[od];
                        break;
                    }
                }
            }
        } else if (rawTile == kTileDowned) {
            // Downed party member — find their original saved tile.
            const Common::Array<CombatantTable::DownedMemberRecord> &downed =
                table.getDownedMembers();
            for (uint i = 0; i < downed.size(); ++i) {
                if (downed[i].tileCol == x && downed[i].tileRow == y) {
                    icon = downed[i].savedTile;
                    break;
                }
            }
        }

        cloud.occupantIcon[dir] = icon;
        map.setRawTile(x, y, kTileCloud);
    }

    _clouds.push_back(cloud);
    return cloud.cloudIndex;
}

void CloudEffectManager::expire(Data::PlayerCharacter *owner,
                                uint8 cloudIndex,
                                Data::Effects::EffectHostBridge *bridge) {
    CombatContext *ctx = Goldbox::g_engine->getCombatContext();
    if (!ctx)
        return;

    BattlefieldMap &map = ctx->map;
    const CombatantTable &table = ctx->table;
    Common::List<CloudEffect>::iterator it = _clouds.begin();
    for (; it != _clouds.end(); ++it) {
        if (it->owner == owner && it->cloudIndex == cloudIndex)
            break;
    }
    if (it == _clouds.end())
        return;

    if (bridge)
        bridge->postEffectMessage(owner, "The air clears a little...", true);

    // Restore the four cells covered by this cloud.
    const Common::Array<CombatantTable::DownedMemberRecord> &downed =
        table.getDownedMembers();

    for (int dir = 0; dir < 4; ++dir) {
        if (!it->activeTile[dir])
            continue;

        uint8 x, y;
        getCellPos(*it, dir, x, y);

        // If a downed member is at this position, restore the downed tile.
        bool hasDowned = false;
        for (uint i = 0; i < downed.size(); ++i) {
            if (downed[i].tileCol == x && downed[i].tileRow == y) {
                map.setRawTile(x, y, kTileDowned);
                hasDowned = true;
                break;
            }
        }

        if (!hasDowned)
            map.setRawTile(x, y, it->savedTile[dir]);
    }

    _clouds.erase(it);
    rebuildOverlays();
}

uint8 CloudEffectManager::countOwnedBy(const Data::PlayerCharacter *owner) const {
    uint8 count = 0;
    for (const CloudEffect &c : _clouds) {
        if (c.owner == owner)
            ++count;
    }
    return count;
}

const CloudEffect *CloudEffectManager::findAtTile(uint8 x, uint8 y,
                                                   const CloudEffect *exclude) const {
    for (const CloudEffect &c : _clouds) {
        if (&c == exclude)
            continue;
        for (int dir = 0; dir < 4; ++dir) {
            if (!c.activeTile[dir])
                continue;
            uint8 cx, cy;
            getCellPos(c, dir, cx, cy);
            if (cx == x && cy == y)
                return &c;
        }
    }
    return nullptr;
}

void CloudEffectManager::rebuildOverlays() const {
    CombatContext *ctx = Goldbox::g_engine->getCombatContext();
    if (!ctx)
        return;
    BattlefieldMap &map = ctx->map;
    for (const CloudEffect &c : _clouds) {
        for (int dir = 0; dir < 4; ++dir) {
            if (!c.activeTile[dir])
                continue;
            uint8 x, y;
            getCellPos(c, dir, x, y);
            map.setRawTile(x, y, kTileCloud);
        }
    }
}

} // namespace Combat
} // namespace Goldbox
