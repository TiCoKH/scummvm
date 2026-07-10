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
 */

#include "goldbox/gfx/combat_renderer.h"
#include "goldbox/gfx/combat_icon.h"

namespace Goldbox {
namespace Gfx {

CombatRenderer::CombatRenderer() {
    _cicon = new CombatIcon(nullptr);
}

CombatRenderer::CombatRenderer(DaxRenderer *renderer) {
    _cicon = new CombatIcon(renderer);
}

CombatRenderer::~CombatRenderer() {
    delete _cicon;
}

void CombatRenderer::drawCombatIcon(const IconRenderParams &params,
                                    Graphics::ManagedSurface *dst) {
    if (!dst)
        return;

    // Integration placeholder: indexed icon lookup path is still not wired.
    (void)params;
}

void CombatRenderer::drawIcon(const Goldbox::Data::CombatIconData &iconData,
                              IconState state,
                              IconDirection direction,
                              int x, int y,
                              Graphics::ManagedSurface *dst,
                              int32 colorOverride) {
    _cicon->drawIcon(iconData, state, direction, x, y, dst, colorOverride);
}

void CombatRenderer::drawIconAtTile(
        const Goldbox::Data::CombatIconData &iconData,
        IconState state,
        IconDirection direction,
        int tileX, int tileY,
        Graphics::ManagedSurface *dst,
        int32 colorOverride) {
    _cicon->drawIconAtTile(iconData, state, direction, tileX, tileY,
                           dst, colorOverride);
}

void CombatRenderer::applyColorFilter(Graphics::ManagedSurface *surface,
                                      uint8 colorIndex) {
    CombatIcon::applyColorFilter(surface, colorIndex);
}

} // namespace Gfx
} // namespace Goldbox
