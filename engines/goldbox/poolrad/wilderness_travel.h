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
 * Assisted-by: Claude:claude-opus-4.6
 */

#ifndef GOLDBOX_POOLRAD_WILDERNESS_TRAVEL_H
#define GOLDBOX_POOLRAD_WILDERNESS_TRAVEL_H

#include "common/scummsys.h"
#include "goldbox/core/direction.h"

namespace Goldbox {
namespace Poolrad {

/**
 * Pure wilderness-travel rules (GB13-023, Requirement 33).
 *
 * The overworld move path is a DIRECTIONAL-TRAVEL path distinct from the
 * dungeon turn convention: an arrow key is a compass MOVE (UP=north,
 * DOWN=south, LEFT=west, RIGHT=east), not a facing rotation. These rules own
 * the confirmable DOS-reference behaviour so they can be unit tested without
 * g_engine; the engine side effects (reading the atlas terrain id, advancing
 * the clock, running script transitions) are injected through Environment.
 *
 * Confirmable DOS-reference rules implemented here:
 *   - independent party coordinates: local x clamped 0..15, y 0..35;
 *   - region offset 0/13/26 (west/central/east) into the 44x36 atlas, with
 *     EXPLICIT SIGNED pre-clamp math and a bounds check BEFORE any lookup so
 *     the atlas is never indexed out of range;
 *   - a fixed blocked-terrain id set and a river id set gated on
 *     yarashPyramidLeft;
 *   - a PERMITTED move advances 12 HOURS even when clamping leaves the
 *     coordinates unchanged; a BLOCKED move advances no time and does not move.
 *
 * Terrain ids, the river set and the 12-hour cost are DOS-reference data (see
 * the constant tables in wilderness_travel.cpp).
 */
class WildernessTravel {
public:
	/** Local coordinate clamp ranges (independent party coordinates). */
	static const int kLocalXMin = 0;
	static const int kLocalXMax = 15;
	static const int kLocalYMin = 0;
	static const int kLocalYMax = 35;

	/** 44x36 wilderness atlas dimensions (mirrors the combat constants). */
	static const int kAtlasCols = 44;
	static const int kAtlasRows = 36;

	/** Region atlas offsets (west/central/east), == LocationContext.region. */
	static const uint8 kRegionWest = 0;
	static const uint8 kRegionCentral = 13;
	static const uint8 kRegionEast = 26;

	/** Hours a permitted overworld step costs (DOS-reference timing). */
	static const uint8 kHoursPerMove = 12;

	/** Compass travel direction. Arrow keys map onto these at one boundary. */
	enum Compass {
		TRAVEL_NORTH = 0,
		TRAVEL_SOUTH,
		TRAVEL_WEST,
		TRAVEL_EAST
	};

	/** Arrow keycode class, decoupled from Common::KeyCode for the pure test. */
	enum Arrow {
		ARROW_UP = 0,
		ARROW_DOWN,
		ARROW_LEFT,
		ARROW_RIGHT
	};

	/**
	 * Map an arrow key to a compass travel direction (the ONE conversion
	 * boundary). UP=north, DOWN=south, LEFT=west, RIGHT=east. This is
	 * directional travel, NOT a dungeon turn.
	 */
	static Compass arrowToCompass(Arrow arrow);

	/** Per-axis signed step for a compass direction (y: north = -1). */
	static void compassStep(Compass dir, int &dx, int &dy);

	/**
	 * True if @p terrainId is in the fixed blocked-terrain set (DOS-reference
	 * data). River ids are NOT part of this set; they are gated separately by
	 * isRiverGatedOpen().
	 */
	static bool isBlockedTerrain(uint8 terrainId);

	/** True if @p terrainId is one of the gated river ids (DOS-reference). */
	static bool isRiverTerrain(uint8 terrainId);

	/**
	 * A river tile is passable ONLY when the Yarash "pyramid left" flag gates
	 * it open. @p yarashPyramidLeft is the confirmable world-state predicate;
	 * its exact VM address is documented OPEN (no named VM field exists yet).
	 */
	static bool isRiverGatedOpen(uint8 terrainId, bool yarashPyramidLeft);

