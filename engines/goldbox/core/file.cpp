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
#include "common/memstream.h"
#include "common/util.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/core/file.h"

namespace Goldbox {

File::File(const Common::Path &filename) {
	File::open(filename);
}

File::File(const char *filename) {
	File::open(Common::Path(filename));
}

bool File::open(const Common::Path &filename) {
    if (!Common::File::open(filename)) {
        error("Could not open file - %s", filename.toString(Common::Path::kNativeSeparator).c_str());
        return false;
    }
    _ctype = determineContentType(filename);
    parseHeaders();
    return true;
}

void File::parseHeaders() {
    Common::Platform platform = g_engine->getPlatform();
    if (platform == Common::kPlatformAmiga) {
        headerContainer.parseHeadersBE(*this);
    } else {
        headerContainer.parseHeadersLE(*this);
    }
}

Data::ContentType File::determineContentType(const Common::Path &filename) {
    Common::String s = filename.toString();
    if (s.contains("8x8d")) return Data::ContentType::TILE;
    if (s.contains("bacpac")) return Data::ContentType::TILE;
    if (s.contains("dungcom")) return Data::ContentType::TILE;
    if (s.contains("randcom")) return Data::ContentType::TILE;
    if (s.contains("sqrpaci")) return Data::ContentType::TILE;
    if (s.contains("back")) return Data::ContentType::BACK;
    if (s.contains("bigpic")) return Data::ContentType::BIGPIC;
    if (s.contains("cbody")) return Data::ContentType::CBODY;
    if (s.contains("body")) return Data::ContentType::PIC;
    if (s.contains("comspr")) return Data::ContentType::COMSPR;
    if (s.contains("ecl")) return Data::ContentType::ECL;
    if (s.contains("geo")) return Data::ContentType::GEO;
    if (s.contains("chead")) return Data::ContentType::CHEAD;
    if (s.contains("head")) return Data::ContentType::PIC;
    if (s.contains("moncha")) return Data::ContentType::MONCHA;
    if (s.contains("pic")) return Data::ContentType::PIC;
    if (s.contains("sprit")) return Data::ContentType::SPRIT;
    if (s.contains("title")) return Data::ContentType::PIC;
    if (s.contains("walldef")) return Data::ContentType::WALLDEF;
    return Data::ContentType::UNKNOWN;
}

Data::DaxBlock* File::getBlockById(int blockId) {
    for (const auto &header : headerContainer.getHeaders()) {
        if (header.id == blockId) {
            seek(headerContainer.getFileDataOffset() + header.offset, SEEK_SET);
            Common::Array<uint8> dax_data(header.compSize);
            read(dax_data.data(), header.compSize);

            Data::DaxBlock* daxBlock = Data::DaxBlock::createDaxBlock(_ctype);
            if (!daxBlock) {
                error("Unknown content type for block ID: %d", blockId);
                return nullptr;
            }

            if (header.rawSize <= 0) {
                daxBlock->_data = dax_data;
            } else {
                Common::Array<uint8> decode_data(header.rawSize);
                decodeRLE(header.compSize, decode_data.data(), dax_data.data());
                daxBlock->_data = decode_data;
            }
            daxBlock->blockId = blockId;
            daxBlock->adjust();
            return daxBlock;
        }
    }
    error("Block not found: %d", blockId);
    return nullptr;
}

void File::decodeRLE(int dataLength, uint8 *output_ptr, const uint8 *input_ptr) {
    int input_index = 0;
    int output_index = 0;

    while (input_index < dataLength) {
        int8 run_length = static_cast<int8>(input_ptr[input_index]);

        if (run_length >= 0) {
            memcpy(output_ptr + output_index, input_ptr + input_index + 1, run_length + 1);
            input_index += run_length + 2;
            output_index += run_length + 1;
        } else {
            run_length = -run_length;
            memset(output_ptr + output_index, input_ptr[input_index + 1], run_length);
            input_index += 2;
            output_index += run_length;
        }
    }
}

} // namespace Goldbox
