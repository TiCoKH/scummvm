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

#ifndef GOLDBOX_POOLRAD_WILDERNESS_MAP_H
#define GOLDBOX_POOLRAD_WILDERNESS_MAP_H

#include "common/scummsys.h"

namespace Goldbox {
namespace Poolrad {

/**
 * GB13-024 - pure wilderness drawing geometry, site-override resolver and the
 * one-time YarashComplete world-state operation (Requirement 39).
 *
 * This module owns the CONFIRMABLE STRUCTURE only; the uncertain DOS-reference
 * DATA stays documented OPEN and is NOT fabricated:
 *   - the 44x36 terrain grid (behind WildernessAtlas, returns 0 until filled);
 *   - the nine site override ids and their exact quest conditions;
 *   - the mission==254 / yarashPyramidLeft VM fields and the river-open bit.
 *
 * Everything here is pure (no g_engine, no ManagedSurface, no RNG, no VM
 * writes): the view/engine feed grid reads, quest predicates and the
 * persistent Yarash flag through small seams so the geometry, the
 * subtract-one-once id->tile-slot rule, the clamp, the nine-case resolver, the
 * idempotent YarashComplete and the viewport/marker independence are all unit
 * testable deterministically.
 */
class WildernessMap {
public:
	/** 5x5 canonical map window (MAP_DrawAreaMapBlock). */
	static const int kWindowCells = 5;

	/** Each cell is a 24x24 tile == three 8-pixel character cells. */
	static const int kCellPixels = 24;
	static const int kCharCell = 8;
	static const int kCharCellsPerTile = 3; // 24 / 8

	/** Map-window clamp bounds for the viewport origin (x 0..39, y 0..31). */
	static const int kMapWindowXMin = 0;
	static const int kMapWindowXMax = 39;
	static const int kMapWindowYMin = 0;
	static const int kMapWindowYMax = 31;

	/** 44x36 atlas dimensions (mirror WildernessAtlas / combat constants). */
	static const int kAtlasCols = 44;
	static const int kAtlasRows = 36;

	/** Sentinel for an empty/baseline cell (one-based id 0). */
	static const uint8 kEmptyId = 0;

	/** Tile slot that means "no tile" (baseline/empty cell). */
	static const int kNoTileSlot = -1;

	// --------------------------------------------------------------------
	// Draw geometry (Req 39.1)
	// --------------------------------------------------------------------

	/**
	 * Clamp a prospective 5x5 map-window origin to x 0..39 / y 0..31. Returns
	 * true if either axis was clamped. The window origin and the marker cell
	 * are computed INDEPENDENTLY, so this clamp never shifts the marker.
	 */
	static bool clampViewportOrigin(int &originX, int &originY);

	/**
	 * Resolve a ONE-BASED atlas/override id to a decoded tile slot, SUBTRACTING
	 * ONE exactly once here. id 0 -> kNoTileSlot (baseline/empty, NOT slot -1
	 * used as a real index); id N>=1 -> slot N-1. The platform blitter MUST NOT
	 * subtract again.
	 */
	static int idToTileSlot(uint8 oneBasedId);

	/**
	 * Read-only grid-read seam, so the geometry can be tested against a fake
	 * grid without WildernessAtlas. Returns the one-based id at an atlas cell
	 * (0 == empty/out of range).
	 */
	class GridReader {
	public:
		virtual ~GridReader() {}
		virtual uint8 idAt(int atlasX, int atlasY) = 0;
	};

	/** Default GridReader backed by WildernessAtlas (the shared seam). */
	class AtlasGridReader : public GridReader {
	public:
		uint8 idAt(int atlasX, int atlasY) override;
	};

	/**
	 * One resolved window cell: its atlas coordinates, the final one-based id
	 * (after the site-override resolver) and the subtract-one-once tile slot.
	 */
	struct WindowCell {
		int atlasX;
		int atlasY;
		uint8 baseId;   // grid id before override
		uint8 finalId;  // id after the site resolver (one-based)
		int tileSlot;   // finalId - 1, or kNoTileSlot when finalId == 0
		bool inRange;   // atlas cell is within 0..43 / 0..35

		WindowCell()
			: atlasX(0), atlasY(0), baseId(0), finalId(0),
			  tileSlot(kNoTileSlot), inRange(false) {}
	};

	/** Fully-resolved 5x5 window. */
	struct Window {
		WindowCell cells[kWindowCells][kWindowCells];

		Window() {}

		const WindowCell &at(int row, int col) const {
			return cells[row][col];
		}
	};

	// --------------------------------------------------------------------
	// Site-override resolver (Req 39.2)
	// --------------------------------------------------------------------

	/**
	 * Quest-predicate seam for the nine indexed site cases. Each case reads one
	 * world-state condition; the resolver substitutes the override tile ONLY
	 * when the condition holds. The exact quest flags are OPEN, so the resolver
	 * gates through this predicate rather than a guessed VM address.
	 */
	class QuestPredicate {
	public:
		virtual ~QuestPredicate() {}

		/**
		 * True when the quest condition for the indexed site @p siteIndex
		 * holds. siteIndex is 0..kSiteCount-1. Cases 1 and 2 (indices that map
		 * to them) never substitute, so the resolver never calls this for
		 * them.
		 */
		virtual bool questConditionMet(int siteIndex) = 0;
	};

	/** Number of indexed site cases in Wilds.BuildWildernessMap. */
	static const int kSiteCount = 9;

