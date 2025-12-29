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

#include "goldbox/engine.h"  // for accessing engine methods
#include "goldbox/vm_interface.h"
#include "goldbox/core/icon_manager.h"
#include "goldbox/poolrad/poolrad.h"

namespace Goldbox {

IconManager *VmInterface::getIconManager() {
	Poolrad::PoolradEngine *engine = dynamic_cast<Poolrad::PoolradEngine *>(g_engine);
	return engine ? engine->getIconManager() : nullptr;
}

VM::VM() {
    reset();
    _entryPoints.resize(5, 0xFFFF); // Initialize all entry points to invalid
}

void VM::reset() {
    _state.pc = 0;
    _state.stack.clear();
}

VmResult VM::step() {
    // Placeholder: real dispatch to be implemented with ECL opcodes
    return VM_YIELD;
}

VmResult VM::run(uint32 maxSteps) {
    for (uint32 i = 0; i < maxSteps; ++i) {
        VmResult r = step();
        if (r != VM_OK && r != VM_YIELD) {
            return r;
        }
    }
    return VM_OK;
}

VmResult VM::runAtEntryPoint(EntryPointSelector entry, uint32 maxSteps) {
    uint16 offset = getEntryPoint(entry);
    if (offset == 0xFFFF) {
        return VM_ERROR;
    }
    _state.pc = offset;
    return run(maxSteps);
}

VmResult VM::loadEntryPoints(Common::Span<const uint8> program) {
    if (program.size() < 10) {
        // Need at least 5 x VAL16 (2 bytes each) = 10 bytes
        return VM_ERROR;
    }

    for (int i = 0; i < 5; ++i) {
        uint16 offset = program[i * 2] | (program[i * 2 + 1] << 8);
        _entryPoints[i] = offset;
    }

    return VM_OK;
}

uint16 VM::getEntryPoint(EntryPointSelector entry) const {
    int idx = static_cast<int>(entry);
    if (idx >= 0 && idx < (int)_entryPoints.size()) {
        return _entryPoints[idx];
    }
    return 0xFFFF;
}

} // namespace Goldbox
