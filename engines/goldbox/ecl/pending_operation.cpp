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
        const Completion &completion) {
    (void)completion;

    // TODO(8.2): single-owner + generation matching. Only the matching active
    // owner may consume a completion; stale/duplicate/out-of-generation
    // completions are discarded deliberately and logged. 8.1 keeps the check
    // minimal so no existing async flow routes through the stack yet.
    PendingOperation *frame = top();
    if (!frame || frame->key != key)
        return false;
    return true;
}

void PendingOperationStack::runTailAndAdvance() {
    // TODO(8.3): run the owning operation's mutation/cleanup tail (the 0x24
    // service/combat/loot + search/display reset, NewEclDone, GameLoop
    // post-call work), advance its phase and pop when finished, so the tail
    // runs exactly once before the VM fetches resumePcWhenFinished().
}

} // namespace ECL
} // namespace Goldbox
