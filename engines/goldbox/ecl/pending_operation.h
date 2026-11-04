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

#ifndef GOLDBOX_ECL_PENDING_OPERATION_H
#define GOLDBOX_ECL_PENDING_OPERATION_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/str.h"

namespace Goldbox {
namespace ECL {

/**
 * GB13-009 continuation / yield contract (sub-task 8.1 scaffolding).
 *
 * This header introduces the typed, keyed pending-operation model that
 * generalizes the per-kind async booleans currently kept on
 * PoolradEngineHostImpl (_asyncMenuPending, _asyncShopPending,
 * _asyncCombatPending, _asyncDelayPending, _asyncPrintPending) and the
 * hasPendingAsync()/isPendingAsyncReady()/finalizePendingAsync() hooks.
 *
 * 8.1 only introduces the data structures and the stack API and wires the
 * stack ALONGSIDE the existing booleans without changing current behavior.
 * Single-owner completion matching (8.2), the full 0x24 tail / outer-loop
 * phases (8.3) and equivalence/staleness tests (8.4) come later.
 */

/** Category of host-side work an operation represents. */
enum class OperationKind : uint8 {
    Input,
    Encounter,
    Service,
    Battle,
    Settlement,
    Camp,
    Program,
    Exploration
};

/** How a completion resolved the operation that requested it. */
enum class CompletionKind : uint8 {
    Selected,
    Returned,
    Cancelled,
    Quit,
    Failed
};

/**
 * Identity for exactly-once + staleness checks.
 *
 * @c id is monotonic per host; @c generation is bumped on NEW ECL, new game
 * load and new battle setup (8.2) so a late/duplicate/out-of-generation
 * completion can be discarded deliberately.
 */
struct RequestKey {
    uint32 id;
    uint32 generation;

    RequestKey() : id(0), generation(0) {}
    RequestKey(uint32 keyId, uint32 keyGeneration)
        : id(keyId), generation(keyGeneration) {}

    bool operator==(const RequestKey &other) const {
        return id == other.id && generation == other.generation;
    }
    bool operator!=(const RequestKey &other) const {
        return !(*this == other);
    }
};

/**
 * Forward/optional return-context placeholder (TODO(GB13-021)).
 *
 * LocationContext is introduced by GB13-021 (task 15), which is not yet
 * implemented. To avoid a hard build dependency on a type that does not exist
 * yet, PendingOperation carries a small opaque scalar return-context field.
 * GB13-021 replaces this with the real LocationContext owner; the scalars here
 * are only enough to remember "which caller location to restore" until then.
 */
struct ReturnContextRef {
    uint8  kind;      // opaque location-kind tag; interpreted by GB13-021
    uint8  region;    // opaque region/atlas-offset tag
    uint16 activeId;  // opaque active ECL/GEO id

    ReturnContextRef() : kind(0), region(0), activeId(0) {}
};

/**
 * One suspended host operation.
 *
 * Owns its operands/text/destination addresses by value (Common containers);
 * it NEVER holds pointers into decoder scratch buffers, because the decoder
 * reuses those between instructions.
 */
struct PendingOperation {
    RequestKey       key;
    OperationKind    kind;
    uint16           phase;        // which sub-step this operation is on
    uint16           nextPc;       // decoder-produced resume PC (getNextInsnPc)
    uint16           savedCharId;  // stable selected-character ID, restored on finish
    ReturnContextRef returnCtx;    // TODO(GB13-021): becomes LocationContext

    // A frame neutralized by invalidateForCharacter() because the character or
    // combatant it referenced actually left (Requirement 4.4). A neutralized
    // frame still occupies its stack slot (so parent/child nesting stays
    // intact) but can no longer consume a completion, so a stale callback for
    // a departed id cannot act on it.
    bool             invalidated;

    // True once deliver() has handed this frame its completion. A second
    // completion for the same key is a duplicate and is discarded.
    bool             completionDelivered;

    // Owned, kind-specific state. Copied in at push time; no decoder aliasing.
    Common::Array<uint16> destinations; // VM result/destination addresses
    Common::Array<uint8>  operands;     // decoded operand bytes
    Common::String        text;         // prompt / message text

    PendingOperation()
        : kind(OperationKind::Input), phase(0), nextPc(0), savedCharId(0),
          invalidated(false), completionDelivered(false) {}
};

/**
 * Typed completion payload kept distinct per result shape.
 *
 * RuntimeExchange::AsyncCompletion carries a single int16 that cannot
 * represent an unsigned number input (full 0..65535) unambiguously, so the
 * contract keeps a tagged value instead of reusing int16. 8.2 wires the
 * single-owner matching through RequestKey; 8.1 only defines the shape.
 */
struct Completion {
    enum PayloadTag : uint8 {
        kPayloadNone = 0,
        kPayloadNumber,   // unsigned 16-bit number input
        kPayloadIndex,    // menu selection index
        kPayloadCharId,   // stable character ID
        kPayloadText,     // text result
        kPayloadAction    // semantic action id
    };

    RequestKey     key;
    CompletionKind kind;
    PayloadTag     tag;
    uint16         number;  // kPayloadNumber
    int16          index;   // kPayloadIndex
    uint16         charId;  // kPayloadCharId
    uint16         action;  // kPayloadAction
    Common::String text;    // kPayloadText

