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

#ifndef GOLDBOX_GFX_MENU_ANIMATION_H
#define GOLDBOX_GFX_MENU_ANIMATION_H

#include "common/array.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {
namespace Gfx {

/**
 * Per-frame display times of a PIC block in milliseconds (LoadAnimation keeps
 * at most 4 frames of PIC 1).
 */
inline Common::Array<uint32> picFrameDelaysMs(const Data::DaxBlockSprit &block,
		uint8 picId, int loadedFrames) {
	Common::Array<uint32> delays;
	int count = loadedFrames;
	if (picId == 1 && count > 4)
		count = 4;
	for (int i = 0; i < count; ++i) {
		const Data::DaxBlockSprit::FrameInfo *fi = block.frameInfo(i);
		delays.push_back(fi ? fi->delay * 100 : 0);
	}
	return delays;
}

/**
 * MenuHorizontal picture animation: frames cycle 0..count-1 and wrap, each
 * shown for its own delay; a frame without one uses fallbackMs.
 * @return true when the frame changed
 */
inline bool advanceMenuAnimation(uint8 &current, uint32 &lastTime, uint32 now,
		uint frameCount, const Common::Array<uint32> &delaysMs,
		uint32 fallbackMs) {
	if (frameCount <= 1)
		return false;

	uint32 delay = (current < delaysMs.size()) ? delaysMs[current] : 0;
	if (delay == 0)
		delay = fallbackMs;
	if (now - lastTime < delay)
		return false;

	lastTime = now;
	current = static_cast<uint8>((current + 1) % frameCount);
	return true;
}

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_MENU_ANIMATION_H
