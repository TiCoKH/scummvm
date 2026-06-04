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

#include "goldbox/gfx/encounter_sprite_cache.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/vm_interface.h"
#include "goldbox/data/daxblock.h"
#include "common/debug.h"
#include "common/system.h"

namespace Goldbox {
namespace Gfx {

EncounterSpriteCache::EncounterSpriteCache()
    : _spriteLoaded(false), _headDrawn(false),
      _spriteBlockId(0), _bodyPicId(0),
      _distance(0), _lastHeadPicId(0xFF),
      _headCurrentFrame(0), _headLastFrameTime(0) {
}

EncounterSpriteCache::~EncounterSpriteCache() {
}

void EncounterSpriteCache::clear() {
    _spriteLoaded = false;
    _headDrawn = false;
    _spriteBlockId = 0;
    _bodyPicId = 0;
    _distance = 0;
    _lastHeadPicId = 0xFF;
    _spritePic.reset();
    _headPic.reset();
    _headFrames.clear();
    _headCurrentFrame = 0;
    _headLastFrameTime = 0;
}

void EncounterSpriteCache::loadSprite(uint8 spriteBlockId, uint8 bodyPicId,
        uint8 distance) {
    _spriteBlockId = spriteBlockId;
    _bodyPicId = bodyPicId;
    _distance = distance;
    _headDrawn = false;
    _lastHeadPicId = 0xFF;
    _spritePic.reset();

    // SPRIT DAX blocks are DaxBlockSprit (multi-frame EGA planar), not DaxBlockPic.
    Data::DaxBlock *rawBlock = VmInterface::getDaxSprit().getBlockById(spriteBlockId);
    if (!rawBlock) {
        _spriteLoaded = false;
        return;
    }

    Data::DaxBlockSprit *spritBlock = dynamic_cast<Data::DaxBlockSprit *>(rawBlock);
    if (!spritBlock) {
        debug(3, "EncounterSpriteCache::loadSprite: block %u is not DaxBlockSprit",
            (unsigned)spriteBlockId);
        _spriteLoaded = false;
        return;
    }

    if (!spritBlock->isValidLayout()) {
        // The strict layout validator may reject blocks with slightly different
        // header sizes or trailing padding. Accept the block if frameCount > 0
        // was parsed before the strict tail check failed.
        debug(3, "EncounterSpriteCache::loadSprite: block %u layout invalid "
            "(frameCount=%d, dataSize=%u)",
            (unsigned)spriteBlockId, spritBlock->frameCount(),
            (unsigned)rawBlock->_data.size());
    }

    if (spritBlock->frameCount() < 1) {
        debug(3, "EncounterSpriteCache::loadSprite: block %u has 0 frames",
            (unsigned)spriteBlockId);
        _spriteLoaded = false;
        return;
    }

    const uint8 maxFrame = static_cast<uint8>(spritBlock->frameCount() - 1);
    const uint8 clampedDistance = MIN<uint8>(distance, maxFrame);

    // Mark as loaded — frame rendering will use DaxBlockSprit::frameInfo()
    // and EGA plane decode at draw time.
    _spriteLoaded = true;
    _distance = clampedDistance;

    // Decode the frame at the requested distance into a Pic for rendering.
    _spritePic.reset(Gfx::Pic::readSpriteFrame(spritBlock,
        static_cast<int>(clampedDistance)));
}

void EncounterSpriteCache::loadHead(uint8 headPicId, uint8 bodyPicId) {
    _lastHeadPicId = headPicId;
    _headFrames.clear();
    _headCurrentFrame = 0;
    _headLastFrameTime = 0;

    Data::DaxBlockPic *block = nullptr;

    if (headPicId == 0xFF) {
        Data::DaxBlock *rawBlock = VmInterface::getDaxPic().getBlockById(bodyPicId);
        block = rawBlock ? dynamic_cast<Data::DaxBlockPic *>(rawBlock) : nullptr;
    } else {
        Data::DaxBlock *rawBlock = VmInterface::getDaxHead().getBlockById(headPicId);
        block = rawBlock ? dynamic_cast<Data::DaxBlockPic *>(rawBlock) : nullptr;
    }

    if (!block) {
        _headPic.reset();
        _headDrawn = false;
        return;
    }

    const int frames = (block->frameCount > 1) ? block->frameCount : 1;
    for (int i = 0; i < frames; ++i) {
        Pic *frame = Pic::readFrame(block, i);
        if (frame) {
            _headFrames.push_back(Common::SharedPtr<Pic>(frame));
        } else {
            break;
        }
    }

    if (!_headFrames.empty()) {
        _headPic = _headFrames[0];
        _headLastFrameTime = g_system->getMillis();
    } else {
        _headPic.reset();
    }

    _headDrawn = (_headPic.get() != nullptr);
}

const Pic *EncounterSpriteCache::currentHeadFrame() const {
    if (_headFrames.empty())
        return _headPic.get();
    return _headFrames[_headCurrentFrame].get();
}

bool EncounterSpriteCache::tickAnimation(uint32 intervalMs) {
    if (_headFrames.size() <= 1)
        return false;

    uint32 now = g_system->getMillis();
    if (now - _headLastFrameTime < intervalMs)
        return false;

    _headCurrentFrame = (_headCurrentFrame + 1) % _headFrames.size();
    _headPic = _headFrames[_headCurrentFrame];
    _headLastFrameTime = now;
    return true;
}

void EncounterSpriteCache::setDistance(uint8 distance) {
    // Re-decode the sprite frame for the new distance.
    if (!_spriteLoaded || _spriteBlockId == 0xFF)
        return;

    Data::DaxBlock *rawBlock = VmInterface::getDaxSprit().getBlockById(_spriteBlockId);
    Data::DaxBlockSprit *spritBlock = rawBlock
        ? dynamic_cast<Data::DaxBlockSprit *>(rawBlock) : nullptr;
    if (!spritBlock || spritBlock->frameCount() < 1) {
        _spritePic.reset();
        return;
    }

    const uint8 maxFrame = static_cast<uint8>(spritBlock->frameCount() - 1);
    const uint8 clampedDistance = MIN<uint8>(distance, maxFrame);
    _distance = clampedDistance;
    _spritePic.reset(Pic::readSpriteFrame(spritBlock,
        static_cast<int>(clampedDistance)));
}

} // namespace Gfx
} // namespace Goldbox
