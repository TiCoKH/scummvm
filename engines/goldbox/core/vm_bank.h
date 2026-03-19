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

#ifndef GOLDBOX_CORE_VM_BANK_H
#define GOLDBOX_CORE_VM_BANK_H

#include "common/memstream.h"
#include "common/scummsys.h"
#include "common/serializer.h"

namespace Goldbox {

enum VmBankId {
	kVmBankGeo   = 0,
	kVmBankDat   = 1,
	kVmBankHeap  = 2,
	kVmBankEcl   = 3,
	kVmBankSystem = 4,
	kVmBankCount = 5
};

/**
 * Helpers for converting between VM byte addresses and byte offsets.
 *
 * Gold Box VM addresses are byte addresses. Word-sized variables occupy
 * 2 consecutive bytes at even-aligned addresses. These helpers keep the
 * conversion logic in one place.
 */
class VmAddressMapper {
public:
	static int32 toByteOffset(uint16 vmAddr, uint16 firstVmAddr);
	static uint16 toVmAddr(uint16 firstVmAddr, uint16 wordIndex);

	/**
	 * Legacy conversion helper for formulas like:
	 *   byteOffset = vmAddr + legacyBias
	 */
	static int32 toLegacyByteOffset(uint16 vmAddr, int32 legacyBias);
	static uint16 fromLegacyByteOffset(int32 byteOffset, int32 legacyBias);
};

/**
 * Fixed-size word-addressed VM state bank backed by a raw little-endian byte buffer.
 *
 * The buffer holds wordCount words as 2 * wordCount bytes in LE layout.
 * VM addresses are byte addresses: byteOffset = vmAddr - firstVmAddr
 * Word addresses are always even (aligned to 2-byte boundaries).
 *
 * A seekable read/write stream view can be obtained via
 * openReadWriteStream() for code that prefers stream-based access.
 */
class VmWordBank {
public:
	VmWordBank(uint16 firstVmAddr, uint16 wordCount);
	~VmWordBank();

	uint16 firstVmAddr() const { return _firstVmAddr; }
	uint16 wordCount() const { return _wordCount; }
	uint16 byteSize() const { return _wordCount * 2; }
	uint16 lastVmAddr() const;

	bool containsWordAddr(uint16 vmAddr) const;
	bool containsByteAddr(uint16 vmAddr) const;

	uint16 readWord(uint16 vmAddr) const;
	void writeWord(uint16 vmAddr, uint16 value);

	uint8 readByte(uint16 vmAddr) const;
	void writeByte(uint16 vmAddr, uint8 value);

	void reset(uint16 value = 0);
	void sync(Common::Serializer &s);

	/** Raw access to the backing byte buffer (LE layout, 2 bytes per word). */
	byte *getData() { return _data; }
	const byte *getData() const { return _data; }

	/**
	 * Returns a new seekable read/write stream over the entire bank buffer.
	 * Caller takes ownership and must delete the returned object.
	 */
	Common::MemorySeekableReadWriteStream *openReadWriteStream();

private:
	int32 byteOffsetFromWordAddr(uint16 vmAddr) const;

	uint16 _firstVmAddr;
	uint16 _wordCount;
	byte *_data;
};

/**
 * Lightweight holder for VM banks indexed by VmBankId.
 */
class VmBankRouter {
public:
	VmBankRouter();

	void setBank(VmBankId bankId, VmWordBank *bank);
	VmWordBank *getBank(VmBankId bankId);
	const VmWordBank *getBank(VmBankId bankId) const;

	bool hasBank(VmBankId bankId) const;

	uint16 readWord(VmBankId bankId, uint16 vmAddr) const;
	void writeWord(VmBankId bankId, uint16 vmAddr, uint16 value);

private:
	VmWordBank *_banks[kVmBankCount];
};

/**
 * Flat 16-bit VM memory with optional debug bank range metadata.
 *
 * This class is intended for runtime paths that want a single canonical
 * address space and stream-friendly contiguous storage while still being able
 * to classify addresses by legacy bank boundaries for diagnostics.
 */
class VmFlatMemory {
public:
	static const uint32 kMemorySize = 65536;

	VmFlatMemory();
	~VmFlatMemory();

	uint8 readByte(uint16 addr) const;
	void writeByte(uint16 addr, uint8 value);

	uint16 readWordLE(uint16 addr) const;
	void writeWordLE(uint16 addr, uint16 value);

	void clear(uint8 value = 0);
	void sync(Common::Serializer &s);

	byte *getData() { return _data; }
	const byte *getData() const { return _data; }

	Common::MemorySeekableReadWriteStream *openReadWriteStream();

	void setDebugRange(VmBankId bankId, uint16 firstVmAddr, uint16 lastVmAddr);
	bool classifyAddr(uint16 addr, VmBankId &bankId, uint16 &byteOffset) const;

private:
	struct DebugRange {
		bool enabled;
		uint16 firstVmAddr;
		uint16 lastVmAddr;
	};

	byte *_data;
	DebugRange _debugRanges[kVmBankCount];
};

} // namespace Goldbox

#endif // GOLDBOX_CORE_VM_BANK_H
