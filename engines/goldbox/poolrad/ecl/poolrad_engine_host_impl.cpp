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

#include "goldbox/poolrad/ecl/poolrad_engine_host_impl.h"
#include "goldbox/engine.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/vm_interface.h"
#include "goldbox/gfx/dax_tile.h"
#include "goldbox/gfx/walldef_surface_builder.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/data/daxblockcontainer.h"
#include "common/str.h"

namespace Goldbox {
namespace Poolrad {

PoolradEngineHostImpl::PoolradEngineHostImpl(::Goldbox::Engine *engine,
        ECL::AddressSpace *memory)
    : _engine(engine), _memory(memory),
      _yieldReason(ECL::YieldReason::None), _pendingInputAddr(0) {
}

void PoolradEngineHostImpl::printText(const Common::String &text, bool clearBox) {
    if (!_engine) return;
    _showMessageBox(text, clearBox);
}

VmResult PoolradEngineHostImpl::inputNumber(uint8 maxDigits, uint16 resultAddr) {
    if (!_engine || !_memory) return VmResult::VM_ERROR;

    _pendingInputAddr = resultAddr;
    _yieldReason = ECL::YieldReason::WaitingForInput;

    // TODO: Show input number dialog via View/Dialog system
    return VmResult::VM_YIELD;
}

VmResult PoolradEngineHostImpl::inputString(uint8 maxLength, uint16 resultAddr) {
    if (!_engine || !_memory) return VmResult::VM_ERROR;

    _pendingInputAddr = resultAddr;
    _yieldReason = ECL::YieldReason::WaitingForInput;

    // TODO: Show input string dialog via View/Dialog system
    return VmResult::VM_YIELD;
}

VmResult PoolradEngineHostImpl::displayPicture(uint8 picID) {
    if (!_engine) return VmResult::VM_ERROR;

    if (picID == 255) {
        _updateViewState();
        return VmResult::VM_OK;
    }

    // TODO: Load and display picture from PIC?.DAX
    _yieldReason = ECL::YieldReason::WaitingForPicture;
    return VmResult::VM_YIELD;
}

VmResult PoolradEngineHostImpl::verticalMenu(const Common::String &message,
        const Common::Array<Common::String> &options, uint16 resultAddr) {
    if (!_engine || !_memory) return VmResult::VM_ERROR;

    _pendingInputAddr = resultAddr;
    _yieldReason = ECL::YieldReason::WaitingForMenu;

    // TODO: Show vertical menu dialog via Dialog system
    return VmResult::VM_YIELD;
}

VmResult PoolradEngineHostImpl::horizontalMenu(
        const Common::Array<Common::String> &options, uint16 resultAddr) {
    if (!_engine || !_memory) return VmResult::VM_ERROR;

    _pendingInputAddr = resultAddr;
    _yieldReason = ECL::YieldReason::WaitingForMenu;

    // TODO: Show horizontal menu on bottom bar
    return VmResult::VM_YIELD;
}

VmResult PoolradEngineHostImpl::startCombat() {
    if (!_engine) return VmResult::VM_ERROR;

    _yieldReason = ECL::YieldReason::WaitingForCombat;

    // TODO: Initiate combat with monsters from ECL memory
    return VmResult::VM_YIELD;
}

VmResult PoolradEngineHostImpl::executeProgram(uint8 programID) {
    if (!_engine) return VmResult::VM_ERROR;

    _yieldReason = ECL::YieldReason::WaitingForScript;

    switch (programID) {
    case 0:
        // TODO: Show training hall dialog
        break;
    case 8:
        // TODO: Show win game / victory sequence
        break;
    case 9:
        // TODO: Show camp / rest menu
        break;
    default:
        warning("PoolradEngineHostImpl::executeProgram: unknown programID %d",
                programID);
        return VmResult::VM_ERROR;
    }

    return VmResult::VM_YIELD;
}

VmResult PoolradEngineHostImpl::clearTextBox() {
    if (!_engine) return VmResult::VM_ERROR;
    _showMessageBox(Common::String(""), true);
    return VmResult::VM_OK;
}

VmResult PoolradEngineHostImpl::loadScript(uint8 scriptID) {
    if (!_engine) return VmResult::VM_ERROR;
    _yieldReason = ECL::YieldReason::WaitingForScript;
    // TODO: Engine should call ECL VM to load new script
    return VmResult::VM_YIELD;
}

VmResult PoolradEngineHostImpl::loadWallSet(uint8 blockId, uint8 setSlot) {
    if (!_engine) return VmResult::VM_ERROR;
    if (setSlot < 1 || setSlot > 3) return VmResult::VM_ERROR;

    PoolradEngine *poolrad = dynamic_cast<PoolradEngine *>(_engine);
    if (!poolrad) return VmResult::VM_ERROR;

    Gfx::WalldefSlotCache &walldefCache = poolrad->getWalldefSlotCache();
    Gfx::Tile8x8Cache &tileCache = poolrad->getTileCache();

    Goldbox::Data::DaxBlock *rawBlock =
            _engine->getDaxManager().getWalldef().getBlockById(blockId);
    if (!rawBlock) {
        warning("PoolradEngineHostImpl::loadWallSet: walldef block %d not found",
                blockId);
        return VmResult::VM_ERROR;
    }
    Goldbox::Data::DaxBlockWalldef *walldef =
            dynamic_cast<Goldbox::Data::DaxBlockWalldef *>(rawBlock);
    if (!walldef) return VmResult::VM_ERROR;

    const int numChunks = walldef->chunkCount();
    for (int i = 0; i < numChunks; ++i) {
        const int curSlot = setSlot + i;
        if (curSlot < 1 || curSlot > 3)
            break;

        walldefCache.loadSlot(curSlot, walldef, i, tileCache);

        const uint16 tileBlockId = static_cast<uint16>(blockId);
        Goldbox::Data::DaxBlock *tileRaw =
                _engine->getDaxManager().get8x8d().getBlockById(
                        static_cast<uint8>(tileBlockId));
        if (tileRaw) {
            Goldbox::Data::DaxBlock8x8D *tile8x8 =
                dynamic_cast<Goldbox::Data::DaxBlock8x8D *>(tileRaw);
            if (tile8x8) {
                const int slotIdx = curSlot - 1;
                _walldefTiles[slotIdx].reset(new Gfx::DaxTile(tile8x8));
                tileCache.setSlot(curSlot, _walldefTiles[slotIdx].get());
            }
        }
    }

    return VmResult::VM_OK;
}

VmResult PoolradEngineHostImpl::loadGeoBlock(uint8 blockId) {
    if (!_engine) return VmResult::VM_ERROR;
    // TODO: Load DaxBlockGeo from _engine->getDaxManager().getGeo()
    return VmResult::VM_OK;
}

void PoolradEngineHostImpl::_showMessageBox(const Common::String &text,
        bool clear) {
    if (!_engine) return;
    // TODO: Route to message box system:
    // - Write text to ECL memory buffer at 0x84c8
    // - Set print-ready flag at 0x84de / output-complete at 0x84df
}

View *PoolradEngineHostImpl::_getCurrentView() const {
    if (!_engine) return nullptr;
    // TODO: Return current active View from engine
    return nullptr;
}

void PoolradEngineHostImpl::_updateViewState() {
    if (!_engine) return;
    // TODO: Call engine to redraw current view after picture/menu/input
}

} // namespace Poolrad
} // namespace Goldbox