	/**
	 * One entry of the read-only nine-case site decision table. Keyed by the
	 * atlas base id that triggers the case; when hasReplacement is false (the
	 * cases 1 and 2 shape: an indexed site with NO explicit replacement
	 * branch) the resolver leaves the baseline tile unchanged. When
	 * hasReplacement is true the override id is substituted ONLY if the quest
	 * condition for siteIndex holds.
	 *
	 * OPEN (trace-gated): triggerId / overrideId are DOS-reference data that is
	 * NOT confirmed, so all entries ship with triggerId 0 and hasReplacement
	 * false (a resolver that changes nothing). The table SHAPE - nine indexed
	 * cases, each checked before and after its condition, cases 1/2 with no
	 * replacement - is the confirmable structure; the ids/conditions drop in
	 * once traced.
	 */
	//
	// NOTE: SiteCase is an AGGREGATE on purpose (no user-declared constructor)
	// so the static const nine-case table can be brace-initialized with no
	// runtime global constructor. All four fields are always provided by the
	// table, so there is no uninitialized-field risk.
	struct SiteCase {
		int siteIndex;        // 0..8
		uint8 triggerId;      // one-based atlas id this case matches (0 = none)
		uint8 overrideId;     // one-based override tile id (0 = none)
		bool hasReplacement;  // false for the cases-1-and-2 shape
	};

	/**
	 * READ-ONLY site-override resolver. Given a one-based atlas base id and a
	 * quest predicate, returns the id to draw: the override id when the id
	 * matches an indexed case WITH a replacement branch AND its quest condition
	 * holds; otherwise the unchanged base id. An atlas id with no case stays
	 * baseline; cases 1 and 2 (no replacement) never substitute. Mutates
	 * nothing (no movement/quests/RNG).
	 */
	static uint8 resolveSiteOverride(uint8 baseId, QuestPredicate &quest);

	/** Access the read-only nine-case table (for tests / diagnostics). */
	static const SiteCase *siteTable();

	// --------------------------------------------------------------------
	// Window builder (Req 39.1 + 39.2 combined)
	// --------------------------------------------------------------------

	/**
	 * Build the resolved 5x5 window for a given (already region-offset) atlas
	 * origin. The window origin is clamped to x 0..39 / y 0..31 FIRST, then
	 * each of the 25 cells is read from @p grid, run through the site resolver
	 * (@p quest) and the subtract-one-once tile-slot rule. Out-of-range atlas
	 * cells become baseline/empty (id 0, kNoTileSlot). Pure: no draw, no
	 * mutation.
	 */
	static Window buildWindow(int originAtlasX, int originAtlasY,
			GridReader &grid, QuestPredicate &quest);

	// --------------------------------------------------------------------
	// Marker (Req 39.4)
	// --------------------------------------------------------------------

	/** Marker cell within the 5x5 window (col,row) plus validity. */
	struct MarkerCell {
		int col;      // 0..4 within the window
		int row;      // 0..4 within the window
		bool visible; // true when the party cell lies inside the window

		MarkerCell() : col(0), row(0), visible(false) {}
	};

	/**
	 * Compute the marker cell INDEPENDENTLY of the viewport-origin clamp: given
	 * the UNCLAMPED prospective origin and the party's atlas cell, the marker
	 * is party - clampedOrigin, where the origin is clamped exactly as
	 * clampViewportOrigin() does. Because the origin is clamped but the party
	 * cell is not, the marker drifts toward the window edge at map borders
	 * instead of staying pinned to the centre. Pure: no RNG/quest mutation.
	 */
	static MarkerCell markerCell(int prospectiveOriginX, int prospectiveOriginY,
			int partyAtlasX, int partyAtlasY);

	// --------------------------------------------------------------------
	// YarashComplete one-time world-state op (Req 39.4)
	// --------------------------------------------------------------------

	/**
	 * Persistent world-state seam for the Yarash gate. These accessors read and
	 * write the PERSISTENT world-state (save/load survivable), NOT a transient
	 * view flag, so the op stays idempotent across redraw and service/battle
	 * return.
	 *
	 * OPEN (trace-gated): the mission byte, the yarashPyramidLeft byte and the
	 * river-open bit have no confirmed VM addresses. The host binds these to
	 * the confirmable persistent world-state source; tests bind an in-memory
	 * fake. The ONCE-ONLY structure is driven entirely through these
	 * well-named predicates, never a guessed address.
	 */
	class WorldState {
	public:
		virtual ~WorldState() {}

		/** The mission byte (central-region gate wants 254). OPEN address. */
		virtual uint8 mission() = 0;

		/**
		 * The persistent "pyramid left" / river-open flag. false == closed
		 * (rivers blocked); true == open. This is the SAME flag GB13-023 gates
		 * rivers on, so YarashComplete OPENING it opens the river. OPEN address.
		 */
		virtual bool yarashPyramidLeft() = 0;

		/** Flip the persistent pyramid-left/river-open flag. */
		virtual void setYarashPyramidLeft(bool open) = 0;
	};

	/** Mission value that marks the central-region YarashComplete gate. */
	static const uint8 kMissionYarashComplete = 254;

	/**
	 * True when the central-region YarashComplete gate condition holds:
	 * mission == 254 AND the pyramid-left flag is still 0 (closed). Read-only.
	 */
	static bool shouldRunYarashComplete(WorldState &world);

	/**
	 * The one-time YarashComplete world change: when the gate holds (mission
	 * 254 AND pyramid-left 0) it OPENS the river by setting the persistent
	 * pyramid-left flag, then returns true (ran). When the gate does not hold
	 * (wrong mission, or already open) it does NOTHING and returns false. Being
	 * guarded by the persistent flag makes it IDEMPOTENT: a second call after
	 * a redraw / save-load re-entry is a no-op. Mutates only the persistent
	 * world-state flag - no movement, quests or RNG.
	 */
	static bool runYarashComplete(WorldState &world);
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_WILDERNESS_MAP_H
