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

#include "common/list.h"
#include "common/str.h"
#include "goldbox/data/effects/effect.h"

namespace Goldbox {
namespace Data {
namespace Effects {

class CharacterEffects {
private:
    Common::List<Effect> _effects;

    Common::List<Effect>::iterator iteratorAt(uint idx) {
        Common::List<Effect>::iterator it = _effects.begin();
        for (uint i = 0; i < idx; ++i)
            ++it;
        return it;
    }

    Common::List<Effect>::const_iterator iteratorAt(uint idx) const {
        Common::List<Effect>::const_iterator it = _effects.begin();
        for (uint i = 0; i < idx; ++i)
            ++it;
        return it;
    }

public:
    bool load(const Common::String &filename);
    void loadFromStream(Common::SeekableReadStream &stream);
    bool save(const Common::String &filename) const;

    const Common::List<Effect> &effects() const { return _effects; }
    Common::List<Effect> &effects() { return _effects; }

    uint effectCount() const { return _effects.size(); }
    bool isEmpty() const { return _effects.empty(); }

    const Effect &effectAt(uint idx) const { return *iteratorAt(idx); }
    Effect &effectAt(uint idx) { return *iteratorAt(idx); }
    Effect &lastEffect() { return _effects.back(); }

    void removeEffectAt(uint idx) {
        if (idx >= _effects.size())
            return;
        _effects.erase(iteratorAt(idx));
    }
    // Unconditionally appends an effect. Stacking/dedup is the caller's responsibility.
    void appendEffect(const Effect &effect) { _effects.push_back(effect); }

    bool hasEffectType(uint8 type) const {
        for (Common::List<Effect>::const_iterator it = _effects.begin();
                it != _effects.end(); ++it) {
            if ((*it).type == type)
                return true;
        }
        return false;
    }

    int findEffectIndexByType(uint8 type) const {
        uint i = 0;
        for (Common::List<Effect>::const_iterator it = _effects.begin();
                it != _effects.end(); ++it, ++i) {
            if ((*it).type == type)
                return static_cast<int>(i);
        }
        return -1;
    }

    void clear() { _effects.clear(); }
};

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_CHARACTER_EFFECTS_H