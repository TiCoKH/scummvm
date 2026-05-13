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

#include "common/array.h"
#include "common/scummsys.h"
#include "common/ptr.h"
#include "goldbox/ecl/ecl_syscall_impl.h"
#include "goldbox/ecl/ecl_memory.h"

namespace Goldbox {

class Engine;
class View;

namespace Gfx {
class DaxTile;
}

namespace Poolrad {

namespace Data {
class PoolradCharacter;
}

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
class PoolradEngineHostImpl : public ECL::EclSyscallImpl {
public:
    /**
     * @param engine Reference to main GoldBox engine (must be PoolradEngine)
     * @param memory Reference to ECL virtual memory for reading/writing results
     */
    PoolradEngineHostImpl(::Goldbox::Engine *engine, ECL::AddressSpace *memory);
    ~PoolradEngineHostImpl() override;

    VmResult loadMonster(uint8 monsterId, uint8 count,
        uint8 graphicId) override;
    VmResult clearMonsters() override;
    VmResult displayPicture(uint8 picID) override;
    VmResult loadGeoBlock(uint8 blockId) override;
    VmResult loadIconBlock() override;
    VmResult loadWallSet(uint8 blockId, uint8 setSlot) override;
    VmResult onMapDataReady() override;

private:
    uint8 allocateMonsterIconSlot() const;

    // Owned DaxTile instances for walldef tile atlases (slots 1-3, 0-based idx)
    Common::ScopedPtr<Gfx::DaxTile> _walldefTiles[3];
    Common::Array<Data::PoolradCharacter *> _loadedMonsters;
    Common::Array<uint8> _monsterIconSlots;
    uint8 _nextMonsterIconSlot = 26;
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_ECL_POOLRAD_ENGINE_HOST_IMPL_H
