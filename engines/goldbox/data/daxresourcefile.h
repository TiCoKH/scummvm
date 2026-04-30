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

#ifndef GOLDBOX_DATA_DAXRESOURCEFILE_H
#define GOLDBOX_DATA_DAXRESOURCEFILE_H

#include "common/path.h"
#include "common/platform.h"
#include "goldbox/core/file.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/data/daxheadercontainer.h"

namespace Goldbox {
namespace Data {

class DaxResourceFile {
private:
    File _file;
    DaxHeaderContainer _headerContainer;
    ContentType _contentType;

    static void decodeRLE(int dataLength, uint8 *outputPtr,
            const uint8 *inputPtr);
    static ContentType determineContentType(const Common::Path &filename);

public:
    DaxResourceFile();

    bool open(const Common::Path &filename, Common::Platform platform);
    void close();
    bool isOpen() const;

    ContentType getContentType() const;
    const DaxHeaderContainer &getHeaderContainer() const;

    DaxBlock *loadBlock(const DaxHeader &header, ContentType contentType);
    DaxBlock *getBlockById(uint8 blockId,
            ContentType contentType = ContentType::UNKNOWN);
};

} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_DAXRESOURCEFILE_H
