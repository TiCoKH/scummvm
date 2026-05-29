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

namespace Goldbox {
namespace Gfx {

EncounterSpriteCache::EncounterSpriteCache()
    : _spriteLoaded(false), _headDrawn(false),
      _spriteBlockId(0), _bodyPicId(0),
      _distance(0), _lastHeadPicId(0xFF) {
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

    // Mark as loaded — frame rendering will use DaxBlockSprit::frameInfo()
    // and EGA plane decode at draw time.
    _spriteLoaded = true;

    // Decode the frame at the requested distance into a Pic for rendering.
    _spritePic.reset(Gfx::Pic::readSpriteFrame(spritBlock,
        static_cast<int>(distance)));
}

void EncounterSpriteCache::loadHead(uint8 headPicId, uint8 bodyPicId) {
    _lastHeadPicId = headPicId;

    if (headPicId == 0xFF) {
        // Use body pic from PIC DAX (GFX_DrawDaxPICFrames("PIC", bodyPicId, 0))
        Data::DaxBlock *rawBlock = VmInterface::getDaxPic().getBlockById(bodyPicId);
        Data::DaxBlockPic *block = rawBlock
            ? dynamic_cast<Data::DaxBlockPic *>(rawBlock) : nullptr;
        if (block)
            _headPic.reset(Pic::read(block));
        else
            _headPic.reset();
    } else {
        // Use full-size portrait from HEAD DAX (GFX_drawPortrait).
        // Note: CHEAD/CBODY are small combat-icon sprites, not these.
        Data::DaxBlock *rawBlock = VmInterface::getDaxHead().getBlockById(headPicId);
        Data::DaxBlockPic *block = rawBlock
            ? dynamic_cast<Data::DaxBlockPic *>(rawBlock) : nullptr;
        if (block)
            _headPic.reset(Pic::read(block));
        else
            _headPic.reset();
    }

    _headDrawn = (_headPic.get() != nullptr);
}

void EncounterSpriteCache::setDistance(uint8 distance) {
    _distance = distance;

    // Re-decode the sprite frame for the new distance.
    if (_spriteLoaded && _spriteBlockId != 0xFF) {
        Data::DaxBlock *rawBlock = VmInterface::getDaxSprit().getBlockById(_spriteBlockId);
        Data::DaxBlockSprit *spritBlock = rawBlock
            ? dynamic_cast<Data::DaxBlockSprit *>(rawBlock) : nullptr;
        if (spritBlock && distance < static_cast<uint8>(spritBlock->frameCount()))
            _spritePic.reset(Pic::readSpriteFrame(spritBlock, static_cast<int>(distance)));
        else
            _spritePic.reset();
    }
}

} // namespace Gfx
} // namespace Goldbox
