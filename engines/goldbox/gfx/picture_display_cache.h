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

#ifndef GOLDBOX_GFX_PICTURE_DISPLAY_CACHE_H
#define GOLDBOX_GFX_PICTURE_DISPLAY_CACHE_H

#include "common/scummsys.h"
#include "common/array.h"
#include "common/ptr.h"

namespace Goldbox {
namespace Gfx {

class Pic;

/**
 * Cache for the PICTURE opcode's portrait/scene display.
 *
 * Separate from EncounterSpriteCache (which handles SPRITE_START/ADVANCE/OFF).
 * Supports multi-frame animated pictures (e.g. campfire PIC 29) with
 * ping-pong animation.
 *
 * Rendering priority in the view:
 *   1. If EncounterSpriteCache has sprite -> draw 3D + sprite overlay
 *   2. If PictureDisplayCache is active -> draw picture (replaces 3D view)
 *   3. Else -> draw normal 3D view
 */
class PictureDisplayCache {
public:
    PictureDisplayCache();
    ~PictureDisplayCache();

    /**
     * Load and activate a picture/portrait display.
     * @param headPicId  Head portrait ID (from D_PictureHeadId); 0xFF = scene pic
     * @param bodyPicId  Body/scene resource ID (PICTURE opcode parameter)
     */
    void load(uint8 headPicId, uint8 bodyPicId);

    /** Clear the picture display (PICTURE 0xFF or new area). */
    void clear();

    bool isActive() const { return _active; }
    uint8 headPicId() const { return _headPicId; }
    uint8 bodyPicId() const { return _bodyPicId; }

    /** Current display frame (accounts for animation). */
    const Pic *currentHeadFrame() const;
    const Pic *headPic() const { return _headPic.get(); }
    const Pic *bodyPic() const { return _bodyPic.get(); }

    /** True if the loaded picture has animation frames. */
    bool isAnimated() const { return _headFrames.size() > 1; }
    int headFrameCount() const { return (int)_headFrames.size(); }

    /**
     * Advance animation frame using ping-pong pattern.
     * Self-timed: checks elapsed time internally.
     * @param intervalMs Milliseconds per frame
     * @return True if frame changed (caller should redraw)
     */
    bool tickAnimation(uint32 intervalMs);

    /** Self-tick using internally stored interval. */
    bool tickAnimation();

    void setAnimInterval(uint32 intervalMs) { _animInterval = intervalMs; }

private:
    bool _active;
    uint8 _headPicId;
    uint8 _bodyPicId;
    Common::SharedPtr<Pic> _headPic;
    Common::SharedPtr<Pic> _bodyPic;

    Common::Array<Common::SharedPtr<Pic>> _headFrames;
    uint8 _currentFrame;
    int8 _animDirection;
    uint32 _lastFrameTime;
    uint32 _animInterval;
};

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_PICTURE_DISPLAY_CACHE_H
