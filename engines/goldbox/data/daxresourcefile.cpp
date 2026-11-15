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

#include "goldbox/data/daxresourcefile.h"
#include "common/array.h"
#include "common/textconsole.h"
#include "goldbox/data/format/dax_format.h"

namespace Goldbox {
namespace Data {

DaxResourceFile::DaxResourceFile() : _contentType(ContentType::UNKNOWN),
        _format(nullptr) {
}

bool DaxResourceFile::open(const Common::Path &filename,
        const DaxFormat &format) {
    if (!_file.open(filename)) {
        return false;
    }

    _contentType = determineContentType(filename);
    _format = &format;
    _format->parseHeaders(_file, _headerContainer);

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
        daxBlock->_data.resize(header.rawSize);
        if (!_format->unpack(daxData, daxBlock->_data))
            warning("DaxResourceFile::loadBlock: block %d failed to unpack",
                    (int)header.id);
    }

    if (!_format->normalize(contentType, header.id, daxBlock->_data)) {
        delete daxBlock;
        return nullptr;
    }

    daxBlock->blockId = header.id;
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
