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

#include "common/util.h"
#include "goldbox/gfx/combat_renderer.h"

namespace Goldbox {
namespace Gfx {

CombatRenderer::CombatRenderer() : _renderer(nullptr), _lastIconIndex(0xFFFFFFFF) {
}

CombatRenderer::CombatRenderer(DaxRenderer *renderer) : _renderer(renderer), _lastIconIndex(0xFFFFFFFF) {
}

CombatRenderer::~CombatRenderer() {
	// Clean up cached icons
	for (auto it = _iconCache.begin(); it != _iconCache.end(); ++it) {
		delete it->_value;
	}
	_iconCache.clear();
}

uint32 CombatRenderer::computeCacheKey(const Goldbox::Data::CombatIconData &iconData,
                                       IconState state,
                                       IconDirection direction) const {
	// Combine: head (0-13), body (0-31), state (0-1), direction (0-1)
	// into a 32-bit hash for cache lookup
	uint32 key = iconData.iconHead;
	key = (key << 5) | iconData.iconBody;
	key = (key << 1) | static_cast<uint32>(state);
	key = (key << 1) | static_cast<uint32>(direction);
	return key;
}

Icon *CombatRenderer::getOrCreateIcon(const Goldbox::Data::CombatIconData &iconData,
                                      IconState state,
                                      IconDirection direction) {
	uint32 key = computeCacheKey(iconData, state, direction);

	if (_iconCache.contains(key)) {
		return _iconCache[key];
	}

	// Create new icon (with renderer if available)
	Icon *icon;
	if (_renderer) {
		icon = new Icon(iconData, _renderer, state);
	} else {
		icon = new Icon(iconData, state);
	}

	// Apply orientation
	icon->setOrientation(direction);

	_iconCache[key] = icon;
	return icon;
}

void CombatRenderer::drawCombatIcon(const IconRenderParams &params,
                                    Graphics::ManagedSurface *dst) {
	if (!dst) {
		return;
	}

	// TODO: Map iconIndex from params to CombatIconData lookup
	// Current architecture requires CombatIconData for Icon creation
	// This is a design point where we need a character/icon manager bridge
	// For now, this method is intended for future integration with indexed icon lookups
}

void CombatRenderer::drawIcon(const Goldbox::Data::CombatIconData &iconData,
                              IconState state,
                              IconDirection direction,
                              int x, int y,
                              Graphics::ManagedSurface *dst,
                              int32 colorOverride) {
	if (!dst) {
		return;
	}

	Icon *icon = getOrCreateIcon(iconData, state, direction);
	if (!icon) {
		return;
	}

	const Pic *composite = icon->getPic();
	if (!composite) {
		return;
	}

	// If no effects, render directly
	if (colorOverride < 0) {
		composite->draw(dst, x, y);
		return;
	}

	// Apply color filter via temporary copy
	Pic *effectPic = composite->clone();
	if (!effectPic) {
		return;
	}

	if (colorOverride >= 0 && colorOverride < 256) {
		applyColorFilter(effectPic, static_cast<uint8>(colorOverride));
	}

	effectPic->draw(dst, x, y);
	delete effectPic;
}

void CombatRenderer::drawIconAtTile(const Goldbox::Data::CombatIconData &iconData,
                                    IconState state,
                                    IconDirection direction,
                                    int tileX, int tileY,
                                    Graphics::ManagedSurface *dst,
                                    int32 colorOverride) {
	if (!dst) {
		return;
	}

	Icon *icon = getOrCreateIcon(iconData, state, direction);
	if (!icon) {
		return;
	}

	const Pic *composite = icon->getPic();
	if (!composite) {
		return;
	}

	// If no effects, render directly
	if (colorOverride < 0) {
		composite->drawAtIconPos(dst, tileX, tileY);
		return;
	}

	// Apply color filter via temporary copy
	Pic *effectPic = composite->clone();
	if (!effectPic) {
		return;
	}

	if (colorOverride >= 0 && colorOverride < 256) {
		applyColorFilter(effectPic, static_cast<uint8>(colorOverride));
	}

	effectPic->drawAtIconPos(dst, tileX, tileY);
	delete effectPic;
}

void CombatRenderer::applyColorFilter(Graphics::ManagedSurface *surface, uint8 colorIndex) {
	if (!surface) {
		return;
	}

	byte *pixels = (byte *)surface->getPixels();
	int pixelCount = surface->w * surface->h;

	for (int i = 0; i < pixelCount; ++i) {
		byte pixel = pixels[i];

		// Pixel 0 is transparent; skip it
		if (pixel != Icon::TRANSPARENT_COLOR_INDEX) {
			pixels[i] = colorIndex;
		}
	}
}

} // namespace Gfx
} // namespace Goldbox


