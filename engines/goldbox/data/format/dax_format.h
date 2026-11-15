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

#ifndef GOLDBOX_DATA_FORMAT_DAX_FORMAT_H
#define GOLDBOX_DATA_FORMAT_DAX_FORMAT_H

#include "common/array.h"
#include "common/path.h"
#include "common/platform.h"
#include "common/str.h"
#include "common/stream.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {
namespace Data {

class DaxBlockContainer;
class DaxHeaderContainer;

/**
 * Platform layout of the DAX resource files. Every port stores the same
 * blocks with its own directory, packing, file names and picture format.
 * A DaxFormat turns them into the common (DOS) block layout, so code outside
 * data/format never depends on the platform.
 */
class DaxFormat {
public:
	virtual ~DaxFormat() {}

	/** Create the format of a platform; DOS for any other platform. */
	static DaxFormat *create(Common::Platform platform);

	/** Read the block directory at the start of a DAX file. */
	virtual void parseHeaders(Common::ReadStream &stream,
		DaxHeaderContainer &headers) const = 0;

	/** Unpack a block; out is already sized to the unpacked size. */
	virtual bool unpack(const Common::Array<uint8> &packed,
		Common::Array<uint8> &out) const = 0;

	/**
	 * Convert an unpacked block of a DAX file to the common (DOS) layout.
	 * @return false when the block can't be used on this platform.
	 */
	virtual bool normalize(const Common::Path &file, ContentType type,
		uint8 blockId, Common::Array<uint8> &data) const = 0;

	/**
	 * Append the files of a DAX family. The pattern holds "%d" where DOS
	 * numbers one file per area ("8x8d%d"); dosIndices is the set of those
	 * numbers as bits, or 0 for a family stored in a single file ("cbody").
	 */
	virtual void familyFiles(const char *pattern, uint16 dosIndices,
		Common::Array<Common::Path> &files) const = 0;

	/**
	 * Add block ids a DOS file has but this platform shares with another
	 * block, after a file was loaded into the container.
	 */
	virtual void addAliases(const Common::Path &file,
		DaxBlockContainer &container) const {}

protected:
	/** "8x8d%d" -> "8x8d.dax": the name of a family kept in one file. */
	static Common::String singleFileName(const char *pattern);
};

} // namespace Data
} // namespace Goldbox

#endif