    Completion()
        : kind(CompletionKind::Returned), tag(kPayloadNone),
          number(0), index(0), charId(0), action(0) {}
};

/**
 * Host-owned stack of PendingOperation frames.
 *
 * Backed by a Common::Array used as a LIFO stack and OWNED by the host
 * (PoolradEngineHostImpl member) — never a global and with no runtime global
 * constructor. A nested dialog pushes a child frame and may yield while the
 * parent stays at its phase.
 *
 * 8.1 provides push/top/pop/advance and resumePcWhenFinished(). Owner- and
 * generation-checked completion delivery (deliver()), the finish-tail-before-
 * fetch driver (runTailAndAdvance) and generation bumping are finished in
 * 8.2/8.3; the API hooks are present and documented here.
 */
class PendingOperationStack {
public:
    PendingOperationStack() : _nextId(1), _generation(0) {}

    /** True when no operation is suspended. */
    bool empty() const { return _frames.empty(); }

    /** Number of suspended frames (parent + children). */
    uint32 size() const { return _frames.size(); }

    /** Current generation (bumped on NEW ECL / new game / new battle). */
    uint32 generation() const { return _generation; }

    /**
     * Push a new operation frame and return its RequestKey.
     *
     * The recorded @p nextPc is the decoder-produced resume PC captured from
     * EclVM::getNextInsnPc at VM_YIELD time; it is where the VM resumes only
     * AFTER this operation's tail has run and its phase has advanced to
     * finished.
     */
    RequestKey push(OperationKind kind, uint16 nextPc, uint16 savedCharId,
            const ReturnContextRef &returnCtx);

    /** Top (most recently pushed) frame, or nullptr when empty. */
    PendingOperation *top();
    const PendingOperation *top() const;

    /** Pop the top frame. No-op when empty. */
    void pop();

    /**
     * Advance the top frame's phase by one sub-step.
     *
     * Advancing is explicit so the tail (mutation/cleanup) always runs before
     * the VM fetches the recorded resume PC; the VM must never both re-execute
     * the opcode and finalize its mutation.
     */
    void advancePhase();

    /**
     * Resume PC the VM should fetch once the top operation is finished.
     *
     * Returns the decoder-produced nextPc recorded at push time. 0 when the
     * stack is empty.
     */
    uint16 resumePcWhenFinished() const;

    /**
     * Owner- and generation-checked completion delivery (GB13-009, 8.2).
     *
     * Single-owner matching: ONLY the matching active owner consumes a
     * completion. A frame is the matching active owner when it is the top
     * frame, it is not invalidated, its RequestKey::id equals @p key.id AND
     * its RequestKey::generation equals the stack's current generation. When a
     * frame matches, @p completion is copied into @p out and true is returned;
     * the caller (the owning operation, 8.3) applies the typed payload to VM
     * memory. The match is consumed exactly once per owner: the frame records
     * that a completion was delivered so a duplicate for the same key is
     * discarded.
     *
     * A completion is discarded deliberately (returns false, @p out untouched)
     * and logged when:
     *  - the stack is empty, or
     *  - the id does not match the active owner (wrong/stale owner), or
     *  - the generation does not match the current generation (a late result
     *    from a NEW ECL / new-game / new-battle context that has since been
     *    replaced), or
     *  - the active owner was invalidated because its character/combatant left,
     *    or
     *  - a completion for this owner was already delivered (duplicate).
     *
     * "Exactly once" here applies to the MUTATION a completion drives, not to
     * legitimate repeated original menu-entry effects: re-entering a menu mints
     * a NEW request/key and pushes (or re-pushes) a frame, so its effects run
     * again. deliver() only prevents the SAME completion key from being applied
     * more than once.
     */
    bool deliver(const RequestKey &key, const Completion &completion,
            Completion &out);

    /**
     * Invalidate any frame referencing a departed character/combatant id so a
     * stale callback cannot act on it (Requirement 4.4).
     *
     * A frame whose savedCharId equals @p charId is marked invalidated and can
     * no longer consume a completion. Returns the number of frames neutralized.
     * This is the stack-level API; the character/combatant removal seams that
     * CALL it live in not-yet-done findings (GB13-006 ADD NPC / LOAD CHARACTER
     * removal is task 11; GB13-011 combat teardown is task 12), so 8.2 provides
     * and unit-tests the mechanism and leaves documented TODO hooks there.
     */
    uint32 invalidateForCharacter(uint16 charId);

    /**
     * Run the top frame's tail, advance its phase and pop when finished.
     *
     * TODO(8.3): models the 0x24 cleanup/finalization tail and outer-loop
     * phases. 8.1 leaves the driver unimplemented so existing async flows keep
     * using the legacy booleans.
     */
    void runTailAndAdvance();

    /** Bump the generation. TODO(8.2): call on NEW ECL / new game / new battle. */
    void bumpGeneration() { ++_generation; }

private:
    Common::Array<PendingOperation> _frames;
    uint32 _nextId;
    uint32 _generation;
};

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_PENDING_OPERATION_H
