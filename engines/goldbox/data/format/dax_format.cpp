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

#include "goldbox/data/format/dax_format.h"
#include "goldbox/data/format/amiga_dax_format.h"
#include "goldbox/data/format/dos_dax_format.h"
#include "goldbox/data/format/pc98_dax_format.h"

namespace Goldbox {
namespace Data {

DaxFormat *DaxFormat::create(Common::Platform platform) {
	switch (platform) {
	case Common::kPlatformAmiga:
		return new AmigaDaxFormat();
	case Common::kPlatformPC98:
		return new Pc98DaxFormat();
	default:
		return new DosDaxFormat();
	}
}

Common::String DaxFormat::singleFileName(const char *pattern) {
	Common::String name(pattern);
	const size_t pos = name.find("%d");
	if (pos != Common::String::npos)
		name.erase(pos, 2);
	return name + ".dax";
}

} // namespace Data
} // namespace Goldbox
