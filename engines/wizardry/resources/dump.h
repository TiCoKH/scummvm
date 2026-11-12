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

#ifndef WIZARDRY_RESOURCES_DUMP_H
#define WIZARDRY_RESOURCES_DUMP_H

#include "common/stream.h"
#include "wizardry/profile.h"

namespace Wizardry {

/**
 * Where dumpResources() puts its files
 */
class DumpTarget {
public:
	virtual ~DumpTarget() {}

	/**
	 * Creates an output file. The caller deletes the stream.
	 */
	virtual Common::WriteStream *create(const char *name) = 0;
};

/**
 * Loads every resource of a release through the resource classes and
 * writes what they return, for comparing with pcode_tools/wiz_extract.py:
 *
 *   charset_NNN.bin    all glyphs, one after another, a byte per pixel
 *   pictures_NNN.bin   all pictures, one after another, a byte per pixel
 *   title_NNN.bin      the title, a byte per pixel
 *   messages_*.txt     one line per message: number, tab, bytes in hex
 *   scenario.txt       title and table of contents
 *   scenario.bin       all records of types 1 to 7, one after another
 *   summary.txt        what was found
 *
 * NNN is "200" or "400". Both sets are written if the release has them.
 */
extern void dumpResources(const Profile &profile, DumpTarget &target);

} // End of namespace Wizardry

#endif
