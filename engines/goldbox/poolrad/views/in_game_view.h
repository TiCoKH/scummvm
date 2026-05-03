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

#ifndef GOLDBOX_POOLRAD_VIEWS_IN_GAME_VIEW_H
#define GOLDBOX_POOLRAD_VIEWS_IN_GAME_VIEW_H

#include "goldbox/poolrad/views/view.h"
#include "goldbox/core/global.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

/**
 * Persistent aggregated in-game view covering all map-runtime states:
 * dungeon, wilderness, shop, camping, after-combat, and combat.
 *
 * Replaces per-state top-level view churn with a single long-lived View
 * whose internal mode switches when GameState changes.  This matches the
 * original engine's single continuous runtime surface with state-dependent
 * drawing (see VM_MAIN_TODO_PICKUP_GUIDE.md § "single InGameView aggregator").
 *
 * Architecture guardrails:
 *  - EclVM is owned by PoolradEngine/orchestrator, NOT by this view.
 *  - GameState remains authoritative; view only adjusts presentation.
 *  - Mode switch is idempotent: re-entering the same mode must not reload
 *    assets or reset transient UI unnecessarily.
 *  - Script dispatch (ONMOVE/ONSEARCH/ONREST) stays in the orchestrator.
 *
 * Layout (40×25 character grid):
 *  - Cols  0-15, rows  0-15: 3D viewport / left panel
 *  - Cols  3-13, rows  3-13: Inner mini-window (dungeon/camping/shop)
 *  - Cols 17-39, rows  1-14: Party summary (Name / AC / HP)
 *  - Cols 17-39, row  15:    Position / direction / status
 *  - Cols  0-39, rows 17-22: Prompt / text area
 *
 * Direction convention (8-direction, only even values used):
 *   0=N, 2=E, 4=S, 6=W
 */
class InGameView : public View {
public:
	/** Internal mode, parallel to GameState but view-scoped. */
	enum InGameMode {
		kModeNone,
		kModeDungeon,
		kModeWilderness,
		kModeShop,
		kModeCamping,
		kModeAfterCombat,
		kModeCombat
	};

private:
	InGameMode _mode = kModeNone;

	// --- Dungeon navigation state ---
	/** Party map X position (column, 0-15). */
	uint16 _mapX = 8;
	/** Party map Y position (row, 0-15). */
	uint16 _mapY = 8;
	/** Party facing direction: 0=N, 2=E, 4=S, 6=W (8-dir wire format). */
	uint8 _mapDir = 0;
	/** Search mode active flag. */
	bool _searchMode = false;

	// --- Mode-specific drawing ---
	void drawDungeonMode();
	void drawWildernessMode();
	void drawShopMode();
	void drawCampingMode();
	void drawAfterCombatMode();

	// --- Dungeon / party-panel helpers ---
	/** Draw party names, AC, and HP into the right panel. */
	void drawPartySummary(Surface &s);
	/** Draw position / direction string at the status row. */
	void drawPositionTime(Surface &s);
	/** Attempt to step one tile forward in the current facing direction. */
	void stepForward();

	// --- Mode-specific input ---
	bool handleDungeonKeypress(const KeypressMessage &msg);

public:
	InGameView();
	~InGameView() override = default;

	/**
	 * Apply screen layout and internal mode for the given GameState.
	 * Called from onEnter() and may be re-called on state refresh.
	 * Safe to call when already in the matching mode (idempotent).
	 */
	void applyScreenByState(GameState state);

	bool msgFocus(const FocusMessage &msg) override;
	bool msgUnfocus(const UnfocusMessage &msg) override;
	bool msgKeypress(const KeypressMessage &msg) override;
	void draw() override;
	void onEnter(GameState state) override;
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_IN_GAME_VIEW_H
