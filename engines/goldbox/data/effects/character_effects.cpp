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

#include "common/file.h"
#include "goldbox/data/effects/character_effects.h"

namespace Goldbox {
namespace Data {
namespace Effects {

bool CharacterEffects::load(const Common::String &filename) {
    Common::File f;
    if (!f.open(filename.c_str()))
        return false;

    _effects.clear();

    while (!f.eos()) {
        Effect effect;
        if (f.pos() + 9 > f.size())
            break;

        effect.load(f);
        _effects.push_back(effect);
    }

    f.close();
    return true;
}

bool CharacterEffects::save(const Common::String &filename) const {
    Common::DumpFile f;
    if (!f.open(filename.c_str()))
        return false;

    for (const auto &effect : _effects)
        effect.save(f);

    f.flush();
    f.close();
    return true;
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox
