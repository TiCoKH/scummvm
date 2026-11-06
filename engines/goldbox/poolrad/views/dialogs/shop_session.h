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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_SHOP_SESSION_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_SHOP_SESSION_H

#include "common/scummsys.h"
#include "goldbox/ecl/pending_operation.h"
#include "goldbox/poolrad/location_context.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * GB13-025 — explicit shop/temple input and session ownership (Requirement 29).
 *
 * This is the PURE, view-independent session-ownership model shared by
 * Store/Temple/Treasure. It does not touch g_engine, the treasure pool or any
 * view; ShopBaseDialog wires it to those. Keeping it pure lets the
 * once-on-entry reset, single-route dispatch and stable-id child tracking be
 * unit-tested deterministically (test/engines/goldbox/shop_session.h).
 *
 * The three explicit lifecycle stages the finding asks for:
 *   beginSession() — GENUINE entry (ST_ENTER_SHOP). Runs the once-on-entry
 *                    work: capture the caller LocationContext + selected
 *                    character, set the service interface mode, mint a fresh
 *                    session, and mark that the entry reset (pool/coin/session
 *                    flags) is OWED exactly once. Clears any stale child /
 *                    pending service / payment / appraised-item IDs.
 *   resumeChild()  — REFOCUS / return from a pushed child (ViewCharacter or a
 *                    nested selection). Does NOT re-run the entry reset; it
 *                    only rebuilds the menu/redraw and resumes the OWNING
 *                    stage. Pooled money survives a View round-trip.
 *   endSession()   — exit / cancel / quit. Deactivates+resets every child and
 *                    transaction stage, completes the host request ONCE and
 *                    restores the captured caller LocationContext.
 */
class ShopSession {
public:
	/** Service interface mode set at actual entry. */
	enum ServiceKind : uint8 {
		KIND_STORE = 0,
		KIND_TEMPLE,
		KIND_TREASURE
	};

	/**
	 * Main-menu selections. There is exactly ONE dispatch route (the direct
	 * route in ShopBaseDialog::msgKeypress); this enum lets the session record
	 * which action a selection resolved to so a second interpretation of a
	 * queued horizontal-menu result would be observable (and is proven absent).
	 */
	enum MenuAction : uint8 {
		ACT_NONE = 0,
		ACT_PRIMARY,   // Buy (store) / Heal (temple)
		ACT_VIEW,
		ACT_TAKE,
		ACT_POOL,
		ACT_SHARE,
		ACT_APPRAISE,
		ACT_DETECT,
		ACT_EXIT
	};

	/** Which owning stage a child was pushed from, so we resume it (not rebuild). */
	enum OwningStage : uint8 {
		OWN_NONE = 0,
		OWN_MENU,
		OWN_TAKE,
		OWN_APPRAISE,
		OWN_HEAL
	};

	ShopSession()
		: _kind(KIND_STORE), _active(false), _entryResetOwed(false),
		  _savedCharId(0), _dispatchCount(0), _lastAction(ACT_NONE),
		  _owningStage(OWN_NONE), _callerLocation(), _activeChild(),
		  _pendingService(), _paymentTarget(), _appraisedItem(),
		  _hostRequestCompleted(false) {}

	// --- Lifecycle -----------------------------------------------------

	/**
	 * GENUINE entry. Returns the number of entry resets owed (always 1 per
	 * genuine entry) so the caller performs the pool/coin/session-flag reset
	 * exactly once. A different/again shop mints a brand-new session and owes
	 * the reset again.
	 */
	void beginSession(ServiceKind kind, const LocationContext &caller,
			uint16 selectedCharId) {
		_kind = kind;
		_active = true;
		_callerLocation = caller;
		_savedCharId = selectedCharId;
		_entryResetOwed = true;       // reset is owed ONCE on genuine entry
		_hostRequestCompleted = false;

		// A genuinely new session carries nothing over from a prior shop.
		_dispatchCount = 0;
		_lastAction = ACT_NONE;
		_owningStage = OWN_MENU;
		_activeChild = Goldbox::ECL::RequestKey();
		_pendingService = Goldbox::ECL::RequestKey();
		_paymentTarget = Goldbox::ECL::RequestKey();
		_appraisedItem = Goldbox::ECL::RequestKey();
	}

