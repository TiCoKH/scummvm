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
#include "goldbox/gfx/dax_tile.h"
#include "goldbox/gfx/walldef_surface_builder.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/data/daxblockcontainer.h"

namespace Goldbox {
namespace Poolrad {

PoolradEngineHostImpl::PoolradEngineHostImpl(::Goldbox::Engine *engine,
        ECL::AddressSpace *memory)
    : EclSyscallImpl(engine, memory) {
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

} // namespace Poolrad
} // namespace Goldbox
