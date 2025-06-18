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

#ifndef GOLDBOX_DATA_STRINGS_DATA_H
#define GOLDBOX_DATA_STRINGS_DATA_H

#include "common/hash-str.h"
#include "common/path.h"

namespace Goldbox {
namespace Data {

class StringsData : public Common::StringMap {
public:
    StringsData() : Common::StringMap() {}

    /**
     * Loads the data
     */
    bool load(const Common::Path &filename);

    /**
     * Gets all strings under a specific node prefix
     * @param prefix The node prefix to search for (e.g. "menu" will find "menu.item1", "menu.item2", etc.)
     * @return Array of string values under that prefix
     */
    Common::Array<Common::String> getNodeValues(const Common::String &prefix);
};

} // namespace Data
} // namespace Goldbox

#endif
