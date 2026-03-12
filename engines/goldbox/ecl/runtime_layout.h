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

#ifndef GOLDBOX_ECL_RUNTIME_LAYOUT_H
#define GOLDBOX_ECL_RUNTIME_LAYOUT_H

#include "common/scummsys.h"
#include "goldbox/core/vm_layout.h"

namespace Goldbox {
namespace ECL {

enum EclRuntimeFieldId {
	kEclRuntimeBreakFlag = 0,
	kEclRuntimeExitScript,
	kEclRuntimeProgramState,
	kEclRuntimeGameState,
	kEclRuntimeMenuCombatState,
	kEclRuntimeHaltFlag,
	kEclRuntimeSelectedCharPtr,
	kEclRuntimeNextCharPtr,
	kEclRuntimeTextBuffer,
	kEclRuntimeTextPrintFlag,
	kEclRuntimeTextOutputFlag,
	kEclRuntimeMenuStatus,
	kEclRuntimeSpriteState,
	kEclRuntimeUiFlag,
	kEclRuntimeOperandBufferLow,
	kEclRuntimeOperandBufferHigh,
	kEclRuntimeMonsterCount,
	kEclRuntimeMonsterData,
	kEclRuntimeEncounterFlags,
	kEclRuntimeFieldCount
};

class EclRuntimeLayout {
public:
	static const uint16 kInvalidVmAddr = 0xFFFF;

	virtual ~EclRuntimeLayout() {}

	virtual const char *layoutName() const = 0;
	virtual uint16 field(EclRuntimeFieldId fieldId) const = 0;

	bool hasField(EclRuntimeFieldId fieldId) const;
	static bool isValidVmAddr(uint16 vmAddr);
};

class EclLayoutAccess {
public:
	EclLayoutAccess(const Goldbox::VmLayout &vmLayout,
			const Goldbox::VmGlobalLayout &globalLayout,
			const EclRuntimeLayout &runtimeLayout);

	Goldbox::VmFieldLocation vmField(Goldbox::VmFieldId fieldId) const;
	Goldbox::VmFieldLocation vmGlobalField(
		Goldbox::VmGlobalFieldId fieldId) const;
	uint16 runtimeField(EclRuntimeFieldId fieldId) const;

private:
	const Goldbox::VmLayout &_vmLayout;
	const Goldbox::VmGlobalLayout &_globalLayout;
	const EclRuntimeLayout &_runtimeLayout;
};

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_RUNTIME_LAYOUT_H
