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

#ifndef GOLDBOX_GFX_COMBAT_ICON_LOGIC_H
#define GOLDBOX_GFX_COMBAT_ICON_LOGIC_H

#include "common/scummsys.h"
#include "common/hashmap.h"
#include "goldbox/gfx/icon.h"

namespace Goldbox {
namespace Gfx {

class DaxRenderer;

class CombatIconLogic {
public:
    explicit CombatIconLogic(DaxRenderer *renderer);
    ~CombatIconLogic();

    void drawIcon(const Goldbox::Data::CombatIconData &iconData,
                  IconState state,
                  IconDirection direction,
                  int x, int y,
                  Graphics::ManagedSurface *dst,
                  int32 colorOverride);

    void drawIconAtTile(const Goldbox::Data::CombatIconData &iconData,
                        IconState state,
                        IconDirection direction,
                        int tileX, int tileY,
                        Graphics::ManagedSurface *dst,
                        int32 colorOverride);

    static void applyColorFilter(Graphics::ManagedSurface *surface,
                                 uint8 colorIndex);

private:
    uint32 computeCacheKey(const Goldbox::Data::CombatIconData &iconData,
                           IconState state,
                           IconDirection direction) const;

    Icon *getOrCreateIcon(const Goldbox::Data::CombatIconData &iconData,
                          IconState state,
                          IconDirection direction);

    DaxRenderer *_renderer;
    Common::HashMap<uint32, Icon *> _iconCache;
};

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_COMBAT_ICON_LOGIC_H
