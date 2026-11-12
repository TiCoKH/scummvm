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

#ifndef WIZARDRY_RESOURCES_MESSAGES_H
#define WIZARDRY_RESOURCES_MESSAGES_H

#include "common/array.h"
#include "common/str.h"

namespace Wizardry {

/**
 * One message file (ASCII.KRN, KANA.KRN or KANJI.KRN). The three files of
 * a release use the same message numbers.
 *
 * Strings are returned as the bytes the game uses: plain ASCII, character
 * set glyph numbers for kana, Shift-JIS for kanji.
 */
class MessageFile {
public:
	struct Entry {
		uint id;
		Common::String text;
	};

private:
	Common::Array<Entry> _entries;   // ordered by id
	Common::String _empty;

	void loadPlain(const Common::Array<byte> &data);
	void loadHuffman(const Common::Array<byte> &data, const Common::Array<byte> &table);
	void sortEntries();

public:
	/**
	 * @param name          The message file
	 * @param huffmanTable  File with the decoding table (Wizardry IV and V),
	 *                      or nullptr for the layout of Wizardry I to III
	 */
	bool open(const char *name, const char *huffmanTable);

	uint size() const { return _entries.size(); }
	const Entry &entry(uint index) const { return _entries[index]; }

	bool contains(uint id) const;

	/**
	 * Returns a message, or an empty string if there is none with this number
	 */
	const Common::String &get(uint id) const;
};

} // End of namespace Wizardry

#endif
