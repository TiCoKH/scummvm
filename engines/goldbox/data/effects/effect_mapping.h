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

#ifndef GOLDBOX_DATA_EFFECTS_EFFECT_MAPPING_H
#define GOLDBOX_DATA_EFFECTS_EFFECT_MAPPING_H

#include "common/scummsys.h"
#include "goldbox/data/effects/effect.h"

namespace Goldbox {
namespace Data {
namespace Effects {

struct EffectMapping {
    const Effects *table;
    uint size;

    Effects mapRaw(uint8 rawId) const {
        if (rawId < size)
            return table[rawId];
        return E_MISC;
    }
};

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_EFFECT_MAPPING_H
