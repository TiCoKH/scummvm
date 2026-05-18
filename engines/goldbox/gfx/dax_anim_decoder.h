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
 */

#ifndef GOLDBOX_GFX_DAX_ANIM_DECODER_H
#define GOLDBOX_GFX_DAX_ANIM_DECODER_H

#include "common/array.h"
#include "common/ptr.h"
#include "common/str.h"
#include "common/scummsys.h"

namespace Goldbox {
namespace Data {
class DaxBlockPic;
}

namespace Gfx {
class Pic;

/**
 * Resource key used to resolve legacy animation-specific decode rules.
 */
struct DaxAnimResourceKey {
    Common::String _resourceName;
    uint8 _blockId = 0xFF;
    bool _masked = false;
};

/**
 * Decoder facade for animated DAX PIC resources.
 *
 * This class owns no persistent frame state; callers provide output arrays.
 * Current implementation returns a stable frame cache scaffold (base frame +
 * clones for expected frame count). Full planar/delta frame decode can be
 * implemented behind this same API without touching callers.
 */
class DaxAnimDecoder {
public:
    DaxAnimDecoder() {}

    bool decodeFrames(const Data::DaxBlockPic &picBlock,
        const DaxAnimResourceKey &key,
        Common::Array<Common::SharedPtr<Pic> > &outFrames,
        uint8 &outFrameCount) const;

private:
    struct FrameDescriptor {
        uint16 _widthBytes;
        uint16 _height;
        uint16 _xPosChars;
        uint16 _yPosChars;
        uint32 _payloadOffset;
        uint32 _payloadSize;
    };

    uint8 applyLegacyFrameCaps(uint8 frameCount,
        const DaxAnimResourceKey &key) const;
    uint16 readUint16LE(const Common::Array<uint8> &data,
        uint32 offset) const;
    uint32 planarFrameByteSize(uint16 widthx8, uint16 height) const;
    uint8 countFramesAtDescriptorBase(const Data::DaxBlockPic &picBlock,
        uint8 requestedFrameCount, uint32 descriptorBase) const;
    bool buildFrameDescriptors(const Data::DaxBlockPic &picBlock,
        uint8 frameCount, uint32 descriptorBase,
        Common::Array<FrameDescriptor> &outDescriptors) const;
    bool decodeFromDescriptors(const Data::DaxBlockPic &picBlock,
        const DaxAnimResourceKey &key,
        const Common::Array<FrameDescriptor> &descriptors,
        Common::Array<Common::SharedPtr<Pic> > &outFrames) const;
    uint8 clampFrameCountByPayload(const Data::DaxBlockPic &picBlock,
        uint8 requestedFrameCount) const;
};

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_DAX_ANIM_DECODER_H
