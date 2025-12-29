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

#ifndef GOLDBOX_ECL_SYSCALL_IMPL_H
#define GOLDBOX_ECL_SYSCALL_IMPL_H

#include "common/scummsys.h"
#include "common/str.h"
#include "common/array.h"
#include "goldbox/ecl/syscall_handler.h"
#include "goldbox/ecl/ecl_memory.h"

namespace Goldbox {

class Engine;
class View;

namespace ECL {

/**
 * ECL Syscall Handler Implementation
 * 
 * Bridges ECL VM syscalls to ScummVM's Engine, View, and Dialog systems.
 * Handles:
 * - Text output → Message box / Dialog
 * - Player input → Dialog input fields
 * - Combat → Combat engine / Dialog
 * - Menus → Menu dialogs / View selection
 * - Pictures → Picture display system
 * - Scripts → ECL VM loader
 */
class EclSyscallImpl : public SyscallHandler {
public:
    /**
     * Constructor.
     * @param engine Reference to main GoldBox engine
     * @param memory Reference to ECL virtual memory for reading/writing results
     */
    EclSyscallImpl(::Goldbox::Engine *engine, AddressSpace *memory);
    ~EclSyscallImpl() override = default;

    // SyscallHandler interface implementation
    void printText(const Common::String &text, bool clearBox = false) override;
    
    VmResult inputNumber(uint8 maxDigits, uint16 resultAddr) override;
    VmResult inputString(uint8 maxLength, uint16 resultAddr) override;
    
    VmResult displayPicture(uint8 picID) override;
    
    VmResult verticalMenu(const Common::String &message,
            const Common::Array<Common::String> &options, uint16 resultAddr) override;
    VmResult horizontalMenu(const Common::Array<Common::String> &options, uint16 resultAddr) override;
    
    VmResult startCombat() override;
    VmResult executeProgram(uint8 programID) override;
    VmResult clearTextBox() override;
    
    VmResult loadScript(uint8 scriptID) override;
    
    YieldReason getYieldReason() const override { return _yieldReason; }
    void setYieldReason(YieldReason reason) override { _yieldReason = reason; }

private:
    ::Goldbox::Engine *_engine;         // Reference to main engine
    AddressSpace *_memory;               // Reference to ECL virtual memory
    YieldReason _yieldReason;            // Current yield state
    uint16 _pendingInputAddr;            // Address to store input result
    
    // Helper methods for dialog/view integration
    void _showMessageBox(const Common::String &text, bool clear);
    View *_getCurrentView() const;
    void _updateViewState();
};

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_SYSCALL_IMPL_H
