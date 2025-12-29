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

#include "goldbox/ecl/ecl_syscall_impl.h"
#include "goldbox/engine.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/vm_interface.h"
#include "common/str.h"

namespace Goldbox {
namespace ECL {

EclSyscallImpl::EclSyscallImpl(::Goldbox::Engine *engine, AddressSpace *memory)
    : _engine(engine), _memory(memory), _yieldReason(YieldReason::None), 
      _pendingInputAddr(0) {
}

void EclSyscallImpl::printText(const Common::String &text, bool clearBox) {
    if (!_engine) return;
    
    // Route to message box / text display system
    _showMessageBox(text, clearBox);
}

VmResult EclSyscallImpl::inputNumber(uint8 maxDigits, uint16 resultAddr) {
    if (!_engine || !_memory) return VmResult::VM_ERROR;
    
    // Delegate to input dialog
    // Store pending address for completion callback
    _pendingInputAddr = resultAddr;
    _yieldReason = YieldReason::WaitingForInput;
    
    // TODO: Show input number dialog via View/Dialog system
    // For now, return YIELD to pause execution
    // Dialog completion will resume VM and store result at resultAddr

    return VmResult::VM_YIELD;
}

VmResult EclSyscallImpl::inputString(uint8 maxLength, uint16 resultAddr) {
    if (!_engine || !_memory) return VmResult::VM_ERROR;
    
    // Delegate to input dialog
    _pendingInputAddr = resultAddr;
    _yieldReason = YieldReason::WaitingForInput;
    
    // TODO: Show input string dialog via View/Dialog system
    
    return VmResult::VM_YIELD;
}

VmResult EclSyscallImpl::displayPicture(uint8 picID) {
    if (!_engine) return VmResult::VM_ERROR;
    
    if (picID == 255) {
        // End picture display, restore normal view
        _updateViewState();
		return VmResult::VM_OK;
    }
    
    // Route to picture display system
    // TODO: Load and display picture from PIC?.DAX
    
    _yieldReason = YieldReason::WaitingForPicture;
	return VmResult::VM_YIELD;
}

VmResult EclSyscallImpl::verticalMenu(const Common::String &message,
        const Common::Array<Common::String> &options, uint16 resultAddr) {
    if (!_engine || !_memory) return VmResult::VM_ERROR;
    
    _pendingInputAddr = resultAddr;
    _yieldReason = YieldReason::WaitingForMenu;
    
    // Route to vertical menu dialog
    // TODO: Show menu dialog via Dialog system, store selection (0-based) at resultAddr
    
    return VmResult::VM_YIELD;
}

VmResult EclSyscallImpl::horizontalMenu(const Common::Array<Common::String> &options, 
        uint16 resultAddr) {
    if (!_engine || !_memory) return VmResult::VM_ERROR;
    
    _pendingInputAddr = resultAddr;
    _yieldReason = YieldReason::WaitingForMenu;
    
    // Route to horizontal menu dialog (bottom bar menu)
    // TODO: Show menu on bottom bar, store selection at resultAddr
    
    return VmResult::VM_YIELD;
}

VmResult EclSyscallImpl::startCombat() {
    if (!_engine) return VmResult::VM_ERROR;
    
    _yieldReason = YieldReason::WaitingForCombat;
    
    // Delegate to Combat system
    // TODO: Initiate combat with monsters from ECL memory
    // Engine should handle combat resolution and store results in memory
    
    return VmResult::VM_YIELD;
}

VmResult EclSyscallImpl::executeProgram(uint8 programID) {
    if (!_engine) return VmResult::VM_ERROR;
    
    _yieldReason = YieldReason::WaitingForScript;
    
    // Route to appropriate program handler:
    // programID 0  = training hall (show character training dialog)
    // programID 8  = win game (show victory screen/credits)
    // programID 9  = camp (show rest/camp menu)
    
    switch (programID) {
        case 0:
            // TODO: Show training hall dialog
            // Uses class bitmask from ECL memory at 0x6DA8
            break;
        case 8:
            // TODO: Show win game / victory sequence
            break;
        case 9:
            // TODO: Show camp / rest menu
            break;
        default:
            warning("Unknown PROGRAM ID: %d", programID);
            return VmResult::VM_ERROR;
    }
    
    return VmResult::VM_YIELD;
}

VmResult EclSyscallImpl::clearTextBox() {
    if (!_engine) return VmResult::VM_ERROR;
    
    // Clear the message/text box display
    _showMessageBox(Common::String(""), true);
    
    return VmResult::VM_OK;
}

VmResult EclSyscallImpl::loadScript(uint8 scriptID) {
    if (!_engine) return VmResult::VM_ERROR;
    
    _yieldReason = YieldReason::WaitingForScript;
    
    // Delegate to ECL VM script loader
    // TODO: Engine should call ECL VM to load new script
    
    return VmResult::VM_YIELD;
}

void EclSyscallImpl::_showMessageBox(const Common::String &text, bool clear) {
    if (!_engine) return;
    
    // Integrate with message box system
    // TODO: Route to appropriate message display:
    // - Print to text buffer at ECL memory 0x84c8
    // - Update print flags at 0x84de (print ready) and 0x84df (output complete)
    // - Set text print flag for UI rendering
    
    // For now, route through existing message system
    // Engine's View should poll memory flags to render text
}

View *EclSyscallImpl::_getCurrentView() const {
    if (!_engine) return nullptr;
    
    // TODO: Return current active View from engine
    // Used for menu routing and display coordination
    
    return nullptr;
}

void EclSyscallImpl::_updateViewState() {
    if (!_engine) return;
    
    // After picture/menu/input completion, refresh view state
    // TODO: Call engine to redraw current view
}

} // namespace ECL
} // namespace Goldbox
