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

#ifndef GOLDBOX_GFX_ENCOUNTER_SPRITE_CACHE_H
#define GOLDBOX_GFX_ENCOUNTER_SPRITE_CACHE_H

#include "common/scummsys.h"
#include "common/array.h"
#include "common/ptr.h"

namespace Graphics {
class ManagedSurface;
}

namespace Goldbox {
namespace Gfx {

class Pic;

/**
 * Engine-global cache for encounter (monster approach) sprite and head/body
 * picture resources.
 *
 * Mirrors the original runtime globals:
 *   - ARRAY_DRAW_STATE[0] = isSpriteLoaded (sprite DAX decoded and ready)
 *   - ARRAY_DRAW_STATE[1] = isHeadDrawn (head/portrait blitted this encounter)
 *   - BYTE_SPRITE_ID      = sprite_block_id (SPRIT DAX block index)
 *   - BYTE_BODY_PIC_ID    = body_or_pic_id (PIC DAX block index for scene pic)
 *   - BYTE_PIC_FRAMES     = decoded sprite frame data (here: Pic pointer)
 *   - D_MonsterDistance    = current display distance (0=adjacent, 1=mid, 2=far)
 *   - D_DistanceCap       = max distance from opcode operand
 *   - BYTE_PREV_HEAD_ID   = last drawn head pic ID (change detection)
 *
 * DAX resource mapping:
 *   - SPRIT DAX: encounter approach sprite (monster body at distance)
 *   - PIC DAX:   scene picture (used when D_PictureHeadId == 0xFF)
 *   - HEAD DAX:  full-size NPC portrait (used when D_PictureHeadId != 0xFF)
 *   - CHEAD/CBODY DAX: combat-map icon sprites (NOT used here)
 *
 * Lifecycle:
 *   - Populated by opcode 0x0C (SPRITE_START) via host drawEncounterStage()
 *   - Updated by opcode 0x0D (SPRITE_ADVANCE) via host redrawEncounterStage()
 *   - Cleared by opcode 0x31 (SPRITE_OFF) via host spriteOff()
 *   - Read by InGameMainScreenDialog::draw() for 3D viewport overlay
 *
 * Ownership: Engine owns this cache. Pic pointers are owned by the cache.
 */
class EncounterSpriteCache {
public:
    EncounterSpriteCache();
    ~EncounterSpriteCache();

    /** Clear all state (sprite off / new area). */
    void clear();

    /**
     * Load sprite frames from SPRIT DAX and set up encounter state.
     * Corresponds to the first-time path in GFX_drawEncounterStage where
     * draw_state->isSpriteLoaded transitions from false to true.
     *
     * @param spriteBlockId  SPRIT DAX block index
     * @param bodyPicId      PIC/BODY DAX block index (for head drawing)
     * @param distance       Calculated (clamped) monster distance
     */
    void loadSprite(uint8 spriteBlockId, uint8 bodyPicId, uint8 distance);

    /**
     * Load head/portrait picture from PIC or HEAD DAX.
     * Called when distance==0 and head pic ID has changed.
     *
     * When headPicId == 0xFF: loads full scene pic from PIC DAX (body_or_pic_id).
     * Otherwise: loads NPC portrait from HEAD DAX (headPicId).
     * Note: CHEAD/CBODY are combat-icon sprites, not used here.
     *
     * @param headPicId  Head picture ID (0xFF = use PIC DAX scene picture)
     * @param bodyPicId  PIC DAX block ID (used when headPicId == 0xFF)
     */
    void loadHead(uint8 headPicId, uint8 bodyPicId);

    /** Update distance (SPRITE_ADVANCE). */
    void setDistance(uint8 distance);

    // --- State queries for dialog drawing ---

    bool isSpriteLoaded() const { return _spriteLoaded; }
    bool isHeadDrawn() const { return _headDrawn; }
    uint8 spriteBlockId() const { return _spriteBlockId; }
    uint8 bodyPicId() const { return _bodyPicId; }
    uint8 distance() const { return _distance; }
    uint8 lastHeadPicId() const { return _lastHeadPicId; }

    /** Decoded sprite Pic (nullptr until EGA frame decode is implemented). */
    const Pic *spritePic() const { return _spritePic.get(); }

    /** Decoded head/portrait Pic (nullptr if not loaded). */
    const Pic *headPic() const { return _headPic.get(); }

    /**
     * Current display frame for animated PIC blocks (frameCount > 1).
     * Returns the Pic for the active animation frame, cycling through
     * all frames. Returns headPic() for single-frame pictures.
     */
    const Pic *currentHeadFrame() const;

    /** True if the loaded head picture has animation frames. */
    bool isAnimated() const { return _headFrames.size() > 1; }

    /** Total frame count for the loaded head picture. */
    int headFrameCount() const { return (int)_headFrames.size(); }

    /**
     * Advance animation frame using ping-pong pattern (1→max→1).
     * Self-timed: checks elapsed time internally.
     * Call from any periodic update path (tick, draw, timeout).
     * @param intervalMs Milliseconds per frame (e.g. gameSpeed * 500)
     * @return True if frame changed (caller should redraw)
     */
    bool tickAnimation(uint32 intervalMs);

    /**
     * Set the animation frame interval in milliseconds.
     * Allows external control of animation speed.
     */
    void setAnimInterval(uint32 intervalMs) { _headAnimInterval = intervalMs; }

    /**
     * Self-tick: advances animation using internally stored interval.
     * @return True if frame changed
     */
    bool tickAnimation();

    /** Mark head as drawn (prevents redundant reloads). */
    void setHeadDrawn(bool drawn) { _headDrawn = drawn; }

private:
    bool _spriteLoaded;
    bool _headDrawn;
    uint8 _spriteBlockId;
    uint8 _bodyPicId;
    uint8 _distance;
    uint8 _lastHeadPicId;

    Common::SharedPtr<Pic> _spritePic;
    Common::SharedPtr<Pic> _headPic;

    // Multi-frame PIC animation state.
    Common::Array<Common::SharedPtr<Pic>> _headFrames;
    uint8 _headCurrentFrame;
    int8 _headAnimDirection;  // +1 forward, -1 reverse (ping-pong)
    uint32 _headLastFrameTime;
    uint32 _headAnimInterval; // ms per frame
};

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_ENCOUNTER_SPRITE_CACHE_H
