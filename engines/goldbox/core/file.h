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

#ifndef GOLDBOX_CORE_FILE_H
#define GOLDBOX_CORE_FILE_H

#include "common/file.h"
#include "goldbox/data/daxheadercontainer.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {

class File : public Common::File {
private:
    Data::DaxHeaderContainer headerContainer;
    Data::ContentType _ctype;

    Data::ContentType determineContentType(const Common::Path &filename);

public:
	File() : Common::File() {}
	File(const Common::Path &filename);
	File(const char *filename);
    bool open(const Common::Path& filename) override;
    void parseHeaders();
    Data::DaxBlock* getBlockById(int blockId);
    void decodeRLE(int dataLength, uint8 *output_ptr, const uint8 *input_ptr);
};

} // namespace Goldbox

#endif // GOLDBOX_CORE_FILE_H