	/**
	 * Consume the once-on-entry reset. Returns true exactly once per genuine
	 * entry; subsequent calls (and any resumeChild path) return false so the
	 * pool/coin/session reset never re-runs on refocus.
	 */
	bool consumeEntryReset() {
		if (!_entryResetOwed)
			return false;
		_entryResetOwed = false;
		return true;
	}

	/** True while a genuine session is active (not torn down). */
	bool isActive() const { return _active; }

	/** True while the once-on-entry reset is still pending. */
	bool entryResetOwed() const { return _entryResetOwed; }

	/**
	 * REFOCUS / return-from-child. Never resets pool/session flags; only
	 * restores the owning stage so the owner resumes instead of rebuilding a
	 * fresh transaction. Returns the stage to resume.
	 */
	OwningStage resumeChild() {
		_activeChild = Goldbox::ECL::RequestKey();
		return _owningStage;
	}

	/**
	 * exit / cancel / quit. Clears every child / pending service / payment /
	 * appraised-item id and marks the host request completed exactly once.
	 * Returns true on the first endSession so the caller completes the host
	 * request (kAsyncShopDone) and restores the caller location ONCE.
	 */
	bool endSession() {
		bool firstCompletion = _active && !_hostRequestCompleted;
		_active = false;
		_entryResetOwed = false;
		_owningStage = OWN_NONE;
		_activeChild = Goldbox::ECL::RequestKey();
		_pendingService = Goldbox::ECL::RequestKey();
		_paymentTarget = Goldbox::ECL::RequestKey();
		_appraisedItem = Goldbox::ECL::RequestKey();
		if (firstCompletion)
			_hostRequestCompleted = true;
		return firstCompletion;
	}

	// --- Single-route dispatch ----------------------------------------

	/**
	 * Record a main-menu dispatch. There is ONE route; calling this once per
	 * selection yields _dispatchCount == 1 for that selection. If a second
	 * (queued) interpretation of the same selection were wired, this would be
	 * called twice and the count would reveal the double-dispatch hazard.
	 */
	void recordDispatch(MenuAction action) {
		_lastAction = action;
		++_dispatchCount;
	}

	/** Reset the per-selection dispatch counter (called before each selection). */
	void beginSelection() { _dispatchCount = 0; _lastAction = ACT_NONE; }

	uint32 dispatchCount() const { return _dispatchCount; }
	MenuAction lastAction() const { return _lastAction; }

	// --- Stable-id child / transaction tracking (Req 29.4, GB13-009) ---

	void setOwningStage(OwningStage stage) { _owningStage = stage; }
	OwningStage owningStage() const { return _owningStage; }

	void setActiveChild(const Goldbox::ECL::RequestKey &key) { _activeChild = key; }
	const Goldbox::ECL::RequestKey &activeChild() const { return _activeChild; }

	void setPendingService(const Goldbox::ECL::RequestKey &key) { _pendingService = key; }
	const Goldbox::ECL::RequestKey &pendingService() const { return _pendingService; }

	void setPaymentTarget(const Goldbox::ECL::RequestKey &key) { _paymentTarget = key; }
	const Goldbox::ECL::RequestKey &paymentTarget() const { return _paymentTarget; }

	void setAppraisedItem(const Goldbox::ECL::RequestKey &key) { _appraisedItem = key; }
	const Goldbox::ECL::RequestKey &appraisedItem() const { return _appraisedItem; }

	bool hasActiveChild() const { return _activeChild.id != 0; }
	bool hasPendingService() const { return _pendingService.id != 0; }
	bool hasPaymentTarget() const { return _paymentTarget.id != 0; }
	bool hasAppraisedItem() const { return _appraisedItem.id != 0; }

	// --- Captured entry state -----------------------------------------

	ServiceKind kind() const { return _kind; }
	uint16 savedCharId() const { return _savedCharId; }
	const LocationContext &callerLocation() const { return _callerLocation; }
	bool hostRequestCompleted() const { return _hostRequestCompleted; }

private:
	ServiceKind _kind;
	bool _active;
	bool _entryResetOwed;
	uint16 _savedCharId;

	uint32 _dispatchCount;
	MenuAction _lastAction;

	OwningStage _owningStage;
	LocationContext _callerLocation;

	Goldbox::ECL::RequestKey _activeChild;
	Goldbox::ECL::RequestKey _pendingService;
	Goldbox::ECL::RequestKey _paymentTarget;
	Goldbox::ECL::RequestKey _appraisedItem;

	bool _hostRequestCompleted;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_SHOP_SESSION_H
