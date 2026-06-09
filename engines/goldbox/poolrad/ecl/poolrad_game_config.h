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

#ifndef GOLDBOX_POOLRAD_ECL_POOLRAD_GAME_CONFIG_H
#define GOLDBOX_POOLRAD_ECL_POOLRAD_GAME_CONFIG_H

#include "goldbox/ecl/game_config.h"
#include "goldbox/ecl/opcode_handlers.h"
#include "goldbox/ecl/opcode_table.h"
#include "goldbox/ecl/ecl_memory.h"
#include "goldbox/poolrad/data/poolrad_vm_layout.h"
#include "common/debug.h"

namespace Goldbox {
namespace Poolrad {

namespace {
static const uint16 kPoolradScriptVmStart = 0x9900;
}

/** Pool of Radiance ECL configuration. */
class PoolradGameConfig : public ECL::GameConfig {
public:
    const Goldbox::VmLayout &getVmLayout() const override {
        return Data::getPoolradVmLayout();
    }

    const Goldbox::VmGlobalLayout &getVmGlobalLayout() const override {
        return Data::getPoolradGlobalVmLayout();
    }

    const ECL::EclRuntimeLayout &getEclRuntimeLayout() const override {
        return Data::getPoolradEclRuntimeLayout();
    }

    uint16 getScriptVmStart() const override {
        return kPoolradScriptVmStart;
    }

    bool getVmBankRange(Goldbox::VmBankId bankId, uint16 &firstAddr,
            uint16 &lastAddr) const override {
        switch (bankId) {
        case Goldbox::kVmBankWorld:
            firstAddr = 0x4900;
            lastAddr = 0x4CFF;
            return true;
        case Goldbox::kVmBankDat:
            firstAddr = 0x6B00;
            lastAddr = 0x6EFF;
            return true;
        case Goldbox::kVmBankHeap:
            firstAddr = 0x9700;
            lastAddr = 0x98FF;
            return true;
        case Goldbox::kVmBankEcl:
            firstAddr = kPoolradScriptVmStart;
            lastAddr = 0xB6FF;
            return true;
        default:
            return false;
        }
    }

    Common::String getGameName() const override { return "Pool of Radiance"; }

    void registerDialect() const override {
        ECL::setOpcodeLayout(getVmLayout(), getVmGlobalLayout(),
            getEclRuntimeLayout(), getCharacterBase(), getCharacterSize());
        Goldbox::ECL::registerBaselineOpcodeTable(*this);
        // Core/common opcode handlers own opcode semantics.
        // Poolrad-specific behavior is injected via EclEngineHost callbacks
        // (loadGeoBlock/loadWallSet/loadMonster/etc.) in PoolradEngineHostImpl.
        ECL::registerBaselineOpcodeHandlers();
    }

    /**
     * Intercept writes to DAT walldef trigger addresses.
     *
     * Original decomp (m68k/x86 parity):
     *   DAT+0x322 (vmAddr 0x6C91) -> GFX_LoadWalldef(value & 0x7F, slot=1)
     *   DAT+0x324 (vmAddr 0x6C92) -> GFX_LoadWalldef(value & 0x7F, slot=2)
     *   DAT+0x326 (vmAddr 0x6C93) -> GFX_LoadWalldef(value & 0x7F, slot=3)
     *
     * Triggered only when value > 0x80 (bit 7 acts as "load" flag).
     * The low 7 bits specify the DAX walldef block ID to load.
     */
    bool onWriteVmMemory(ECL::AddressSpace &memory, uint16 vmAddr,
            uint16 &ioValue, uint8 region,
            ECL::SyscallHandler *syscalls) const override {
        // Only intercept DAT bank writes (region determined by address range).
        static const uint16 kWalldefSlot1Addr = 0x6C91; // DAT+0x322
        static const uint16 kWalldefSlot2Addr = 0x6C92; // DAT+0x324
        static const uint16 kWalldefSlot3Addr = 0x6C93; // DAT+0x326

        uint8 slot = 0;
        if (vmAddr == kWalldefSlot1Addr)
            slot = 1;
        else if (vmAddr == kWalldefSlot2Addr)
            slot = 2;
        else if (vmAddr == kWalldefSlot3Addr)
            slot = 3;

        if (slot != 0) {
            // Write the value through to memory first.
            memory.write8(vmAddr, static_cast<uint8>(ioValue & 0xFF));

            // Trigger walldef reload only when bit 7 is set (value > 0x80).
            const uint8 rawValue = static_cast<uint8>(ioValue & 0xFF);
            if (rawValue > 0x80 && syscalls) {
                const uint8 blockId = rawValue & 0x7F;
                debug(3, "PoolradGameConfig::onWriteVmMemory: walldef trigger "
                    "addr=0x%04X slot=%u blockId=%u",
                    (unsigned)vmAddr, (unsigned)slot, (unsigned)blockId);
                syscalls->loadWallSet(blockId, slot);
            }
            return true; // Handled — skip default write path.
        }

        return false;
    }

    bool getOpcodeOperandCount(uint8 opcode, int &count) const override {
        switch (opcode) {
        case 0x0C: count = 3; return true;   // SPRITE_START3
        case 0x1D: count = 1; return true;   // PARTY_STRENGTH
        case 0x1F: count = 0; return true;   // UNUSED_1F
        case 0x21: count = 3; return true;   // LOAD_AREA_MAP
        case 0x22: count = 2; return true;   // PARTY_SKILL_CHECK2
        case 0x23: count = 4; return true;   // SURPRISE
        case 0x27: count = 8; return true;   // TREASURE_MULTICOIN
        case 0x29: count = 14; return true;  // ENCOUNTER_MENU
        case 0x2C: count = 6; return true;   // PARLAY
        case 0x31: count = 0; return true;   // SPRITE_OFF
        case 0x32: count = 1; return true;   // FIND_ITEM
        case 0x34: count = 1; return true;   // CLOCK1 (PoR)
        case 0x37: count = 3; return true;   // LOAD_AREA_DECO
        default:
            break;
        }
        return false;
    }
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_ECL_POOLRAD_GAME_CONFIG_H
