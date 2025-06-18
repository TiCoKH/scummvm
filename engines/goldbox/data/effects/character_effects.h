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

 #ifndef GOLDBOX_DATA_EFFECTS_CHARACTER_EFFECTS_H
 #define GOLDBOX_DATA_EFFECTS_CHARACTER_EFFECTS_H

#include "common/array.h"
#include "common/str.h"
#include "goldbox/data/effects/effect.h"

namespace Goldbox {
namespace Data {
namespace Effects {

class CharacterEffects {
private:
    Common::Array<Effect> _effects;

public:
    bool load(const Common::String &filename);
    bool save(const Common::String &filename) const;

    const Common::Array<Effect> &effects() const { return _effects; }
    Common::Array<Effect> &effects() { return _effects; }

    void clear() { _effects.clear(); }
};

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_CHARACTER_EFFECTS_H