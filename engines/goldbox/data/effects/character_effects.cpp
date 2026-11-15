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

// Implementation of runtime character effect list handling.
// Mirrors original linked-list style behaviour with list-backed storage.
#include "common/file.h"
#include "goldbox/data/effects/character_effects.h"

namespace Goldbox {
namespace Data {
namespace Effects {

void CharacterEffects::loadFromStream(Common::SeekableReadStream &f,
        const RecordLayout &layout) {
    _effects.clear();

    const int32 fileSize = f.size();
    if (fileSize <= 0)
        return;

    const int nodeSize = Effect::recordSize(layout);
    if (fileSize % nodeSize != 0) {
        warning("CharacterEffects::loadFromStream: size %d not multiple of %d",
            fileSize, nodeSize);
    }

    const int count = fileSize / nodeSize;
    for (int i = 0; i < count; ++i) {
        Effect e;
        e.load(f, layout);
        _effects.push_back(e);
    }
}

bool CharacterEffects::load(const Common::String &filename,
        const RecordLayout &layout) {
    Common::File f;
    if (!f.open(filename.c_str()))
        return false;

    loadFromStream(f, layout);

    f.close();
    return true;
}

bool CharacterEffects::save(const Common::String &filename,
        const RecordLayout &layout) const {
    Common::DumpFile f;
    if (!f.open(filename.c_str()))
        return false;

    for (const auto &effect : _effects)
        effect.save(f, layout);

    f.flush();
    f.close();
    return true;
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox
