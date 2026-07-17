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

#include "goldbox/gfx/combat_icon.h"
#include "goldbox/gfx/dax_renderer.h"
#include "goldbox/gfx/pic.h"

namespace Goldbox {
namespace Gfx {

CombatIcon::CombatIcon(DaxRenderer *renderer)
    : _renderer(renderer) {
}

CombatIcon::~CombatIcon() {
    for (auto it = _iconCache.begin(); it != _iconCache.end(); ++it) {
        delete it->_value;
    }
    _iconCache.clear();
}

uint32 CombatIcon::computeCacheKey(
        const Goldbox::Data::CombatIconData &iconData,
        IconState state,
        IconDirection direction) const {
    // Must include all color fields to avoid collisions between monsters
    // that share the same head/body sprites but have different colors.
    uint32 key = iconData.iconHead;
    key = (key << 5) ^ iconData.iconBody;
    key ^= ((uint32)iconData.iconColorBody1   << 0);
    key ^= ((uint32)iconData.iconColorBody2   << 4);
    key ^= ((uint32)iconData.iconColorArm1    << 8);
    key ^= ((uint32)iconData.iconColorArm2    << 12);
    key ^= ((uint32)iconData.iconColorLeg1    << 16);
    key ^= ((uint32)iconData.iconColorLeg2    << 20);
    key ^= ((uint32)iconData.iconColorHair    << 24);
    key ^= ((uint32)iconData.iconColorFace    << 28);
    key ^= ((uint32)iconData.iconColorShield1 * 0x1000003u);
    key ^= ((uint32)iconData.iconColorShield2 * 0x2000003u);
    key ^= ((uint32)iconData.iconColorWeapon1 * 0x4000003u);
    key ^= ((uint32)iconData.iconColorWeapon2 * 0x8000003u);
    key ^= ((uint32)iconData.iconSize         << 2);
    key ^= (static_cast<uint32>(state)        << 1);
    key ^= (static_cast<uint32>(direction)    << 0);
    return key;
}

Icon *CombatIcon::getOrCreateIcon(
        const Goldbox::Data::CombatIconData &iconData,
        IconState state,
        IconDirection direction) {
    uint32 key = computeCacheKey(iconData, state, direction);

    if (_iconCache.contains(key)) {
        Icon *cached = _iconCache[key];
        // Direction is part of the key so the cached icon already has the
        // correct orientation — no need to call setOrientation again.
        return cached;
    }

    Icon *icon;
    if (_renderer)
        icon = new Icon(iconData, _renderer, state);
    else
        icon = new Icon(iconData, state);

    // Set orientation before caching so the stored composite pointer is correct.
    icon->setOrientation(direction);

    _iconCache[key] = icon;
    return icon;
}

void CombatIcon::drawIcon(
        const Goldbox::Data::CombatIconData &iconData,
        IconState state,
        IconDirection direction,
        int x, int y,
        Graphics::ManagedSurface *dst,
        int32 colorOverride) {
    if (!dst)
        return;

    Icon *icon = getOrCreateIcon(iconData, state, direction);
    if (!icon)
        return;

    const Pic *composite = icon->getPic();
    if (!composite)
        return;

    if (colorOverride < 0) {
        composite->draw(dst, x, y);
        return;
    }

    Pic *effectPic = composite->clone();
    if (!effectPic)
        return;

    if (colorOverride >= 0 && colorOverride < 256)
        applyColorFilter(effectPic, static_cast<uint8>(colorOverride));

    effectPic->draw(dst, x, y);
    delete effectPic;
}

void CombatIcon::drawIconAtTile(
        const Goldbox::Data::CombatIconData &iconData,
        IconState state,
        IconDirection direction,
        int tileX, int tileY,
        Graphics::ManagedSurface *dst,
        int32 colorOverride) {
    if (!dst)
        return;

    Icon *icon = getOrCreateIcon(iconData, state, direction);
    if (!icon)
        return;

    const Pic *composite = icon->getPic();
    if (!composite)
        return;

    if (colorOverride < 0) {
        composite->drawAtIconPos(dst, tileX, tileY);
        return;
    }

    Pic *effectPic = composite->clone();
    if (!effectPic)
        return;

    if (colorOverride >= 0 && colorOverride < 256)
        applyColorFilter(effectPic, static_cast<uint8>(colorOverride));

    effectPic->drawAtIconPos(dst, tileX, tileY);
    delete effectPic;
}

void CombatIcon::applyColorFilter(Graphics::ManagedSurface *surface,
                                       uint8 colorIndex) {
    if (!surface)
        return;

    byte *pixels = (byte *)surface->getPixels();
    int pixelCount = surface->w * surface->h;

    for (int i = 0; i < pixelCount; ++i) {
        byte pixel = pixels[i];
        if (pixel != Icon::TRANSPARENT_COLOR_INDEX)
            pixels[i] = colorIndex;
    }
}

} // namespace Gfx
} // namespace Goldbox
