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

#include "goldbox/gfx/first_person_renderer.h"

#include "graphics/managed_surface.h"
#include "goldbox/gfx/pic.h"

namespace Goldbox {
namespace Gfx {

// Movement deltas: NORTH=0, EAST=1, SOUTH=2, WEST=3
const int FirstPersonRenderer::kDx[4] = {  0, 1, 0, -1 };
const int FirstPersonRenderer::kDy[4] = { -1, 0, 1,  0 };

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

void FirstPersonRenderer::draw3dWorld(Graphics::ManagedSurface *dst,
		uint8 partyDir, int partyX, int partyY,
		const Data::DaxBlockGeo &geo,
		const WalldefSlotCache &wallSlots) {
	// Convert C# direction (0/2/4/6) to C++ direction (0/1/2/3)
	const int dir   = partyDir / 2;
	const int dLeft  = dirLeft(dir);
	const int dRight = dirRight(dir);
	const int dBack  = (dir + 2) % 4;

	// Start two steps ahead, then step toward party
	int drawX = partyX + 2 * kDx[dir];
	int drawY = partyY + 2 * kDy[dir];

	for (int dist = 2; dist >= 0; --dist) {
		switch (dist) {
		case 2:
			drawFar(dst, dir, dLeft, dRight, drawX, drawY, geo, wallSlots);
			break;
		case 1:
			drawMid(dst, dir, dLeft, dRight, drawX, drawY, geo, wallSlots);
			break;
		case 0:
			drawNear(dst, dir, dLeft, dRight, drawX, drawY, geo, wallSlots);
			break;
		default:
			break;
		}
		drawX += kDx[dBack];
		drawY += kDy[dBack];
	}
}

// ---------------------------------------------------------------------------
// Distance passes  (direct ports of C# Draw3dWorldFar/Mid/Near)
// ---------------------------------------------------------------------------

void FirstPersonRenderer::drawFar(Graphics::ManagedSurface *dst,
		int dir, int dirLeft_, int dirRight_,
		int drawX, int drawY,
		const Data::DaxBlockGeo &geo,
		const WalldefSlotCache &wallSlots) {
	// Pass 1: sweep left from center — facing walls and left filler edges
	{
		int mapX = drawX, mapY = drawY;
		int num2 = 0;
		uint8 prevWall = 0;
		for (int i = 0; i < 4; ++i) {
			uint8 wt = wallTypeAt(dir, mapY, mapX, geo);

			// Out-of-bounds cell with no right-side wall resets filler
			if (!mapCoordIsValid(mapY, mapX) &&
					wallTypeAt(dirRight_, mapY, mapX, geo) == 0)
				prevWall = 0;

			if (wt != 0) {
				if (prevWall > 0)
					drawRegion(dst, Data::WalldefRegionId::FAR_FILLER,
							prevWall, 4, 5 + num2 + 1, wallSlots);
				prevWall = wt;
				drawRegion(dst, Data::WalldefRegionId::FAR_FORWARD,
						wt, 4, 5 + num2, wallSlots);
			} else {
				if (prevWall > 0 &&
						wallTypeAt(dirLeft_,
								mapY - kDy[dirLeft_],
								mapX - kDx[dirLeft_], geo) != 0)
					drawRegion(dst, Data::WalldefRegionId::FAR_FILLER,
							prevWall, 4, 5 + num2 + 1, wallSlots);
				prevWall = 0;
			}

			num2 -= 2;
			mapX += kDx[dirLeft_];
			mapY += kDy[dirLeft_];
		}
	}

	// Pass 2: sweep right from center — facing walls and right filler edges
	{
		int mapX = drawX, mapY = drawY;
		int num5 = 0;
		uint8 prevWall = 0;
		for (int i = 0; i < 4; ++i) {
			uint8 wt = wallTypeAt(dir, mapY, mapX, geo);

			if (!mapCoordIsValid(mapY, mapX) &&
					wallTypeAt(dirLeft_, mapY, mapX, geo) == 0)
				prevWall = 0;

			if (wt != 0) {
				if (prevWall > 0)
					drawRegion(dst, Data::WalldefRegionId::FAR_FILLER,
							prevWall, 4, 5 + num5 - 1, wallSlots);
				prevWall = wt;
				drawRegion(dst, Data::WalldefRegionId::FAR_FORWARD,
						wt, 4, 5 + num5, wallSlots);
			} else {
				if (prevWall > 0 &&
						wallTypeAt(dirRight_,
								mapY - kDy[dirRight_],
								mapX - kDx[dirRight_], geo) != 0)
					drawRegion(dst, Data::WalldefRegionId::FAR_FILLER,
							prevWall, 4, 5 + num5 - 1, wallSlots);
				prevWall = 0;
			}

			num5 += 2;
			mapX += kDx[dirRight_];
			mapY += kDy[dirRight_];
		}
	}

	// Pass 3: far left side walls
	{
		int mapX = drawX, mapY = drawY;
		int num8 = 0;
		for (int i = 0; i < 3; ++i) {
			uint8 wt = wallTypeAt(dirLeft_, mapY, mapX, geo);
			if (wt != 0) {
				if (i == 0)
					drawRegion(dst, Data::WalldefRegionId::FAR_LEFT,
							wt, 3, 4 + num8, wallSlots);
				else
					drawRegion(dst, Data::WalldefRegionId::FAR_LEFT,
							wt, 3, 4 + num8 - 1, wallSlots);
			}
			num8 -= 2;
			mapX += kDx[dirLeft_];
			mapY += kDy[dirLeft_];
		}
	}

	// Pass 4: far right side walls
	{
		int mapX = drawX, mapY = drawY;
		int num10 = 0;
		for (int i = 0; i < 3; ++i) {
			uint8 wt = wallTypeAt(dirRight_, mapY, mapX, geo);
			if (wt != 0) {
				if (i == 0)
					drawRegion(dst, Data::WalldefRegionId::FAR_RIGHT,
							wt, 3, 6 + num10, wallSlots);
				else
					drawRegion(dst, Data::WalldefRegionId::FAR_RIGHT,
							wt, 3, 6 + num10 + 1, wallSlots);
			}
			num10 += 2;
			mapX += kDx[dirRight_];
			mapY += kDy[dirRight_];
		}
	}
}

void FirstPersonRenderer::drawMid(Graphics::ManagedSurface *dst,
		int dir, int dirLeft_, int dirRight_,
		int drawX, int drawY,
		const Data::DaxBlockGeo &geo,
		const WalldefSlotCache &wallSlots) {
	// Left sweep: start 2 left-steps from draw point
	{
		int mapX = kDx[dirLeft_] + drawX + kDx[dirLeft_];
		int mapY = kDy[dirLeft_] + drawY + kDy[dirLeft_];
		int num2 = -6;
		for (int i = 0; i < 3; ++i) {
			uint8 wt1 = wallTypeAt(dir, mapY, mapX, geo);
			if (wt1 != 0)
				drawRegion(dst, Data::WalldefRegionId::MED_FORWARD,
						wt1, 3, 4 + num2, wallSlots);
			uint8 wt2 = wallTypeAt(dirLeft_, mapY, mapX, geo);
			if (wt2 != 0)
				drawRegion(dst, Data::WalldefRegionId::MED_LEFT,
						wt2, 1, 2 + num2, wallSlots);
			num2 += 3;
			mapX += kDx[dirRight_];
			mapY += kDy[dirRight_];
		}
	}

	// Right sweep: start 2 right-steps from draw point
	{
		int mapX = kDx[dirRight_] + kDx[dirRight_] + drawX;
		int mapY = kDy[dirRight_] + kDy[dirRight_] + drawY;
		int num4 = 6;
		for (int i = 0; i < 3; ++i) {
			uint8 wt3 = wallTypeAt(dir, mapY, mapX, geo);
			if (wt3 != 0)
				drawRegion(dst, Data::WalldefRegionId::MED_FORWARD,
						wt3, 3, 4 + num4, wallSlots);
			uint8 wt4 = wallTypeAt(dirRight_, mapY, mapX, geo);
			if (wt4 != 0)
				drawRegion(dst, Data::WalldefRegionId::MED_RIGHT,
						wt4, 1, 7 + num4, wallSlots);
			num4 -= 3;
			mapX += kDx[dirLeft_];
			mapY += kDy[dirLeft_];
		}
	}
}

void FirstPersonRenderer::drawNear(Graphics::ManagedSurface *dst,
		int dir, int dirLeft_, int dirRight_,
		int drawX, int drawY,
		const Data::DaxBlockGeo &geo,
		const WalldefSlotCache &wallSlots) {
	// Left side: start 1 left-step from draw point
	{
		int mapX = kDx[dirLeft_] + drawX;
		int mapY = kDy[dirLeft_] + drawY;
		int colStart = -7;
		for (int i = 0; i < 2; ++i) {
			uint8 wt1 = wallTypeAt(dir, mapY, mapX, geo);
			if (wt1 != 0)
				drawRegion(dst, Data::WalldefRegionId::CLOSE_FORWARD,
						wt1, 1, 2 + colStart, wallSlots);
			uint8 wt2 = wallTypeAt(dirLeft_, mapY, mapX, geo);
			if (wt2 != 0)
				drawRegion(dst, Data::WalldefRegionId::CLOSE_LEFT,
						wt2, 0, colStart, wallSlots);
			colStart += 7;
			mapX += kDx[dirRight_];
			mapY += kDy[dirRight_];
		}
	}

	// Right side: start 1 right-step from draw point
	{
		int mapX = drawX + kDx[dirRight_];
		int mapY = drawY + kDy[dirRight_];
		int num3 = 7;
		for (int i = 0; i < 2; ++i) {
			uint8 wt3 = wallTypeAt(dir, mapY, mapX, geo);
			if (wt3 != 0)
				drawRegion(dst, Data::WalldefRegionId::CLOSE_FORWARD,
						wt3, 1, num3 + 2, wallSlots);
			uint8 wt4 = wallTypeAt(dirRight_, mapY, mapX, geo);
			if (wt4 != 0)
				drawRegion(dst, Data::WalldefRegionId::CLOSE_RIGHT,
						wt4, 0, num3 + 9, wallSlots);
			num3 -= 7;
			mapX += kDx[dirLeft_];
			mapY += kDy[dirLeft_];
		}
	}
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void FirstPersonRenderer::drawRegion(Graphics::ManagedSurface *dst,
		Data::WalldefRegionId regionId, uint8 wallType,
		int rowStart, int colStart,
		const WalldefSlotCache &wallSlots) {
	const WallSurfaceSet *wss = wallSlots.surfaceSetForWallType(wallType);
	if (!wss)
		return;
	const Pic *pic = wss->region(regionId);
	if (!pic)
		return;

	const int dstX = (colStart + k3dViewOffsetX) * kCharW;
	const int dstY = (rowStart + k3dViewOffsetY) * kCharH;
	blitClipped(*pic, dst, dstX, dstY);
}

void FirstPersonRenderer::blitClipped(const Pic &src,
		Graphics::ManagedSurface *dst, int dstX, int dstY) {
	// Compute the source sub-rect that falls within dst bounds
	int srcX0 = 0, srcY0 = 0;
	int blitW  = src.w;
	int blitH  = src.h;

	// Clip left/top
	if (dstX < 0) { srcX0 -= dstX; blitW += dstX; dstX = 0; }
	if (dstY < 0) { srcY0 -= dstY; blitH += dstY; dstY = 0; }

	// Clip right/bottom
	if (dstX + blitW > dst->w) blitW = dst->w - dstX;
	if (dstY + blitH > dst->h) blitH = dst->h - dstY;

	if (blitW <= 0 || blitH <= 0)
		return;

	for (int py = 0; py < blitH; ++py) {
		const byte *srcRow =
				(const byte *)src.getBasePtr(srcX0, srcY0 + py);
		byte *dstRow =
				(byte *)dst->getBasePtr(dstX, dstY + py);
		for (int px = 0; px < blitW; ++px) {
			const byte pixel = srcRow[px];
			if (pixel != 0)
				dstRow[px] = pixel;
		}
	}
}

uint8 FirstPersonRenderer::wallTypeAt(int dir, int y, int x,
		const Data::DaxBlockGeo &geo) {
	// Wrap coordinates for maps that tile seamlessly
	x = (x % kMapSize + kMapSize) % kMapSize;
	y = (y % kMapSize + kMapSize) % kMapSize;
	return geo.getWallType(y, x,
			static_cast<Data::DaxBlockGeo::Direction>(dir));
}

bool FirstPersonRenderer::mapCoordIsValid(int y, int x) {
	return x >= 0 && x < kMapSize && y >= 0 && y < kMapSize;
}

} // namespace Gfx
} // namespace Goldbox
