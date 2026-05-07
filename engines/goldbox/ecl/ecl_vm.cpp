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

/**
 * ECL Script VM Implementation
 *
 * DaxEcl File Structure:
 * ----------------------
 * [Offset 0-9]:    10-byte header with 5 entry point addresses (word each, little-endian)
 * [Offset 10+]:    Bytecode stream with opcodes and operands
 *
 * Memory Layout (Pool of Radiance):
 * ---------------------------------
 * MemStart = 0x9900
 * MemBase = 0x10000 - 0x9900 = 0x6700
 *
 * String Compression (6-bit):
 * ---------------------------
 * Pattern: 0x80 <length> <compressed_data>
 * 3-state decoder packs 4 chars into 3 bytes:
 *   State 1: curr = (thisByte >> 2) & 0x3F
 *   State 2: curr = ((lastByte << 4) | (thisByte >> 4)) & 0x3F
 *   State 3: curr = ((lastByte << 2) | (thisByte >> 6)) & 0x3F
 *   Inflate: if (curr <= 0x1F) curr += 0x40
 *
 * Command Table: 60+ opcodes for game logic
 * Key Opcodes: 0x21 (LOAD FILES), 0x37 (LOAD PIECES), 0x25/0x26 (ON GOTO/GOSUB),
 *              0x0E (PICTURE), 0x24 (COMBAT)
 */

#include "goldbox/ecl/ecl_vm.h"
#include "goldbox/ecl/opcode_handlers.h"
#include "goldbox/ecl/game_config.h"
#include "goldbox/ecl/runtime_layout.h"
#include "common/memstream.h"

