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

#include "goldbox/ecl/pending_operation.h"

#include "common/debug.h"

namespace Goldbox {
namespace ECL {

RequestKey PendingOperationStack::push(OperationKind kind, uint16 nextPc,
        uint16 savedCharId, const ReturnContextRef &returnCtx) {
    PendingOperation frame;
    frame.key = RequestKey(_nextId++, _generation);
    frame.kind = kind;
    frame.phase = 0;
    frame.nextPc = nextPc;
    frame.savedCharId = savedCharId;
    frame.returnCtx = returnCtx;
    _frames.push_back(frame);
    return frame.key;
}

PendingOperation *PendingOperationStack::top() {
    if (_frames.empty())
        return nullptr;
    return &_frames[_frames.size() - 1];
}

const PendingOperation *PendingOperationStack::top() const {
    if (_frames.empty())
        return nullptr;
    return &_frames[_frames.size() - 1];
}

void PendingOperationStack::pop() {
    if (!_frames.empty())
        _frames.pop_back();
}

void PendingOperationStack::advancePhase() {
    PendingOperation *frame = top();
    if (frame)
        ++frame->phase;
}

uint16 PendingOperationStack::resumePcWhenFinished() const {
    const PendingOperation *frame = top();
    return frame ? frame->nextPc : 0;
}

bool PendingOperationStack::deliver(const RequestKey &key,
        const Completion &completion, Completion &out) {
    // GB13-009 (8.2): single-owner + generation-checked matching. Only the
    // matching active owner (the top frame) consumes a completion; every other
    // case is discarded deliberately and logged via debug() (no PC is
    // available here; the host/exchange layer logs through
    // g_events->postEclSyscallMessage where a PC exists).
    PendingOperation *frame = top();
    if (!frame) {
        debug(3, "PendingOperationStack::deliver: discard id=%u gen=%u (stack empty)",
            (unsigned)key.id, (unsigned)key.generation);
        return false;
    }

    if (key.generation != _generation) {
        debug(3, "PendingOperationStack::deliver: discard id=%u gen=%u (out-of-generation, current=%u)",
            (unsigned)key.id, (unsigned)key.generation, (unsigned)_generation);
        return false;
    }

    if (frame->key.id != key.id || frame->key.generation != _generation) {
        debug(3, "PendingOperationStack::deliver: discard id=%u gen=%u (not active owner, owner id=%u gen=%u)",
            (unsigned)key.id, (unsigned)key.generation,
            (unsigned)frame->key.id, (unsigned)frame->key.generation);
        return false;
    }

    if (frame->invalidated) {
        debug(3, "PendingOperationStack::deliver: discard id=%u gen=%u (owner invalidated; char left)",
            (unsigned)key.id, (unsigned)key.generation);
        return false;
    }

    if (frame->completionDelivered) {
        debug(3, "PendingOperationStack::deliver: discard id=%u gen=%u (duplicate completion)",
            (unsigned)key.id, (unsigned)key.generation);
        return false;
    }

    frame->completionDelivered = true;
    out = completion;
    return true;
}

uint32 PendingOperationStack::invalidateForCharacter(uint16 charId) {
    uint32 count = 0;
    for (uint i = 0; i < _frames.size(); ++i) {
        PendingOperation &frame = _frames[i];
        if (!frame.invalidated && frame.savedCharId == charId) {
            frame.invalidated = true;
            ++count;
            debug(3, "PendingOperationStack::invalidateForCharacter: neutralized frame id=%u gen=%u for departed char=%u",
                (unsigned)frame.key.id, (unsigned)frame.key.generation,
                (unsigned)charId);
        }
    }
    return count;
}

void PendingOperationStack::runTailAndAdvance() {
    // The 0x24 ENCOUNTER cleanup tail (GB13-009, 8.3) is run during the host's
    // finalizePendingAsync() resume seam via ECL::runEncounterCleanupTail(),
    // ALONGSIDE the existing _async*Pending booleans, rather than being driven
    // through this stack. That keeps the live combat/shop/temple resume path
    // behavior-preserving and avoids re-sequencing the whole event loop in
    // this task. This generic phase-advancing driver stays available for the
    // later findings (GB13-004 scripted damage, GB13-010, GB13-022 exploration
    // state machine) that push real frames onto the stack; it is intentionally
    // a no-op until a frame-driven operation needs it.
}

} // namespace ECL
} // namespace Goldbox
