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
#include "goldbox/ecl/ecl_engine_host.h"
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
class EclSyscallImpl : public EclEngineHost {
public:
    /**
     * Constructor.
     * @param engine Reference to main GoldBox engine
     * @param memory Reference to ECL virtual memory for reading/writing results
     */
    EclSyscallImpl(::Goldbox::Engine *engine, AddressSpace *memory);
    virtual ~EclSyscallImpl() override = default;

    // SyscallHandler interface implementation
    void printText(const Common::String &text, bool clearBox = false) override;

    int16 inputNumber(uint8 maxDigits) override;
    Common::String inputString(uint8 maxLength) override;

    VmResult displayPicture(uint8 picID) override;

    int16 verticalMenu(const Common::String &message,
            const Common::Array<Common::String> &options) override;
    int16 horizontalMenu(const Common::Array<Common::String> &options) override;

    VmResult startCombat() override;
    VmResult executeProgram(uint8 programID) override;
    void clearTextBox() override;
    void setTextDelayEnabled(bool enabled) override;

    VmResult loadScript(uint8 scriptID) override;
    VmResult loadWallSet(uint8 blockId, uint8 setSlot) override;
    VmResult loadGeoBlock(uint8 blockId) override;

protected:
    ::Goldbox::Engine *_engine;         // Reference to main engine
    AddressSpace *_memory;              // Reference to ECL virtual memory
    uint _baseTextDelay;                // Host-configured text delay value
    bool _textDelayEnabled;             // Legacy CFG_TEXT_DELAY equivalent

    // Helper methods for dialog/view integration
    void _showMessageBox(const Common::String &text, bool clear);
    View *_getCurrentView() const;
    void _updateViewState();
};

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_SYSCALL_IMPL_H
