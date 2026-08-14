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
public:
    bool load(const Common::String &filename);
    void loadFromStream(Common::SeekableReadStream &stream);
    bool save(const Common::String &filename) const;

    const Common::List<Effect> &effects() const { return _effects; }
    Common::List<Effect> &effects() { return _effects; }

    uint effectCount() const { return _effects.size(); }
    bool isEmpty() const { return _effects.empty(); }

    Effect &lastEffect() { return _effects.back(); }

    void appendEffect(const Effect &effect) { _effects.push_back(effect); }

    // Returns a pointer to the first effect with the given id, or nullptr.
    Effect *findEffectById(uint8 id) {
        for (Effect &e : _effects)
            if (e.id == id)
                return &e;
        return nullptr;
    }

    const Effect *findEffectById(uint8 id) const {
        for (const Effect &e : _effects)
            if (e.id == id)
                return &e;
        return nullptr;
    }

    bool hasEffect(uint8 id) const { return findEffectById(id) != nullptr; }

    // Raw erase with no handler callback. Use only when handler notification
    // is intentionally bypassed (e.g. spell interrupt on damage).
    void eraseEffectById(uint8 id) {
        for (Common::List<Effect>::iterator it = _effects.begin();
                it != _effects.end(); ++it) {
            if (it->id == id) {
                _effects.erase(it);
                return;
            }
        }
    }

    void clear() { _effects.clear(); }

private:
    Common::List<Effect> _effects;
};

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_CHARACTER_EFFECTS_H