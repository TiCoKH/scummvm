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

#ifndef WIZARDRY_RESOURCES_PICTURE_FILE_H
#define WIZARDRY_RESOURCES_PICTURE_FILE_H

#include "wizardry/profile.h"
#include "wizardry/resources/cell_image.h"

namespace Wizardry {

/**
 * The picture file of a graphics set (monsters, scenes, portraits).
 * A picture is 6 x 5 cells, or 12 x 14 cells in Wizardry V.
 */
class PictureFile {
private:
	struct Entry {
		uint32 offset;
		uint32 size;
	};

	Common::Array<byte> _data;
	Common::Array<Entry> _index;
	uint8 _cols = 0, _rows = 0;
	uint8 _cellW = 0, _cellH = 0;
	uint8 _planes = 1;
	bool _packed = false;

	uint pictureDataSize() const {
		return _cols * _rows * cellDataSize(_cellH, _planes);
	}

	bool readTable(const Common::Array<byte> &interp);
	void walkPictures();

	/**
	 * Undoes the zero suppression of the Wizardry V picture files.
	 * @returns the number of source bytes used
	 */
	static uint unpack(const byte *src, uint srcSize, Common::Array<byte> &dst, uint groups);

public:
	bool open(const Profile &profile, const GfxSet &gfx);

	uint size() const { return _index.size(); }

	/**
	 * Loads a picture.
	 * @param id  The number the game uses, starting at 1
	 */
	bool load(uint id, CellImage &image) const;
};

} // End of namespace Wizardry

#endif
