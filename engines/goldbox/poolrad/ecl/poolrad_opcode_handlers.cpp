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

#include "goldbox/poolrad/ecl/poolrad_opcode_handlers.h"

namespace Goldbox {
namespace Poolrad {

void registerPoolradOpcodeHandlers() {
    // Intentionally empty.
    //
    // Architecture:
    // - Core opcode handlers in engines/goldbox/ecl/opcode_handlers.cpp
    //   are the single source of opcode semantics.
    // - Game-specific resource behavior is dispatched via SyscallHandler /
    //   EclEngineHost virtual methods (e.g. loadGeoBlock/loadWallSet/loadMonster)
    //   implemented by PoolradEngineHostImpl.
    //
    // Keep this symbol only as a compatibility hook for future dialect-specific
    // exceptions that truly cannot be expressed through host callbacks.
}

} // namespace Poolrad
} // namespace Goldbox
