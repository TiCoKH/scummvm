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

#ifndef GOLDBOX_POOLRAD_DATA_POOLRAD_VM_LAYOUT_H
#define GOLDBOX_POOLRAD_DATA_POOLRAD_VM_LAYOUT_H

#include "goldbox/core/vm_layout.h"
#include "goldbox/ecl/runtime_layout.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {

class PoolradVmLayout : public Goldbox::VmLayout {
public:
	const char *layoutName() const override;
	Goldbox::VmFieldLocation field(Goldbox::VmFieldId fieldId) const override;
};

class PoolradGlobalVmLayout : public Goldbox::VmGlobalLayout {
public:
	const char *layoutName() const override;
	Goldbox::VmFieldLocation field(Goldbox::VmGlobalFieldId fieldId) const override;
};

class PoolradScriptFlags {
public:
	static const uint16 kFirstVmAddr = 0x4A00;
	static const uint16 kFlagCount = 0x100;
	static const uint16 kTransientFlagCount = 0x20;

	static bool isScriptFlagAddr(uint16 vmAddr);
	static uint16 flagAddr(uint8 index);
	static bool flagOffset(uint8 index, uint16 &byteOffset);

	// Original behavior: 0x4A00..0x4A1F are reset on new script load.
	static void resetOnScriptLoad(Goldbox::VmWordBank &geoBank);

	// Utility for full clear when needed by engine/game setup.
	static void clearAll(Goldbox::VmWordBank &geoBank);
};

const Goldbox::VmLayout &getPoolradVmLayout();
const Goldbox::VmGlobalLayout &getPoolradGlobalVmLayout();
const Goldbox::ECL::EclRuntimeLayout &getPoolradEclRuntimeLayout();

// Converts any GEO-backed VM address to a bank-relative byte offset.
// This covers both the fixed GEO fields and the script-flag range
// (0x4A00..0x4AFF).
bool poolradGeoVmAddrOffset(uint16 vmAddr, uint16 &byteOffset);

const char *poolradVmFieldName(Goldbox::VmFieldId fieldId);
bool poolradVmFieldOffset(Goldbox::VmFieldId fieldId, uint16 &byteOffset);
void debugDumpPoolradVmLayout();

const char *poolradVmGlobalFieldName(Goldbox::VmGlobalFieldId fieldId);
bool poolradVmGlobalFieldOffset(Goldbox::VmGlobalFieldId fieldId,
	uint16 &byteOffset);
void debugDumpPoolradGlobalVmLayout();

} // namespace Data
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_DATA_POOLRAD_VM_LAYOUT_H