	/**
	 * Overall passability of a destination terrain id under the river gate:
	 * blocked terrain is never passable; a river is passable only when gated
	 * open; anything else is passable.
	 */
	static bool isPassable(uint8 terrainId, bool yarashPyramidLeft);

	/**
	 * Clamp a signed local coordinate pair into the 0..15 / 0..35 ranges.
	 * Returns true if either axis was clamped (edge/leave condition).
	 */
	static bool clampLocal(int &localX, int &localY);

	/**
	 * Compute the bounds-validated atlas cell for a clamped local (x,y) and a
	 * region offset. atlasX = clampedLocalX + region, atlasY = clampedLocalY.
	 * Returns true and fills @p atlasX / @p atlasY when the cell is in range;
	 * returns false (and leaves the outputs untouched) when it would fall
	 * outside the 44x36 atlas, so the caller NEVER indexes out of range.
	 *
	 * @p localX must already be in 0..15 and @p localY in 0..35 (post-clamp);
	 * region must be 0/13/26.
	 */
	static bool atlasCell(int clampedLocalX, int clampedLocalY, uint8 region,
			int &atlasX, int &atlasY);

	/**
	 * Injected engine side effects, kept minimal so the rules stay pure.
	 * The host wiring forwards to the real engine (atlas read, timeAddUnits,
	 * VM coordinate write, script transitions); tests supply recording fakes.
	 */
	class Environment {
	public:
		virtual ~Environment() {}

		// Current local wilderness coordinates (mirrors kVmFieldWildernessX/Y
		// and LocationContext.wildernessPos).
		virtual void currentLocal(int &localX, int &localY) = 0;

		// Atlas offset for the active region (0/13/26 from
		// LocationContext.region).
		virtual uint8 regionOffset() = 0;

		// The Yarash "pyramid-left" world-state flag that gates river tiles.
		// OPEN: exact VM address not confirmed; host feeds the confirmable
		// source, tests set it directly.
		virtual bool yarashPyramidLeft() = 0;

		// Read the terrain id at a bounds-validated atlas cell (WildCom /
		// AreaMapCache). Called only for an in-range cell.
		virtual uint8 readAtlasTerrain(int atlasX, int atlasY) = 0;

		// Commit the permitted clamped destination to VM memory and
		// LocationContext (write kVmFieldWildernessX/Y, wildernessPos).
		virtual void commitLocal(int localX, int localY) = 0;

		// Advance the game clock by 12 hours (timeAddUnits field=3, amount=12)
		// with timed-effect expiry. Called only on a permitted move.
		virtual void advanceTwelveHours() = 0;

		// Run the overworld script transitions in the original order
		// (ONMOVE/clock/ONSEARCH sequencing). The exact wilderness-specific
		// order is documented OPEN; a permitted move runs this after the
		// commit + time advance.
		virtual void runScriptTransitions() = 0;
	};

	/** Outcome of attempting one overworld step. */
	struct MoveResult {
		bool permitted;   // destination passable; move + time committed
		bool clamped;     // destination clamped at a border (edge/no-op step)
		bool moved;       // local coordinates actually changed
		int destX;        // resolved (clamped) destination local x
		int destY;        // resolved (clamped) destination local y
		uint8 terrainId;  // terrain id read at the destination atlas cell

		MoveResult()
			: permitted(false), clamped(false), moved(false),
			  destX(0), destY(0), terrainId(0) {}
	};

	WildernessTravel() {}

	/**
	 * Attempt one directional-travel step in @p dir. Computes the prospective
	 * destination with signed arithmetic, clamps to the local ranges, selects
	 * the bounds-validated atlas cell (region offset), reads the destination
	 * terrain id and decides passability (blocked set + river gate). On a
	 * PERMITTED move it commits the clamped destination, advances 12 hours
	 * (EVEN IF the clamp left the coordinates unchanged) and runs the script
	 * transitions; on a BLOCKED move nothing is committed and no time passes.
	 */
	MoveResult step(Environment &env, Compass dir);
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_WILDERNESS_TRAVEL_H
