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

#ifndef GOLDBOX_DATA_DAXHEADERCONTAINER_H
#define GOLDBOX_DATA_DAXHEADERCONTAINER_H

#include "common/array.h"
#include "common/stream.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {
namespace Data {

    class DaxHeaderContainer {
    private:
        Common::Array<DaxHeader> headers;
        uint16 dataOffset = 0;

    public:
        void parseHeadersLE(Common::ReadStream &stream);
        void parseHeadersBE(Common::ReadStream &stream);
        const Common::Array<DaxHeader>& getHeaders() const;
        uint16 getFileDataOffset() const;
    };

} // namespace Data
} // namespace GoldBox

#endif // GOLDBOX_DAXHEADERCONTAINER_H
