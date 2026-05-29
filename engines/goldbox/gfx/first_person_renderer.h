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

#ifndef GOLDBOX_GFX_FIRST_PERSON_RENDERER_H
#define GOLDBOX_GFX_FIRST_PERSON_RENDERER_H

#include "common/scummsys.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/gfx/walldef_surface_builder.h"

namespace Graphics {
class ManagedSurface;
}

namespace Goldbox {
namespace Gfx {

/**
 * Renders the 3D first-person dungeon view.
 *
 * Mirrors C# ovr031.Draw3dWorld / Draw3dWorldFar / Draw3dWorldMid /
 * Draw3dWorldNear and draw_3D_8x8_titles.
 *
 * Direction convention (same as DaxBlockGeo::Direction):
 *   NORTH=0, EAST=1, SOUTH=2, WEST=3
 *
 * The caller passes partyDir in the C# wire format (0/2/4/6 = N/E/S/W).
 * Internally it is divided by 2 to get the 0..3 C++ direction index.
 *
 * The 3D viewport occupies character-grid columns 3..13 and rows 3..13 on
 * the 40×25 screen (char cell size 8×8 pixels, matching ovr031 index+3 offset).
 */
class FirstPersonRenderer {
public:
	/** 3D viewport offset within the character grid (C# index2+2 / index3+2). */
	static const int k3dViewOffsetX = 3;
	static const int k3dViewOffsetY = 3;

	/** Pixels per character cell (8×8). */
	static const int kCharW = 8;
	static const int kCharH = 8;

	/**
	 * Render the 3D dungeon view onto dst.
	 *
	 * @param dst       Target surface (full screen or viewport)
	 * @param partyDir  Party facing direction: 0=N, 2=E, 4=S, 6=W (C# wire format)
	 * @param partyX    Party map X position (column)
	 * @param partyY    Party map Y position (row)
	 * @param geo       Current dungeon GEO block
	 * @param wallSlots Pre-loaded walldef slot cache
	 */
	static void draw3dWorld(Graphics::ManagedSurface *dst,
			uint8 partyDir, int partyX, int partyY,
			const Data::DaxBlockGeo &geo,
			const WalldefSlotCache &wallSlots);

private:
	/** GEO map size (16×16). Mirrors ovr031.MapSize. */
	static const int kMapSize = 16;

	/** Movement deltas indexed by C++ direction (0=N, 1=E, 2=S, 3=W). */
	static const int kDx[4]; // {0, 1, 0, -1}
	static const int kDy[4]; // {-1, 0, 1, 0}

	/** Mirrors Draw3dWorldFar: draws walls at distance 2 ahead of party. */
	static void drawFar(Graphics::ManagedSurface *dst,
			int dir, int dirLeft, int dirRight,
			int drawX, int drawY,
			const Data::DaxBlockGeo &geo,
			const WalldefSlotCache &wallSlots);

	/** Mirrors Draw3dWorldMid: draws walls at distance 1 ahead of party. */
	static void drawMid(Graphics::ManagedSurface *dst,
			int dir, int dirLeft, int dirRight,
			int drawX, int drawY,
			const Data::DaxBlockGeo &geo,
			const WalldefSlotCache &wallSlots);

	/** Mirrors Draw3dWorldNear: draws walls at party position. */
	static void drawNear(Graphics::ManagedSurface *dst,
			int dir, int dirLeft, int dirRight,
			int drawX, int drawY,
			const Data::DaxBlockGeo &geo,
			const WalldefSlotCache &wallSlots);

	/**
	 * Blit the walldef region for wallType at char-grid position
	 * (colStart, rowStart) within the 3D viewport.
	 * Mirrors C# draw_3D_8x8_titles(offsetIndex, wallType, rowStart, colStart).
	 * Applies the k3dViewOffset and clips to dst bounds before blitting.
	 */
	static void drawRegion(Graphics::ManagedSurface *dst,
			Data::WalldefRegionId regionId, uint8 wallType,
			int rowStart, int colStart,
			const WalldefSlotCache &wallSlots);

	/**
	 * Blit src onto dst at (dstX, dstY), skipping transparent-color pixels
	 * (index 0). Clips the blit to the viewport rectangle
	 * [clipX0,clipY0)..[clipX1,clipY1) and to dst surface bounds.
	 */
	static void blitClipped(const Gfx::Pic &src, Graphics::ManagedSurface *dst,
			int dstX, int dstY,
			int clipX0, int clipY0, int clipX1, int clipY1);

	/** Return wall type at map cell (y, x) looking in direction dir. Wraps map. */
	static uint8 wallTypeAt(int dir, int y, int x, const Data::DaxBlockGeo &geo);

	/** Return true if (y, x) is within the 16×16 GEO map bounds. */
	static bool mapCoordIsValid(int y, int x);

	static int dirLeft(int d)  { return (d + 3) % 4; }
	static int dirRight(int d) { return (d + 1) % 4; }
};

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_FIRST_PERSON_RENDERER_H
