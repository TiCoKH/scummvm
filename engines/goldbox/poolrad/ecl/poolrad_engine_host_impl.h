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

#ifndef GOLDBOX_POOLRAD_ECL_POOLRAD_ENGINE_HOST_IMPL_H
#define GOLDBOX_POOLRAD_ECL_POOLRAD_ENGINE_HOST_IMPL_H

#include "common/scummsys.h"
#include "common/str.h"
#include "common/array.h"
#include "common/ptr.h"
#include "goldbox/ecl/ecl_engine_host.h"
#include "goldbox/ecl/ecl_memory.h"
#include "goldbox/ecl/syscall_handler.h"

namespace Goldbox {

class Engine;
class View;

namespace Gfx {
class DaxTile;
}

namespace Poolrad {

/**
 * Pool of Radiance implementation of the ECL engine host interface.
 *
 * Bridges ECL VM calls to Pool of Radiance engine systems:
 * - Text output   → Message box / Dialog
 * - Player input  → Dialog input fields
 * - Combat        → Combat engine / Dialog
 * - Menus         → Menu dialogs / View selection
 * - Pictures      → Picture display system
 * - Scripts       → ECL VM loader
 * - Wall sets     → Poolrad walldef + DaxTile cache
 *
 * Inherits both EclEngineHost (full game-host contract) and SyscallHandler
 * (low-level VM dispatch contract) so a single instance can be wired directly
 * to EclVM::setSyscallHandler().
 */
class PoolradEngineHostImpl : public ECL::EclEngineHost {
public:
    /**
     * @param engine Reference to main GoldBox engine (must be PoolradEngine)
     * @param memory Reference to ECL virtual memory for reading/writing results
     */
    PoolradEngineHostImpl(::Goldbox::Engine *engine, ECL::AddressSpace *memory);
    ~PoolradEngineHostImpl() override = default;

    // EclEngineHost interface
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

    VmResult loadScript(uint8 scriptID) override;
    VmResult loadWallSet(uint8 blockId, uint8 setSlot) override;
    VmResult loadGeoBlock(uint8 blockId) override;

private:
    ::Goldbox::Engine *_engine;
    ECL::AddressSpace *_memory;

    // Owned DaxTile instances for walldef tile atlases (slots 1-3, 0-based idx)
    Common::ScopedPtr<Gfx::DaxTile> _walldefTiles[3];

    void _showMessageBox(const Common::String &text, bool clear);
    View *_getCurrentView() const;
    void _updateViewState();
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_ECL_POOLRAD_ENGINE_HOST_IMPL_H
