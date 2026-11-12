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
 * Assisted-by: Claude:claude-opus-4-20250514
 */

#ifndef GOLDBOX_POOLRAD_REST_OPERATION_H
#define GOLDBOX_POOLRAD_REST_OPERATION_H

#include "common/array.h"
#include "common/scummsys.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {
class PoolradCharacter;
}

/**
 * Shared camp/spell-book rest operation (GB13-018).
 *
 * Owns the full resting simulation outside any rendering dialog: requested
 * duration, elapsed counters, per-character preparation timers, the base
 * preparation hour counter, the 288-tick heal counter, the safe-rest
 * interruption counter and the completion reason. Both Camp Rest
 * (camp_menu_dialog.cpp) and SpellBook Rest (spell_book_dialog.cpp) construct
 * and drive the same operation; the dialogs stay thin.
 *
 * Port of GameTime.CampRest (gametime.pas:411) and its helpers
 * CheckRestingHeal (269), AdvanceSpellPrep (355) and AdvanceSpellPrepBase
 * (381). Each tick advances exactly five minutes and runs, in this order:
 *   1. decrement requested time
 *   2. advance game clock + effects
 *   3. heal counter  (1 HP / 288 ticks per member, ordinary restoration)
 *   4. spell preparation  (scribe before memorize, gated by 3-ticks/level)
 *   5. base preparation   (decrements hourly, i.e. every 12 ticks)
 *   6. interruption check (only when REST_SAFE_TIME > 0 and threshold expires)
 *
 * The two engine-coupled side effects (advancing the VM clock with timed
 * effect expiry, and rolling d100 for interruption) are injected through
 * RestEnvironment so the pure timer/counter/heal/prep logic can be unit
 * tested without g_engine. The host wiring supplies the real implementation;
 * ECL (ONREST/ONRESTINTR) is NOT invoked here - the operation only reports
 * the completion reason and the wiring/host layer decides.
 */
class RestOperation {
public:
	/**
	 * Why resting ended. Only REASON_INTERRUPTED later runs the camp
	 * interruption ECL entry (ONRESTINTR); the others do not.
	 */
	enum Reason {
		REASON_NONE = 0,        // still resting
		REASON_STOP = 1,        // player answered "Stop Resting? Y"
		REASON_ELAPSED = 2,     // requested duration fully consumed
		REASON_INTERRUPTED = 3, // scripted/random interruption roll hit
		REASON_QUIT = 4         // application quit
	};

	/**
	 * Injected engine side effects. Kept minimal so the operation stays
	 * pure and testable: the host implementation forwards to timeAddUnits
	 * and VmInterface::rollDice; tests supply deterministic fakes.
	 */
	class Environment {
	public:
		virtual ~Environment() {}

		// Advance the game clock by five minutes and process timed effect
		// expiry (original GameTime.AdvanceTime(5, MINUTES)).
		virtual void advanceFiveMinutes() = 0;

		// Restore one hit point to this member via the ordinary restoration
		// routine (original FxEngine.RestoreHitPoints(cur, 1, False)).
		// Returns true if any HP was actually restored.
		virtual bool restoreOneHitPoint(Data::PoolradCharacter *member) = 0;

		// Roll 1..100 for the interruption check (original
		// FxEngine.RollDice(1, 100)).
		virtual int rollInterruptionPercent() = 0;
	};

	RestOperation();

	/**
	 * Configure the rest from the party and the area safe-rest fields.
	 *
	 * @param party           party members in list order (index 1-based in
	 *                         the original; stored 0-based here).
	 * @param requestedMinutes total minutes to rest (max regain time, as
	 *                         ACTION_Rest computes it from the party).
	 * @param safeTime        REST_SAFE_TIME (zone1): ticks between
	 *                         interruption rolls; 0 disables interruption.
	 * @param interruptPct    REST_INTERRUPT_PCT (zone1): d100 threshold.
	 */
	void start(const Common::Array<Data::PoolradCharacter *> &party,
			uint32 requestedMinutes, uint8 safeTime, uint8 interruptPct);

	/**
	 * Run exactly one five-minute tick. Returns true while resting should
	 * continue (not finished and not interrupted). The caller batches a
	 * bounded number of ticks per ScummVM frame without changing order.
	 */
	bool advanceTick(Environment &env);

	// Player pressed Stop and confirmed. Records REASON_STOP; no extra tick.
	void requestStop();
	// Scripted interruption from an ECL event. Records REASON_INTERRUPTED.
	void requestScriptedInterruption();
	// Application quit. Records REASON_QUIT.
	void requestQuit();

	// True while there is remaining requested time and no completion reason.
	bool isResting() const;
	// True once resting has ended for any reason.
	bool isFinished() const { return _reason != REASON_NONE; }
	Reason reason() const { return _reason; }

	// Whether the completion should run the camp interruption ECL entry
	// (ONRESTINTR). Only a genuine interruption does.
	bool shouldRunInterruptionEcl() const {
		return _reason == REASON_INTERRUPTED;
	}

	// Remaining requested minutes (original dsRestingTimeGbvm converted).
	uint32 remainingMinutes() const { return _remainingMinutes; }
	// Total five-minute ticks simulated so far (elapsed = this * 5 minutes).
	uint32 elapsedTicks() const { return _elapsedTicks; }

	// Introspection for the wiring/tests.
	uint16 healTicks() const { return _healTicks; }
	uint8 baseTick() const { return _baseTick; }
	uint8 prepTimer(uint index) const {
		return index < _prepTimers.size() ? _prepTimers[index] : 0;
	}
	uint16 restTicks() const { return _restTicks; }

private:
	// Tick steps, in the original order.
	void decrementRequestedTime();      // step 1
	void checkRestingHeal(Environment &env);        // step 3
	void advanceSpellPrep();            // step 4 (per-char 3-tick timers)
	void advanceSpellPrepBase();        // step 5 (hourly base timers)
	bool checkInterruption(Environment &env);        // step 6

	// Run the scribe-before-memorize completion for one member, returning
	// the level of the work completed (0 if none) for the 3-ticks/level
	// timer reload. Mirrors CheckScribeDone / CheckMemorizeDone ordering.
	uint8 completeOnePreparation(Data::PoolradCharacter *member);

	Common::Array<Data::PoolradCharacter *> _party;
	Common::Array<uint8> _prepTimers; // per-char 3-ticks/level timer (byte wrap)

	uint32 _remainingMinutes;
	uint32 _elapsedTicks;

	uint16 _healTicks;  // 1 HP per member every 288 ticks
	uint8 _baseTick;    // hourly base-prep counter, wraps at 12 ticks
	uint16 _restTicks;  // safe-rest counter toward REST_SAFE_TIME

	uint8 _safeTime;
	uint8 _interruptPct;

	Reason _reason;
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_REST_OPERATION_H
