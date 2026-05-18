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

#include "goldbox/gfx/dax_anim_decoder.h"

#include "goldbox/data/daxblock.h"
#include "goldbox/gfx/pic.h"

namespace {

bool isXorDeltaResource(const Goldbox::Gfx::DaxAnimResourceKey &key) {
    return key._resourceName.equalsIgnoreCase("PIC")
        || key._resourceName.equalsIgnoreCase("FINAL");
}

uint8 decodePixelWithMask(uint8 pixel, bool masked) {
    if (!masked)
        return pixel;

    if (pixel == 0)
        return 0xFF;

    if (pixel == 13 || pixel == 8)
        return 0;

    return pixel;
}

} // namespace

namespace Goldbox {
namespace Gfx {

uint16 DaxAnimDecoder::readUint16LE(const Common::Array<uint8> &data,
        uint32 offset) const {
    if (offset + 1 >= data.size())
        return 0;

    return static_cast<uint16>(data[offset]
        | (static_cast<uint16>(data[offset + 1]) << 8));
}

uint32 DaxAnimDecoder::planarFrameByteSize(uint16 widthx8,
        uint16 height) const {
    // Frame payload uses packed 4bpp: widthBytes*8 pixels per row,
    // i.e. widthBytes*4 bytes per row.
    return static_cast<uint32>(widthx8) * static_cast<uint32>(height) * 4;
}

uint8 DaxAnimDecoder::countFramesAtDescriptorBase(
        const Data::DaxBlockPic &picBlock,
        uint8 requestedFrameCount,
        uint32 descriptorBase) const {
    if (requestedFrameCount <= 1)
        return requestedFrameCount;

    const Common::Array<uint8> &data = picBlock._data;
    const uint32 descriptorSize = 16;
    const uint32 tableBytes = descriptorBase
        + static_cast<uint32>(requestedFrameCount) * descriptorSize;

    if (data.size() < tableBytes)
        return 0;

    uint32 payloadOffset = tableBytes;
    uint8 validFrames = 0;
    for (uint8 i = 0; i < requestedFrameCount; ++i) {
        const uint32 d = descriptorBase + static_cast<uint32>(i) * descriptorSize;
        const uint16 widthx8 = readUint16LE(data, d + 6);
        const uint16 height = readUint16LE(data, d + 4);
        const uint32 frameBytes = planarFrameByteSize(widthx8, height);

        if (widthx8 == 0 || height == 0 || frameBytes == 0)
            break;

        if (payloadOffset + frameBytes > data.size())
            break;

        payloadOffset += frameBytes;
        validFrames++;
    }

    return validFrames;
}

bool DaxAnimDecoder::buildFrameDescriptors(
        const Data::DaxBlockPic &picBlock,
        uint8 frameCount,
        uint32 descriptorBase,
        Common::Array<FrameDescriptor> &outDescriptors) const {
    outDescriptors.clear();

    if (frameCount == 0)
        return false;

    const Common::Array<uint8> &data = picBlock._data;
    const uint32 descriptorSize = 16;
    const uint32 tableBytes = descriptorBase
        + static_cast<uint32>(frameCount) * descriptorSize;

    if (data.size() < tableBytes)
        return false;

    uint32 payloadOffset = tableBytes;
    for (uint8 i = 0; i < frameCount; ++i) {
        const uint32 d = descriptorBase + static_cast<uint32>(i) * descriptorSize;
        FrameDescriptor fd;
        fd._height = readUint16LE(data, d + 4);
        fd._widthBytes = readUint16LE(data, d + 6);
        fd._xPosChars = readUint16LE(data, d + 8);
        fd._yPosChars = readUint16LE(data, d + 10);
        fd._payloadSize = planarFrameByteSize(fd._widthBytes, fd._height);
        fd._payloadOffset = payloadOffset;

        if (fd._widthBytes == 0 || fd._height == 0 || fd._payloadSize == 0)
            return false;

        if (fd._payloadOffset + fd._payloadSize > data.size())
            return false;

        outDescriptors.push_back(fd);
        payloadOffset += fd._payloadSize;
    }

    return outDescriptors.size() == frameCount;
}

bool DaxAnimDecoder::decodeFromDescriptors(
        const Data::DaxBlockPic &picBlock,
        const DaxAnimResourceKey &key,
        const Common::Array<FrameDescriptor> &descriptors,
        Common::Array<Common::SharedPtr<Pic> > &outFrames) const {
    outFrames.clear();

    const bool xorDelta = isXorDeltaResource(key);
    Common::Array<uint8> firstFramePayload;
    Common::Array<uint8> framePayload;

    const int defaultCanvasW = picBlock.width > 0 ? picBlock.width : 1;
    const int defaultCanvasH = picBlock.height > 0 ? picBlock.height : 1;

    for (uint32 i = 0; i < descriptors.size(); ++i) {
        const FrameDescriptor &fd = descriptors[i];

        framePayload.clear();
        framePayload.resize(fd._payloadSize);
        for (uint32 b = 0; b < fd._payloadSize; ++b)
            framePayload[b] = picBlock._data[fd._payloadOffset + b];

        if (xorDelta && i == 0) {
            firstFramePayload = framePayload;
        } else if (xorDelta && i > 0) {
            if (firstFramePayload.size() != framePayload.size())
                return false;

            for (uint32 b = 0; b < framePayload.size(); ++b)
                framePayload[b] ^= firstFramePayload[b];
        }

        const int frameW = static_cast<int>(fd._widthBytes) * 8;
        const int frameH = static_cast<int>(fd._height);
        const int offsetX = static_cast<int>(fd._xPosChars) * 8;
        const int offsetY = static_cast<int>(fd._yPosChars) * 8;

        int canvasW = defaultCanvasW;
        int canvasH = defaultCanvasH;
        if (canvasW < offsetX + frameW)
            canvasW = offsetX + frameW;
        if (canvasH < offsetY + frameH)
            canvasH = offsetY + frameH;

        Pic *pic = new Pic(canvasW, canvasH);
        pic->setTransparentIndex(key._masked ? 0xFF : 0);

        const uint8 clearPixel = key._masked ? 0xFF : 0;
        for (int y = 0; y < canvasH; ++y) {
            for (int x = 0; x < canvasW; ++x)
                pic->setPixel(x, y, clearPixel);
        }

        const uint32 bytesPerRow = static_cast<uint32>(fd._widthBytes) * 4;
        for (int y = 0; y < frameH; ++y) {
            const uint32 rowStart = static_cast<uint32>(y) * bytesPerRow;
            for (uint32 bx = 0; bx < bytesPerRow; ++bx) {
                const uint8 packed = framePayload[rowStart + bx];
                const uint8 high = decodePixelWithMask((packed & 0xF0) >> 4,
                    key._masked);
                const uint8 low = decodePixelWithMask(packed & 0x0F,
                    key._masked);

                const int px = offsetX + static_cast<int>(bx * 2);
                const int py = offsetY + y;

                if (px >= 0 && py >= 0 && px < canvasW && py < canvasH)
                    pic->setPixel(px, py, high);
                if (px + 1 >= 0 && py >= 0 && px + 1 < canvasW && py < canvasH)
                    pic->setPixel(px + 1, py, low);
            }
        }

        outFrames.push_back(Common::SharedPtr<Pic>(pic));
    }

    return !outFrames.empty();
}

uint8 DaxAnimDecoder::clampFrameCountByPayload(
        const Data::DaxBlockPic &picBlock,
        uint8 requestedFrameCount) const {
    const uint8 validBase4 = countFramesAtDescriptorBase(picBlock,
        requestedFrameCount, 4);
    const uint8 validBase1 = countFramesAtDescriptorBase(picBlock,
        requestedFrameCount, 1);

    const uint8 validFrames = (validBase4 >= validBase1)
        ? validBase4 : validBase1;

    if (validFrames == 0)
        return 1;

    return validFrames;
}

uint8 DaxAnimDecoder::applyLegacyFrameCaps(uint8 frameCount,
        const DaxAnimResourceKey &key) const {
    // x86 parity: PIC block 1 caps animation to first 4 frames.
    if (key._resourceName.equalsIgnoreCase("PIC")
            && key._blockId == 1 && frameCount > 4) {
        return 4;
    }

    return frameCount;
}

bool DaxAnimDecoder::decodeFrames(const Data::DaxBlockPic &picBlock,
        const DaxAnimResourceKey &key,
        Common::Array<Common::SharedPtr<Pic> > &outFrames,
        uint8 &outFrameCount) const {
    outFrames.clear();
    outFrameCount = 0;

    uint8 frameCount = static_cast<uint8>(picBlock.frameCount);
    frameCount = applyLegacyFrameCaps(frameCount, key);
    frameCount = clampFrameCountByPayload(picBlock, frameCount);

    // Ensure at least one drawable frame for resources with missing metadata.
    if (frameCount == 0)
        frameCount = 1;

    if (frameCount > 1) {
        Common::Array<FrameDescriptor> descriptors;
        const uint8 validBase4 = countFramesAtDescriptorBase(picBlock,
            frameCount, 4);
        const uint8 validBase1 = countFramesAtDescriptorBase(picBlock,
            frameCount, 1);
        const uint32 descriptorBase = (validBase4 >= validBase1) ? 4 : 1;

        if (buildFrameDescriptors(picBlock, frameCount, descriptorBase,
                descriptors)
                && decodeFromDescriptors(picBlock, key, descriptors,
                    outFrames)
                && outFrames.size() == frameCount) {
            outFrameCount = frameCount;
            return true;
        }
    }

    Pic *firstFrame = key._masked
        ? Pic::readWithRemapping(const_cast<Data::DaxBlockPic *>(&picBlock), 13, 0)
        : Pic::read(const_cast<Data::DaxBlockPic *>(&picBlock));
    if (!firstFrame)
        return false;

    outFrames.push_back(Common::SharedPtr<Pic>(firstFrame));

    // Fallback behavior if animated descriptor decode is unavailable.
    for (uint8 i = 1; i < frameCount; ++i) {
        outFrames.push_back(Common::SharedPtr<Pic>(firstFrame->clone()));
    }

    outFrameCount = frameCount;
    return true;
}

} // namespace Gfx
} // namespace Goldbox
