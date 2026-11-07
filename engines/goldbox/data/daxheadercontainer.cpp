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

#include "goldbox/data/daxheadercontainer.h"

namespace Goldbox {
namespace Data {

    void DaxHeaderContainer::parseHeadersLE(Common::ReadStream &stream) {
        uint16 headersBlockSize = stream.readUint16LE();
        dataOffset = headersBlockSize + 2;

        int numberOfHeaders = headersBlockSize / 9;
        headers.clear();
        headers.resize(numberOfHeaders);

        for (int i = 0; i < numberOfHeaders; ++i) {
            DaxHeader header;
            header.id = stream.readByte();
            header.offset = stream.readUint32LE();
            header.rawSize = stream.readUint16LE();
            header.compSize = stream.readUint16LE();
            headers[i] = header;
        }
    }

    void DaxHeaderContainer::parseHeadersBE(Common::ReadStream &stream) {
        uint16 headersBlockSize = stream.readUint16BE();
        dataOffset = headersBlockSize + 2;

        int numberOfHeaders = headersBlockSize / 10;
        headers.clear();
        headers.resize(numberOfHeaders);

        for (int i = 0; i < numberOfHeaders; ++i) {
            DaxHeader header;
            header.id = stream.readUint16BE();
            header.offset = stream.readUint32BE();
            header.rawSize = stream.readUint16BE();
            header.compSize = stream.readUint16BE();
            headers[i] = header;
        }
    }

    const Common::Array<DaxHeader>& DaxHeaderContainer::getHeaders() const {
        return headers;
    }

    uint16 DaxHeaderContainer::getFileDataOffset() const {
        return dataOffset;
    }

} // namespace Data
} // namespace GoldBox