namespace Goldbox {
namespace ECL {

EclVM::EclVM(GameConfig *config, SyscallHandler *syscalls)
        : _config(config), _syscalls(syscalls),
            _pc(config ? config->getScriptVmStart()
                    : ECLMemoryLayout::MEM_START_DEFAULT),
            _scriptId(0xFF), _opStartPc(0) {
    if (_config) {
        _config->registerDialect();
    }
}

EclVM::~EclVM() {
}

DecodeStatus EclVM::loadProgram(Common::Span<const uint8> program, uint8 scriptId) {
    const uint16 scriptVmStart = _config ? _config->getScriptVmStart()
            : ECLMemoryLayout::MEM_START_DEFAULT;

    _scriptId = scriptId;
    _pc = scriptVmStart;
    _callStack.clear();
    _program.clear();
    _entryPoints.clear();

    // Mirror original ECL_LoadHeader order: reset runtime state first,
    // then fetch five header words by advancing WORD_ECL_PC from script base.
    initializeECLState();

    // Copy raw ECL bytes into the flat 64K VM memory at the script load address.
    // This is required so that VM-space string and data pointers embedded in
    // operands (type 0x03 / 0x81) resolve to real bytes at runtime.
    _memory.loadBytes(scriptVmStart, program);


    // Parse entry points from header.
    if (!parseECLHeader(program)) {
        return DECODE_OUT_OF_BOUNDS;
    }

    // Decode script body (after 10-byte header) and normalize decoded PCs
    // to VM address space, matching original WORD_ECL_PC addressing.
    Common::Span<const uint8> programBody = program.subspan(kEclHeaderSize);
    DecodeStatus status = decodeProgram(programBody, 0, _program, _config);
    if (status != DECODE_OK) {
        return status;
    }

    const uint16 vmPcBase = static_cast<uint16>(
        scriptVmStart + kEclHeaderSize);
    for (uint i = 0; i < _program.size(); ++i) {
        _program[i].pc = static_cast<uint16>(vmPcBase + _program[i].pc);
    }

    return DECODE_OK;
}

bool EclVM::parseECLHeader(Common::Span<const uint8> program) {
    if (program.size() < kEclHeaderSize) {
        return false;
    }

    const uint16 scriptVmStart = _config ? _config->getScriptVmStart()
            : ECLMemoryLayout::MEM_START_DEFAULT;
        EclLayoutAccess layout = _config->getLayoutAccess();
    static const EclRuntimeFieldId kEntryRuntimeFields[kEclHeaderWordCount] = {
        kEclRuntimeOnMoveEntry,
        kEclRuntimeOnSearchEntry,
        kEclRuntimeOnRestEntry,
        kEclRuntimeOnRestInterruptEntry,
        kEclRuntimeOnInitEntry
    };

    uint16 rawPc = scriptVmStart;
    syncRuntimePc(rawPc);

    // Original loader consumes the header via repeated word fetches from
    // WORD_ECL_PC starting at the script base address.
    for (uint i = 0; i < kEclHeaderWordCount; ++i) {
        const uint16 offset = static_cast<uint16>(i * 2);
        const uint16 entryPc = static_cast<uint16>(program[offset] |
            (program[offset + 1] << 8));
        _entryPoints.push_back(entryPc);
        _memory.write16LE(layout.runtimeField(kEntryRuntimeFields[i]), entryPc);
        rawPc = static_cast<uint16>(rawPc + 2);
        syncRuntimePc(rawPc);
    }

    _pc = rawPc;

    return true;
}

void EclVM::initializeECLState() {
    if (!_config) return;

    EclLayoutAccess layout = _config->getLayoutAccess();

    // Clear transient flags (script-local variables)
    uint16 flagBase = _config->getFlagBase();
    uint16 transientCount = _config->getTransientFlagCount();
    for (uint16 i = 0; i < transientCount; ++i) {
        _memory.write8(flagBase + i, 0);
    }

    // Clear execution control flags
    _memory.write8(layout.runtimeField(kEclRuntimeHaltFlag), 0);
    _memory.write8(layout.runtimeField(kEclRuntimeBreakFlag), 0);
    _memory.write8(layout.runtimeField(kEclRuntimeExitScript), 0);
    _memory.write8(layout.runtimeField(kEclRuntimeProgramState), 0);
    _memory.write16LE(layout.runtimeField(kEclRuntimeOnMoveEntry), 0);
    _memory.write16LE(layout.runtimeField(kEclRuntimeOnSearchEntry), 0);
    _memory.write16LE(layout.runtimeField(kEclRuntimeOnRestEntry), 0);
    _memory.write16LE(layout.runtimeField(kEclRuntimeOnRestInterruptEntry), 0);
    _memory.write16LE(layout.runtimeField(kEclRuntimeOnInitEntry), 0);

    // Set default game state (dungeon)
    _memory.write8(layout.runtimeField(kEclRuntimeGameState), GS_DUNGEON_MAP);
    _memory.write8(layout.vmField(kVmFieldNoMagicFlag).vmAddr, 0);
    _memory.write8(layout.vmField(kVmFieldIndoorModeFlag).vmAddr, 1);

    // Clear character pointers
    _memory.write16LE(layout.runtimeField(kEclRuntimeSelectedCharPtr), 0);
    _memory.write16LE(layout.runtimeField(kEclRuntimeNextCharPtr), 0);

    // Clear menu/combat state
    _memory.write8(layout.runtimeField(kEclRuntimeMenuCombatState), 0);

    // Clear text output flags
    _memory.write8(layout.runtimeField(kEclRuntimeTextPrintFlag), 0);
    _memory.write8(layout.runtimeField(kEclRuntimeTextOutputFlag), 0);

    // Original ECL_LoadHeader reset side effects.
    _memory.write8(layout.vmGlobalField(kVmGlobalFieldPictureHeadId).vmAddr,
        0xFF);
    _memory.write8(layout.vmGlobalField(kVmGlobalFieldRestSafeTime).vmAddr,
        0);
    _memory.write8(
        layout.vmGlobalField(kVmGlobalFieldRestInterruptChance).vmAddr, 0);

    syncRuntimePc(_config->getScriptVmStart());
}

void EclVM::setPC(uint16 pc) {
    _pc = pc;
    syncRuntimePc(pc);
}

void EclVM::syncRuntimePc(uint16 pc) {
    if (!_config)
        return;

    EclLayoutAccess layout = _config->getLayoutAccess();
    _memory.write16LE(layout.runtimeField(kEclRuntimePc), pc);
}

VmResult EclVM::runAtEntryPoint(ECLEntryPoint entry, uint32 maxSteps) {
    int entryIdx = static_cast<int>(entry);
    if (entryIdx < 0 || entryIdx >= 5) {
        return VM_ERROR;
    }

    // Look up entry point offset from parsed header
    if (entryIdx >= (int)_entryPoints.size()) {
        return VM_ERROR;
    }

    return runAtScriptAddress(_entryPoints[entryIdx], maxSteps);
}

VmResult EclVM::runAtScriptAddress(uint16 scriptPc, uint32 maxSteps) {
    if (findInstructionIndexByPc(scriptPc) < 0) {
        return VM_ERROR;
    }

    setPC(scriptPc);
    for (uint32 i = 0; i < maxSteps; ++i) {
        VmResult r = step();
        if (r != VM_OK) {
            return r;
        }
    }
    return VM_OK;
}

VmResult EclVM::step() {
    int insnIndex = findInstructionIndexByPc(_pc);
    if (insnIndex < 0) {
        return VM_HALTED;
    }

    const EclInstruction &insn = _program[insnIndex];
    uint16 defaultNextPc = (insnIndex + 1 < (int)_program.size())
            ? _program[insnIndex + 1].pc
            : static_cast<uint16>(insn.pc + 1);

    return executeInstruction(insn, defaultNextPc);
}

VmResult EclVM::executeInstruction(const EclInstruction &insn,
        uint16 defaultNextPc) {
    OpcodeHandler handler = getOpcodeHandler(insn.opcode);
    if (!handler) {
        warning("EclVM: no handler registered for opcode 0x%02X at PC 0x%04X",
                insn.opcode, insn.pc);
        setPC(defaultNextPc);
        return VM_OK;
    }

    uint16 nextPc = defaultNextPc;
    VmResult result = static_cast<VmResult>(handler(*this, _memory, insn, nextPc, _callStack, _syscalls));

    if (result == VM_OK) {
        setPC(nextPc);
    }

    return result;
}

void EclVM::getOperand(uint8 opCount) {
    _opStartPc = _pc;
    _opValues.resize(opCount + 1);
    _opTypes.resize(opCount + 1);
    _opValues[0] = opCount;
    _opTypes[0]  = 0;

    if (opCount == 0)
        return;

    Common::MemorySeekableReadWriteStream *stream = _memory.openReadWriteStream();
    // Operand bytes start one past the opcode byte.
    stream->seek(static_cast<int64>(_pc + 1), SEEK_SET);

    for (uint8 i = 1; i <= opCount; i++) {
        const uint8 typeTag = stream->readByte();
        const uint8 lo      = stream->readByte();
        _opTypes[i] = typeTag;

        switch (typeTag) {
        case 0x00:
            // VAL8: immediate byte value.
            _opValues[i] = static_cast<uint16>(lo);
            break;
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x81: {
            // ADDR16 / VAL16 / STRING_PTR: type + lo + hi (LE word).
            const uint8 hi = stream->readByte();
            _opValues[i] = static_cast<uint16>(lo | (hi << 8));
            break;
        }
        case 0x80: {
            // STRING_INLINE: type + length + packed bytes.
            // Store length in _opValues; skip packed data.
            const uint32 packedSize = (static_cast<uint32>(lo) * 3 + 3) / 4;
            stream->seek(static_cast<int64>(packedSize), SEEK_CUR);
            _opValues[i] = static_cast<uint16>(lo);
            break;
        }
        default:
            _opValues[i] = 0;
            break;
        }
    }

    delete stream;
}

uint16 EclVM::getOpWord(uint8 index) const {
    if (index < static_cast<uint8>(_opValues.size()))
        return _opValues[index];
    return 0;
}

uint8 EclVM::getOpType(uint8 index) const {
    if (index < static_cast<uint8>(_opTypes.size()))
        return _opTypes[index];
    return 0;
}

uint16 EclVM::readVar(uint8 index) const {
    if (index >= static_cast<uint8>(_opTypes.size()) ||
            index >= static_cast<uint8>(_opValues.size()))
        return 0;
    const uint8  tag = _opTypes[index];
    const uint16 val = _opValues[index];
    // ADDR16 types dereference VM memory to get the numeric value.
    if (tag == 0x01 || tag == 0x03)
        return _memory.read16LE(val);
    return val;
}

Common::String EclVM::readString(uint8 index) const {
    // Re-scan from instruction start to reach operand at 1-based index.
    Common::MemoryReadStream *stream = _memory.openReadStream();
    stream->seek(static_cast<int64>(_opStartPc + 1), SEEK_SET);

    for (uint8 i = 1; i <= index; i++) {
        const uint8 typeTag = stream->readByte();
        const uint8 lo      = stream->readByte();

        if (i == index) {
            switch (typeTag) {
            case 0x80: {
                const uint8 len = lo;
                const uint32 packedSize = (static_cast<uint32>(len) * 3 + 3) / 4;
                Common::Array<uint8> packed;
                packed.resize(packedSize);
                stream->read(packed.data(), packedSize);
                delete stream;
                return decompress6BitString(packed.data(), len);
            }
            case 0x81:
            case 0x01:
            case 0x03: {
                const uint8 hi = stream->readByte();
                const uint16 addr = static_cast<uint16>(lo | (hi << 8));
                delete stream;
                Common::String text;
                uint16 a = addr;
                for (;;) {
                    const uint8 ch = _memory.read8(a++);
                    if (ch == 0) break;
                    text += static_cast<char>(ch);
                }
                return text;
            }
            default:
                delete stream;
                return Common::String();
            }
        }

        // Skip non-target operand.
        switch (typeTag) {
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x81:
            stream->seek(1, SEEK_CUR); // skip hi byte
            break;
        case 0x80: {
            const uint32 packedSize = (static_cast<uint32>(lo) * 3 + 3) / 4;
            stream->seek(static_cast<int64>(packedSize), SEEK_CUR);
            break;
        }
        default:
            break;
        }
    }

    delete stream;
    return Common::String();
}

Common::String EclVM::dumpMemory(uint16 startAddr, uint16 length) const {
    return _memory.dumpRegion(startAddr, length);
}

int EclVM::findInstructionIndexByPc(uint16 scriptPc) const {
    for (uint i = 0; i < _program.size(); ++i) {
        if (_program[i].pc == scriptPc) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

} // namespace ECL
} // namespace Goldbox
