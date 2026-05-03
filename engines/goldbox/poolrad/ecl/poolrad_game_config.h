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
#include "goldbox/poolrad/ecl/poolrad_opcode_handlers.h"

namespace Goldbox {
namespace Poolrad {

namespace {
static const uint16 kPoolradScriptVmStart = 0x9900;
}

/** Pool of Radiance ECL configuration. */
class PoolradGameConfig : public ECL::GameConfig {
public:
    uint16 getScriptVmStart() const override {
        return kPoolradScriptVmStart;
    }

    bool getVmBankRange(ECL::VmBankId bankId, uint16 &firstAddr,
            uint16 &lastAddr) const override {
        switch (bankId) {
        case ECL::kVmBankGeo:
            firstAddr = 0x4900;
            lastAddr = 0x4CFF;
            return true;
        case ECL::kVmBankDat:
            firstAddr = 0x6B00;
            lastAddr = 0x6EFF;
            return true;
        case ECL::kVmBankHeap:
            firstAddr = 0x9700;
            lastAddr = 0x98FF;
            return true;
        case ECL::kVmBankEcl:
            firstAddr = kPoolradScriptVmStart;
            lastAddr = 0xB6FF;
            return true;
        default:
            return false;
        }
    }

    Common::String getGameName() const override { return "Pool of Radiance"; }

    void registerDialect() const override {
        registerPoolradOpcodeHandlers();
    }
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_ECL_POOLRAD_GAME_CONFIG_H
