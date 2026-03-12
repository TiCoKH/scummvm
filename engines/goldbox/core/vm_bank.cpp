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

#include "common/endian.h"
#include "common/textconsole.h"
#include "common/memstream.h"
#include "goldbox/core/vm_bank.h"

namespace Goldbox {

int32 VmAddressMapper::toByteOffset(uint16 vmAddr, uint16 firstVmAddr) {
	return static_cast<int32>(vmAddr - firstVmAddr) * 2;
}

uint16 VmAddressMapper::toVmAddr(uint16 firstVmAddr, uint16 wordIndex) {
	return firstVmAddr + wordIndex;
}

int32 VmAddressMapper::toLegacyByteOffset(uint16 vmAddr, int32 legacyBias) {
	return static_cast<int32>(vmAddr) * 2 + legacyBias;
}

uint16 VmAddressMapper::fromLegacyByteOffset(int32 byteOffset, int32 legacyBias) {
	return static_cast<uint16>((byteOffset - legacyBias) / 2);
}

VmWordBank::VmWordBank(uint16 firstVmAddr, uint16 wordCount)
	: _firstVmAddr(firstVmAddr),
	  _wordCount(wordCount),
	  _data(new byte[wordCount * 2]()) {}

VmWordBank::~VmWordBank() {
	delete[] _data;
}

uint16 VmWordBank::lastVmAddr() const {
	return _firstVmAddr + _wordCount - 1;
}

bool VmWordBank::containsWordAddr(uint16 vmAddr) const {
	return vmAddr >= _firstVmAddr && vmAddr <= lastVmAddr();
}

bool VmWordBank::containsByteAddr(uint16 vmAddr) const {
	uint16 lastByteAddr = _firstVmAddr + (_wordCount * 2) - 1;
	return vmAddr >= _firstVmAddr && vmAddr <= lastByteAddr;
}

int32 VmWordBank::byteOffsetFromWordAddr(uint16 vmAddr) const {
	if (!containsWordAddr(vmAddr)) {
		return -1;
	}

	return static_cast<int32>(vmAddr - _firstVmAddr) * 2;
}

uint16 VmWordBank::readWord(uint16 vmAddr) const {
	int32 offset = byteOffsetFromWordAddr(vmAddr);
	if (offset < 0) {
		warning("VmWordBank::readWord out of range addr=0x%04x", vmAddr);
		return 0;
	}

	return READ_LE_UINT16(_data + offset);
}

void VmWordBank::writeWord(uint16 vmAddr, uint16 value) {
	int32 offset = byteOffsetFromWordAddr(vmAddr);
	if (offset < 0) {
		warning("VmWordBank::writeWord out of range addr=0x%04x", vmAddr);
		return;
	}

	WRITE_LE_UINT16(_data + offset, value);
}

uint8 VmWordBank::readByte(uint16 vmAddr) const {
	if (!containsByteAddr(vmAddr)) {
		warning("VmWordBank::readByte out of range addr=0x%04x", vmAddr);
		return 0;
	}

	return _data[vmAddr - _firstVmAddr];
}

void VmWordBank::writeByte(uint16 vmAddr, uint8 value) {
	if (!containsByteAddr(vmAddr)) {
		warning("VmWordBank::writeByte out of range addr=0x%04x", vmAddr);
		return;
	}

	_data[vmAddr - _firstVmAddr] = value;
}

void VmWordBank::reset(uint16 value) {
	for (uint32 i = 0; i < _wordCount; ++i) {
		WRITE_LE_UINT16(_data + i * 2, value);
	}
}

void VmWordBank::sync(Common::Serializer &s) {
	s.syncBytes(_data, byteSize());
}

Common::MemoryReadStream *VmWordBank::openReadStream() const {
	return new Common::MemoryReadStream(_data, byteSize());
}

Common::SeekableMemoryWriteStream *VmWordBank::openWriteStream() {
	return new Common::SeekableMemoryWriteStream(_data, byteSize());
}

VmBankRouter::VmBankRouter() {
	for (int i = 0; i < kVmBankCount; ++i) {
		_banks[i] = nullptr;
	}
}

void VmBankRouter::setBank(VmBankId bankId, VmWordBank *bank) {
	if (bankId < 0 || bankId >= kVmBankCount) {
		warning("VmBankRouter::setBank invalid bank id=%d", static_cast<int>(bankId));
		return;
	}

	_banks[bankId] = bank;
}

VmWordBank *VmBankRouter::getBank(VmBankId bankId) {
	if (bankId < 0 || bankId >= kVmBankCount) {
		return nullptr;
	}

	return _banks[bankId];
}

const VmWordBank *VmBankRouter::getBank(VmBankId bankId) const {
	if (bankId < 0 || bankId >= kVmBankCount) {
		return nullptr;
	}

	return _banks[bankId];
}

bool VmBankRouter::hasBank(VmBankId bankId) const {
	return getBank(bankId) != nullptr;
}

uint16 VmBankRouter::readWord(VmBankId bankId, uint16 vmAddr) const {
	const VmWordBank *bank = getBank(bankId);
	if (!bank) {
		warning("VmBankRouter::readWord missing bank id=%d", static_cast<int>(bankId));
		return 0;
	}

	return bank->readWord(vmAddr);
}

void VmBankRouter::writeWord(VmBankId bankId, uint16 vmAddr, uint16 value) {
	VmWordBank *bank = getBank(bankId);
	if (!bank) {
		warning("VmBankRouter::writeWord missing bank id=%d", static_cast<int>(bankId));
		return;
	}

	bank->writeWord(vmAddr, value);
}

} // namespace Goldbox
