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

#include "goldbox/ecl/runtime_layout.h"

namespace Goldbox {
namespace ECL {

bool EclRuntimeLayout::hasField(EclRuntimeFieldId fieldId) const {
	return isValidVmAddr(field(fieldId));
}

bool EclRuntimeLayout::isValidVmAddr(uint16 vmAddr) {
	return vmAddr != kInvalidVmAddr;
}

EclLayoutAccess::EclLayoutAccess(const Goldbox::VmLayout &vmLayout,
		const Goldbox::VmGlobalLayout &globalLayout,
		const EclRuntimeLayout &runtimeLayout)
	: _vmLayout(vmLayout),
	  _globalLayout(globalLayout),
	  _runtimeLayout(runtimeLayout) {
}

Goldbox::VmFieldLocation EclLayoutAccess::vmField(
		Goldbox::VmFieldId fieldId) const {
	return _vmLayout.field(fieldId);
}

Goldbox::VmFieldLocation EclLayoutAccess::vmGlobalField(
		Goldbox::VmGlobalFieldId fieldId) const {
	return _globalLayout.field(fieldId);
}

uint16 EclLayoutAccess::runtimeField(EclRuntimeFieldId fieldId) const {
	return _runtimeLayout.field(fieldId);
}

} // namespace ECL
} // namespace Goldbox
