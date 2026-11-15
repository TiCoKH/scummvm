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

#include "common/array.h"
#include "common/endian.h"
#include "common/util.h"
#include "goldbox/data/daxresourcefile.h"

namespace Goldbox {
namespace Data {

DaxResourceFile::DaxResourceFile() : _contentType(ContentType::UNKNOWN),
        _platform(Common::kPlatformUnknown) {
}

bool DaxResourceFile::open(const Common::Path &filename,
        Common::Platform platform) {
    if (!_file.open(filename)) {
        return false;
    }

    _contentType = determineContentType(filename);
    _platform = platform;
    if (platform == Common::kPlatformAmiga) {
        _headerContainer.parseHeadersBE(_file);
    } else {
        _headerContainer.parseHeadersLE(_file);
    }

    return true;
}

void DaxResourceFile::close() {
    _file.close();
}

bool DaxResourceFile::isOpen() const {
    return _file.isOpen();
}

ContentType DaxResourceFile::getContentType() const {
    return _contentType;
}

const DaxHeaderContainer &DaxResourceFile::getHeaderContainer() const {
    return _headerContainer;
}

DaxBlock *DaxResourceFile::loadBlock(const DaxHeader &header,
        ContentType contentType) {
    DaxBlock *daxBlock = DaxBlock::createDaxBlock(contentType);
    if (!daxBlock) {
        warning("DaxResourceFile::loadBlock: Unknown content type %d for block ID %d",
                (int)contentType, (int)header.id);
        return nullptr;
    }

    _file.seek(_headerContainer.getFileDataOffset() + header.offset, SEEK_SET);
    Common::Array<uint8> daxData(header.compSize);
    _file.read(daxData.data(), header.compSize);

    if (header.rawSize <= 0) {
        daxBlock->_data = daxData;
    } else {
        Common::Array<uint8> decodeData(header.rawSize);
        if (_platform == Common::kPlatformAmiga) {
            if (!unpackAmiga(daxData.data(), header.compSize,
                    decodeData.data(), header.rawSize))
                warning("DaxResourceFile::loadBlock: Amiga block %d failed to unpack",
                        (int)header.id);
        } else {
            decodeRLE(header.compSize, decodeData.data(), header.rawSize,
                    daxData.data());
        }
        daxBlock->_data = decodeData;
    }

    daxBlock->blockId = header.id;
    daxBlock->amigaPlanar = (_platform == Common::kPlatformAmiga);
    daxBlock->adjust();
    return daxBlock;
}

DaxBlock *DaxResourceFile::getBlockById(uint8 blockId,
        ContentType contentType) {
    ContentType resolvedType = contentType;
    if (resolvedType == ContentType::UNKNOWN) {
        resolvedType = _contentType;
    }

    const Common::Array<DaxHeader> &headers = _headerContainer.getHeaders();
    for (const auto &header : headers) {
        if (header.id == blockId) {
            return loadBlock(header, resolvedType);
        }
    }

    warning("DaxResourceFile::getBlockById: Block %d not found",
            (int)blockId);
    return nullptr;
}

void DaxResourceFile::decodeRLE(int dataLength, uint8 *outputPtr,
        int outputLength, const uint8 *inputPtr) {
    int inputIndex = 0;
    int outputIndex = 0;

    while (inputIndex + 1 < dataLength && outputIndex < outputLength) {
        int runLength = static_cast<int8>(inputPtr[inputIndex]);

        if (runLength >= 0) {
            int count = MIN(runLength + 1, outputLength - outputIndex);
            count = MIN(count, dataLength - inputIndex - 1);
            memcpy(outputPtr + outputIndex, inputPtr + inputIndex + 1, count);
            inputIndex += runLength + 2;
            outputIndex += count;
        } else {
            const int count = MIN(-runLength, outputLength - outputIndex);
            memset(outputPtr + outputIndex, inputPtr[inputIndex + 1], count);
            inputIndex += 2;
            outputIndex += count;
        }
    }
}

/**
 * Amiga DAX blocks are packed with a ByteKiller-style cruncher: the bit stream
 * is read backwards from the end and the output is written backwards. The last
 * three BE longwords hold the first bit word, a checksum and the unpacked size.
 */
bool DaxResourceFile::unpackAmiga(const uint8 *src, uint srcLength, uint8 *dst,
        uint dstLength) {
    if (srcLength < 12 || READ_BE_UINT32(src + srcLength - 4) != dstLength)
        return false;

    uint srcPos = srcLength - 12;
    uint32 bits = READ_BE_UINT32(src + srcPos);
    uint32 checksum = READ_BE_UINT32(src + srcLength - 8) ^ bits;

    auto getBit = [&]() -> uint {
        uint bit = bits & 1;
        bits >>= 1;
        if (bits == 0 && srcPos >= 4) {
            srcPos -= 4;
            const uint32 word = READ_BE_UINT32(src + srcPos);
            checksum ^= word;
            bit = word & 1;
            bits = 0x80000000 | (word >> 1);
        }
        return bit;
    };
    auto getBits = [&](int count) -> uint {
        uint value = 0;
        while (count-- > 0)
            value = (value << 1) | getBit();
        return value;
    };

    uint dstPos = dstLength;
    auto literal = [&](uint count) {
        while (count-- > 0 && dstPos > 0)
            dst[--dstPos] = getBits(8);
    };
    auto copy = [&](uint count, uint offset) {
        while (count-- > 0 && dstPos > 0) {
            --dstPos;
            dst[dstPos] = (dstPos + offset < dstLength) ? dst[dstPos + offset] : 0;
        }
    };

    while (dstPos > 0) {
        if (getBit()) {
            const uint type = getBits(2);
            if (type == 3) {
                literal(getBits(8) + 9);
            } else if (type < 2) {
                const uint count = type + 3;
                copy(count, getBits(9 + type));
            } else {
                const uint count = getBits(8) + 1;
                copy(count, getBits(12));
            }
        } else if (getBit()) {
            copy(2, getBits(8));
        } else {
            literal(getBits(3) + 1);
        }
    }

    return checksum == 0;
}

ContentType DaxResourceFile::determineContentType(const Common::Path &filename) {
    Common::String s = filename.toString();
    if (s.contains("8x8d")) return ContentType::TILE;
    if (s.contains("bacpac")) return ContentType::TILE;
    if (s.contains("dungcom")) return ContentType::TILE;
    if (s.contains("randcom")) return ContentType::TILE;
    if (s.contains("sqrpaci")) return ContentType::TILE;
    if (s.contains("back")) return ContentType::BACK;
    if (s.contains("bigpic")) return ContentType::BIGPIC;
    if (s.contains("cbody")) return ContentType::CTILE;
    if (s.contains("chead")) return ContentType::CTILE;
    if (s.contains("cpic")) return ContentType::CTILE;
    if (s.contains("comspr")) return ContentType::CTILE;
    if (s.contains("body")) return ContentType::PIC;
    if (s.contains("head")) return ContentType::PIC;
    if (s.contains("ecl")) return ContentType::ECL;
    if (s.contains("geo")) return ContentType::GEO;
    if (s.contains("cha")) return ContentType::CHARACTER;
    if (s.contains("itm")) return ContentType::ITEM;
    if (s.contains("spc")) return ContentType::SPELL;
    if (s.contains("pic")) return ContentType::PIC;
    if (s.contains("sprit")) return ContentType::SPRIT;
    if (s.contains("title")) return ContentType::PIC;
    if (s.contains("walldef")) return ContentType::WALLDEF;
    return ContentType::UNKNOWN;
}

} // namespace Data
} // namespace Goldbox
