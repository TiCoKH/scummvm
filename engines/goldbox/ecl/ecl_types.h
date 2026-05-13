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

#ifndef GOLDBOX_ECL_ECL_TYPES_H
#define GOLDBOX_ECL_ECL_TYPES_H

namespace Goldbox {

/**
 * ECL VM execution result codes.
 */
enum VmResult {
    VM_OK = 0,
    VM_YIELD,
    VM_HALTED,
    VM_ERROR
};

namespace ECL {

/**
 * ECL block entry point selector (from 10-byte header).
 * Each ECL block starts with 5 word addresses (little-endian):
 *   [0-1]:  ON_MOVE           - Main execution entry point
 *   [2-3]:  ON_SEARCH         - Location search handler
 *   [4-5]:  ON_REST           - Pre-camp validation
 *   [6-7]:  ON_REST_INTERRUPT - Camp interruption handler
 *   [8-9]:  ON_INIT           - Initial startup entry
 *
 * Event System Integration:
 *   - Maps reference ECL blocks via 0x21 (LOAD_AREA_GEO) with geoID and block number
 *   - Event numbers (0-127) stored in map cells trigger ECL execution
 *   - ON GOTO/GOSUB (0x25/0x26) dispatch to event subroutines
 */
enum class ECLEntryPoint {
    ON_MOVE           = 0, // Executed during party movement
    ON_SEARCH         = 1, // When searching a location
    ON_REST           = 2, // When party rests
    ON_REST_INTERRUPT = 3, // When rest is interrupted
    ON_INIT           = 4  // Initialization event on map
};

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_ECL_TYPES_H
