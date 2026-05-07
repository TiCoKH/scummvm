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
    : _engine(engine), _memory(memory) {
}

void EclSyscallImpl::printText(const Common::String &text, bool clearBox) {
    if (!_engine) return;
    
    // Route to message box / text display system
    _showMessageBox(text, clearBox);
}

int16 EclSyscallImpl::inputNumber(uint8 maxDigits) {
    if (!_engine) return -1;

    // TODO: Show input number dialog via View/Dialog system and block until
    // the player confirms. Return the entered value.
    return 0;
}

Common::String EclSyscallImpl::inputString(uint8 maxLength) {
    if (!_engine) return Common::String();

    // TODO: Show input string dialog via View/Dialog system and block until
    // the player confirms. Return the entered string.
    return Common::String();
}

VmResult EclSyscallImpl::displayPicture(uint8 picID) {
    if (!_engine) return VmResult::VM_ERROR;

    if (picID == 255) {
        _updateViewState();
        return VmResult::VM_OK;
    }

    // TODO: Load and display picture from PIC?.DAX; block until display is
    // ready (or is immediate for still pictures).
    return VmResult::VM_OK;
}

int16 EclSyscallImpl::verticalMenu(const Common::String &message,
        const Common::Array<Common::String> &options) {
    if (!_engine) return -1;

    // TODO: Show vertical menu dialog via Dialog system; block until player
    // selects. Return 0-based index, or -1 on cancel.
    return 0;
}

int16 EclSyscallImpl::horizontalMenu(const Common::Array<Common::String> &options) {
    if (!_engine) return -1;

    // TODO: Show horizontal menu on bottom bar; block until player selects.
    // Return 0-based index, or -1 on cancel.
    return 0;
}

VmResult EclSyscallImpl::startCombat() {
    if (!_engine) return VmResult::VM_ERROR;

    // TODO: Initiate combat with monsters from ECL memory; block until
    // combat is resolved.
    return VmResult::VM_OK;
}

VmResult EclSyscallImpl::executeProgram(uint8 programID) {
    if (!_engine) return VmResult::VM_ERROR;

    switch (programID) {
        case 0:
            // TODO: Show training hall dialog; block until complete.
            break;
        case 8:
            // TODO: Show win game / victory sequence; block until complete.
            return VmResult::VM_HALTED;
        case 9:
            // TODO: Show camp / rest menu; block until complete.
            break;
        default:
            warning("EclSyscallImpl::executeProgram: unknown programID %d",
                    programID);
            return VmResult::VM_ERROR;
    }

    return VmResult::VM_OK;
}

void EclSyscallImpl::clearTextBox() {
    if (!_engine) return;
    _showMessageBox(Common::String(""), true);
}

VmResult EclSyscallImpl::loadScript(uint8 scriptID) {
    if (!_engine) return VmResult::VM_ERROR;

    // TODO: Load new ECL script via engine; the current script chain ends here.
    return VmResult::VM_HALTED;
}

VmResult EclSyscallImpl::loadWallSet(uint8 blockId, uint8 setSlot) {
    // Shared base has no game-specific wallset cache implementation.
    // Dialect hosts (e.g. Poolrad) override this.
    (void)blockId;
    (void)setSlot;
    return VmResult::VM_OK;
}

VmResult EclSyscallImpl::loadGeoBlock(uint8 blockId) {
    if (!_engine) return VmResult::VM_ERROR;
    // TODO: Load DaxBlockGeo from _engine->getDaxManager().getGeo()
    //       and store as current active map.
    return VmResult::VM_OK;
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
