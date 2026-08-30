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

#ifndef GOLDBOX_COMBAT_CLOUD_EFFECT_MANAGER_H
#define GOLDBOX_COMBAT_CLOUD_EFFECT_MANAGER_H

#include "common/list.h"
#include "common/scummsys.h"
#include "goldbox/core/tile_pos.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
namespace Effects {
class EffectHostBridge;
} // namespace Effects
} // namespace Data

namespace Combat {

/**
 * Persistent state for one active Stinking Cloud instance.
 *
 * The cloud occupies up to 4 cells around its center, determined by
 * kCloudDirections = {8, 2, 3, 4} (wire-format directions: NoMove, E, SE, S).
 * Cells blocked by impassable tiles are inactive (activeTile[i] == false).
 *
 * Power byte encoding: bits 0-3 = casting level, bits 4-7 = cloudIndex.
 * Both nibbles max at 15, preserving original behaviour.
 */
struct CloudEffect {
    Data::PlayerCharacter *owner;
    TilePos center;
    uint8 cloudIndex;       // owner-relative index (effect power >> 4)
    uint8 savedTile[4];     // original map tile under each cell before cloud placed
    uint8 occupantIcon[4];  // combatant/terrain icon saved at each cell
    bool  activeTile[4];    // true if cell is cloud-affected (not impassable)

    CloudEffect()
        : owner(nullptr), center(), cloudIndex(0) {
        for (int i = 0; i < 4; ++i) {
            savedTile[i]    = 0;
            occupantIcon[i] = 0;
            activeTile[i]   = false;
        }
    }
};

/**
 * Manages all active Stinking Cloud instances during combat.
 *
 * Lives as a member of CombatGlobals and is exposed via CombatContext::clouds.
 * Mirrors the original PTR_CLOUD_EFF_HANDLER linked list, using
 * Common::List to preserve the original traversal semantics.
 *
 * Responsibilities:
 *   - Allocate a cloud record and paint the initial tile overlay (create)
 *   - Restore tiles and rebuild overlays when a cloud expires (expire)
 *   - Answer queries used by the spell handler (countOwnedBy, findAtTile)
 */
class CloudEffectManager {
public:
    // Tile IDs used for cloud overlay and downed-member restoration.
    static const uint8 kTileCloud  = 0x1E;
    static const uint8 kTileDowned = 0x1F;  // matches CombatantTable::TILE_DOWNED_MEMBER

    void reset();

    /**
     * Called by Spell_ID34_StinkingCloud.
     * Samples the four cells around (centerX, centerY), saves their tiles,
     * paints kTileCloud on active cells, and appends the record.
     * Returns the cloudIndex assigned to this cloud (= countOwnedBy before insert).
     * Resolves BattlefieldMap and CombatantTable via g_engine->getCombatContext().
     */
    uint8 create(Data::PlayerCharacter *owner, TilePos center);

    /**
     * Called by the E_IN_STINKING_CLOUD EFF_REMOVE handler.
     * Posts the expiry message via bridge, restores tiles, removes the
     * cloud record, then repaints kTileCloud for all remaining clouds.
     * Resolves BattlefieldMap and CombatantTable via g_engine->getCombatContext().
     */
    void expire(Data::PlayerCharacter *owner, uint8 cloudIndex,
                Data::Effects::EffectHostBridge *bridge);

    /** Number of clouds currently owned by this character. */
    uint8 countOwnedBy(const Data::PlayerCharacter *owner) const;

    /**
     * Find the first cloud whose active cells include (x, y).
     * Optionally excludes one entry (used during create to skip self).
     */
    const CloudEffect *findAtTile(TilePos pos,
                                  const CloudEffect *exclude = nullptr) const;

    bool isEmpty() const { return _clouds.empty(); }

private:
    // Wire-format directions for the four cloud cells: NoMove, E, SE, S.
    static const uint8 kCloudDirections[4];

    Common::List<CloudEffect> _clouds;

    void getCellPos(const CloudEffect &c, int dir, TilePos &out) const;
    void rebuildOverlays() const;
};

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_CLOUD_EFFECT_MANAGER_H
