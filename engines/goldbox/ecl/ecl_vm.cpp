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
 * [Offset 0+]:    5 GOTO instructions (opcode 0x01 + ADDR16 operand = 4 bytes each)
 *                 Total header: 20 bytes. These are the entry point addresses
 *                 read by ECL_LoadHeader via VM_GetOprand(1) called five times.
 *                 Format per entry: [01] [01] [lo] [hi]
 *                   - First 01 = GOTO opcode
 *                   - Second 01 = type tag (ADDR16)
 *                   - lo/hi = 16-bit LE target address
 * [After header]: Bytecode stream with opcodes and operands
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
 * Key Opcodes: 0x21 (LOAD_AREA_GEO), 0x37 (LOAD_AREA_WALLDEF), 0x25/0x26 (ON GOTO/GOSUB),
 *              0x0E (PICTURE), 0x24 (COMBAT)
 */

#include "goldbox/ecl/ecl_vm.h"
#include "goldbox/ecl/opcode_handlers.h"
#include "goldbox/ecl/opcode_table.h"
#include "goldbox/ecl/game_config.h"
#include "goldbox/ecl/runtime_layout.h"

namespace Goldbox {
namespace ECL {

namespace {

static uint16 skipEncodedOperandsFromMemory(const AddressSpace &memory,
        uint16 pc, uint8 operandCount) {
    uint16 pos = static_cast<uint16>(pc + 1);

    for (uint8 i = 0; i < operandCount; ++i) {
        const uint8 typeTag = memory.read8(pos++);
        const uint8 lo = memory.read8(pos++);

        switch (typeTag) {
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x81:
            pos++;
            break;
        case 0x80: {
            const uint32 packedSize =
                (static_cast<uint32>(lo) * 3 + 3) / 4;
            pos = static_cast<uint16>(pos + packedSize);
            break;
        }
        default:
            break;
        }
    }

    return pos;
}

} // namespace

EclVM::EclVM(GameConfig *config, SyscallHandler *syscalls)
    : _config(config), _syscalls(syscalls), _memory(config),
            _pc(config ? config->getScriptVmStart()
                    : ECLMemoryLayout::MEM_START_DEFAULT),
            _scriptId(0xFF), _opStartPc(0), _nextInsnPc(0) {
    memset(_opValues, 0, sizeof(_opValues));
    memset(_opTypes, 0, sizeof(_opTypes));
    if (_config) {
        _config->registerDialect();
    }
}

EclVM::~EclVM() {
}

DecodeStatus EclVM::loadProgram(Common::Span<const uint8> program, uint8 scriptId) {
    const uint16 scriptVmStart = _config ? _config->getScriptVmStart()
            : ECLMemoryLayout::MEM_START_DEFAULT;
    // if we like replicate original behavior, sholud need 'Loading...Please Wait' promptMessage() display from here
    _scriptId = scriptId;
    _pc = scriptVmStart;
    _callStack.clear();
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

    return DECODE_OK;
}

bool EclVM::parseECLHeader(Common::Span<const uint8> program) {
    if (!_config)
        return false;

    // Minimum size: 5 encoded operands, each at least 2 bytes (type + lo).
    if (program.size() < kEclHeaderWordCount * 2)
        return false;

    const uint16 scriptVmStart = _config->getScriptVmStart();
    EclLayoutAccess layout = _config->getLayoutAccess();
    static const EclRuntimeFieldId kEntryRuntimeFields[kEclHeaderWordCount] = {
        kEclRuntimeOnMoveEntry,
        kEclRuntimeOnSearchEntry,
        kEclRuntimeOnRestEntry,
        kEclRuntimeOnRestInterruptEntry,
        kEclRuntimeOnInitEntry
    };

    // Original ECL_LoadHeader reads 5 entry points by calling
    // VM_GetOprand(1) five times. Each entry is a full GOTO instruction:
    // [opcode:0x01] [type:0x01] [lo] [hi] = 4 bytes.
    // getOperand() reads from _pc+1 (skipping opcode) and sets _nextInsnPc
    // to the next opcode position.
    _pc = scriptVmStart;
    syncRuntimePc(_pc);

    for (uint i = 0; i < kEclHeaderWordCount; ++i) {
        getOperand(1);
        const uint16 entryPc = getOpWord(1);
        _entryPoints.push_back(entryPc);
        _memory.write16LE(layout.runtimeField(kEntryRuntimeFields[i]),
            entryPc);
        // _nextInsnPc points at the next instruction's opcode.
        _pc = _nextInsnPc;
        syncRuntimePc(_pc);
    }

    return true;
}

void EclVM::initializeECLState() {
    if (!_config) return;

    EclLayoutAccess layout = _config->getLayoutAccess();

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

    // Original ECL_LoadHeader: clear scenario flags (32 bytes) and party
    // flags (10 bytes) only when NOT restoring a saved game.
    if (!stateLoaded) {
        uint16 flagBase = _config->getFlagBase();
        uint16 transientCount = _config->getTransientFlagCount();
        for (uint16 i = 0; i < transientCount; ++i)
            _memory.write8(flagBase + i, 0);

        // Clear party flags (D_PartyFlags[0..9] in original)
        uint16 persistBase = static_cast<uint16>(
            flagBase + transientCount);
        for (uint16 i = 0; i < 10; ++i)
            _memory.write8(persistBase + i, 0);
    } else {
        stateLoaded = false;
    }

    // Mirror GB_EngineMain initial flag state.
    screenRefresh   = true;  // BOOL_SCREEN_REFRESH = true
    geoReady        = false; // BOOL_GEO_READY      = false
    wallsetReady    = false; // BOOL_WALLSET_READY   = false
    eclReady        = false; // BOOL_ECL_READY       = false
    mapdataInload   = false; // BOOL_MAPDATA_INLOAD  = false
    characterInload = false; // BOOL_CHARACTER_INLOAD= false

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
    debug(5, "EclVM::runAtScriptAddress pc=0x%04X maxSteps=%u", scriptPc, maxSteps);
    setPC(scriptPc);
    for (uint32 i = 0; i < maxSteps; ++i) {
        VmResult r = step();
        if (r != VM_OK) {
            debug(5, "EclVM::runAtScriptAddress halted at pc=0x%04X result=%d after %u steps",
                    _pc, (int)r, i + 1);
            return r;
        }
    }
    debug(5, "EclVM::runAtScriptAddress watchdog hit after %u steps at pc=0x%04X", maxSteps, _pc);
    return VM_OK;
}

VmResult EclVM::step() {
    const uint8 opcode = _memory.read8(_pc);
    _opStartPc = _pc;
    _traceBuf.clear();
    VmResult r = executeInstruction(opcode, _pc);
    // Flush trace: getOperand() populates _traceBuf; empty means zero-operand.
    if (_traceBuf.empty()) {
        debug(3, "ECL: 0x%04X  %s()", _opStartPc, getOpcodeName(opcode));
    } else {
        debug(3, "%s", _traceBuf.c_str());
    }
    return r;
}

VmResult EclVM::executeInstruction(uint8 opcode, uint16 currentPc) {
    OpcodeHandler handler = getOpcodeHandler(opcode);
    if (!handler) {
        warning("EclVM: no handler registered for opcode 0x%02X at PC 0x%04X",
                opcode, currentPc);
        return VM_ERROR;
    }

    // _nextInsnPc is set by the handler's getOperand() call.
    // For zero-operand opcodes that don't call getOperand(), default to pc+1.
    _nextInsnPc = static_cast<uint16>(currentPc + 1);
    uint16 nextPc = _nextInsnPc;
    VmResult result = static_cast<VmResult>(handler(*this, _memory, nextPc, _callStack, _syscalls));

    if (result == VM_OK || result == VM_YIELD) {
        // If handler didn't override nextPc, advance past operands.
        if (nextPc == static_cast<uint16>(currentPc + 1))
            nextPc = _nextInsnPc;
        setPC(nextPc);
    }

    return result;
}

void EclVM::getOperand(uint8 opCount) {
    _opStartPc = _pc;
    _opValues[0] = opCount;

    // Original VM_GetOprand reads operands starting at WORD_ECL_PC + 1
    // (skipping the opcode byte at the current PC). After the loop it
    // does WORD_ECL_PC += 1. The net effect is that WORD_ECL_PC ends up
    // pointing at the next instruction's opcode. Our sequential pos++
    // through type/lo/hi bytes achieves the same final position.
    uint16 pos = static_cast<uint16>(_pc + 1);

    if (opCount == 0) {
        _nextInsnPc = pos;
        return;
    }

    assert(opCount <= kMaxOperands);

    for (uint8 i = 1; i <= opCount; i++) {
        const uint8 typeTag = _memory.read8(pos++);
        const uint8 lo      = _memory.read8(pos++);
        _opTypes[i] = typeTag;

        switch (typeTag) {
        case 0x00:
            _opValues[i] = static_cast<uint16>(lo);
            break;
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x81: {
            const uint8 hi = _memory.read8(pos++);
            _opValues[i] = static_cast<uint16>(lo | (hi << 8));
            break;
        }
        case 0x80: {
            const uint32 packedSize = (static_cast<uint32>(lo) * 3 + 3) / 4;
            pos += packedSize;
            _opValues[i] = static_cast<uint16>(lo);
            break;
        }
        default:
            _opValues[i] = 0;
            break;
        }
    }

    _nextInsnPc = pos;
    buildTrace();
}

void EclVM::buildTrace() {
    const uint8 opcode = _memory.read8(_opStartPc);
    const uint8 count = static_cast<uint8>(_opValues[0]);
    const char *name = getOpcodeName(opcode);

    Common::String line = Common::String::format("ECL: 0x%04X  %s(",
        _opStartPc, name);

    bool hasVarargs = false;
    uint8 fixedEnd = count;

    // Detect varargs: ON GOTO/GOSUB (0x25/0x26), HORIZONTAL MENU (0x2B),
    // VERTICAL MENU (0x15) have variable trailing operands.
    if (opcode == 0x25 || opcode == 0x26) {
        fixedEnd = 2;
        hasVarargs = (count > 2);
    } else if (opcode == 0x2B) {
        fixedEnd = 2;
        hasVarargs = (count > 2);
    } else if (opcode == 0x15) {
        fixedEnd = 3;
        hasVarargs = (count > 3);
    }

    for (uint8 i = 1; i <= fixedEnd; ++i) {
        if (i > 1)
            line += ", ";
        const uint8 tag = _opTypes[i];
        const uint16 val = _opValues[i];
        if (tag == 0x80) {
            Common::String s = readString(i);
            if (s.size() > 60) {
                s = Common::String(s.c_str(), 60);
                s += "...";
            }
            line += Common::String::format("%u:\"%s\"", tag, s.c_str());
        } else if (tag == 0x01 || tag == 0x02 || tag == 0x03 || tag == 0x81) {
            line += Common::String::format("%u:0x%04X", tag, val);
        } else {
            line += Common::String::format("%u:%u", tag, val);
        }
    }
    line += ")";

    if (hasVarargs && count > fixedEnd) {
        line += "(";
        for (uint8 i = fixedEnd + 1; i <= count; ++i) {
            if (i > fixedEnd + 1)
                line += ", ";
            const uint8 tag = _opTypes[i];
            const uint16 val = _opValues[i];
            if (tag == 0x80) {
                Common::String s = readString(i);
                if (s.size() > 60) {
                    s = Common::String(s.c_str(), 60);
                    s += "...";
                }
                line += Common::String::format("%u:\"%s\"", tag, s.c_str());
            } else if (tag == 0x01 || tag == 0x02 || tag == 0x03
                    || tag == 0x81) {
                line += Common::String::format("%u:0x%04X", tag, val);
            } else {
                line += Common::String::format("%u:%u", tag, val);
            }
        }
        line += ")";
    }

    _traceBuf = line;
}

uint16 EclVM::getOpWord(uint8 index) const {
    if (index <= kMaxOperands)
        return _opValues[index];
    return 0;
}

uint8 EclVM::getOpType(uint8 index) const {
    if (index <= kMaxOperands)
        return _opTypes[index];
    return 0;
}

uint16 EclVM::readVar(uint8 index) const {
    if (index > kMaxOperands)
        return 0;
    const uint8  tag = _opTypes[index];
    const uint16 val = _opValues[index];
    // ADDR16 types dereference VM memory to get the numeric value.
    if (tag == 0x01 || tag == 0x03)
        return readVmMemory(val);
    return val;
}

Common::String EclVM::readString(uint8 index) const {
    // Re-scan from instruction start to reach operand at 1-based index.
    // Direct memory access — no heap allocation.
    uint16 pos = static_cast<uint16>(_opStartPc + 1);

    for (uint8 i = 1; i <= index; i++) {
        const uint8 typeTag = _memory.read8(pos++);
        const uint8 lo      = _memory.read8(pos++);

        if (i == index) {
            switch (typeTag) {
            case 0x80: {
                const uint8 len = lo;
                const uint32 packedSize = (static_cast<uint32>(len) * 3 + 3) / 4;
                Common::Array<uint8> packed;
                packed.resize(packedSize);
                for (uint32 b = 0; b < packedSize; ++b)
                    packed[b] = _memory.read8(pos++);
                return decompress6BitString(packed.data(), len);
            }
            case 0x81:
            case 0x01:
            case 0x03: {
                const uint8 hi = _memory.read8(pos);
                const uint16 addr = static_cast<uint16>(lo | (hi << 8));
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
                return Common::String();
            }
        }

        // Skip non-target operand.
        switch (typeTag) {
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x81:
            pos++; // skip hi byte
            break;
        case 0x80: {
            const uint32 packedSize = (static_cast<uint32>(lo) * 3 + 3) / 4;
            pos += packedSize;
            break;
        }
        default:
            break;
        }
    }

    return Common::String();
}

void EclVM::setCmpResult(int32 result) {
    if (!_config)
        return;

    EclLayoutAccess layout = _config->getLayoutAccess();
    int8 sign = (result < 0) ? -1 : (result > 0) ? 1 : 0;
    _memory.write8(layout.runtimeField(kEclRuntimeBreakFlag), (uint8)sign);
}

int8 EclVM::getCmpResult() const {
    if (!_config)
        return 0;

    EclLayoutAccess layout = _config->getLayoutAccess();
    return (int8)_memory.read8(layout.runtimeField(kEclRuntimeBreakFlag));
}

uint16 EclVM::skipLegacyInstructionOperands(uint16 instructionPc) const {
    const uint8 opcode = _memory.read8(instructionPc);

    // Poolrad legacy mapping from ECL_SkipInstructionOperands.
    if (opcode == 0x01 || opcode == 0x02 || opcode == 0x0A
            || opcode == 0x0E || opcode == 0x11 || opcode == 0x12
            || opcode == 0x1D || opcode == 0x20 || opcode == 0x2D
            || opcode == 0x32 || opcode == 0x34 || opcode == 0x36
            || opcode == 0x38 || opcode == 0x39 || opcode == 0x3C) {
        return skipEncodedOperandsFromMemory(_memory, instructionPc, 1);
    }

    if (opcode == 0x03 || opcode == 0x08 || opcode == 0x09
            || opcode == 0x0C || opcode == 0x0F || opcode == 0x10
            || opcode == 0x1F || opcode == 0x22) {
        return skipEncodedOperandsFromMemory(_memory, instructionPc, 2);
    }

    if ((opcode >= 0x04 && opcode <= 0x07)
            || opcode == 0x0B || opcode == 0x21 || opcode == 0x28
            || opcode == 0x2A || opcode == 0x2F || opcode == 0x30
            || opcode == 0x35 || opcode == 0x37 || opcode == 0x3B) {
        return skipEncodedOperandsFromMemory(_memory, instructionPc, 3);
    }

    if (opcode == 0x14 || opcode == 0x23)
        return skipEncodedOperandsFromMemory(_memory, instructionPc, 4);
    if (opcode == 0x2E)
        return skipEncodedOperandsFromMemory(_memory, instructionPc, 5);
    if (opcode == 0x1E || opcode == 0x2C)
        return skipEncodedOperandsFromMemory(_memory, instructionPc, 6);
    if (opcode == 0x27)
        return skipEncodedOperandsFromMemory(_memory, instructionPc, 8);
    if (opcode == 0x29)
        return skipEncodedOperandsFromMemory(_memory, instructionPc, 0x0E);

    return static_cast<uint16>(instructionPc + 1);
}

uint8 EclVM::getMemoryRegion(uint16 vmAddr) const {
    if (!_config)
        return 0;

    const Common::Array<MemoryRegionRange> ranges = _config->getMemoryRegions();
    for (uint i = 0; i < ranges.size(); ++i) {
        const MemoryRegionRange &r = ranges[i];
        if (r._startAddr > r._endAddr)
            continue;
        if (vmAddr >= r._startAddr && vmAddr <= r._endAddr)
            return (uint8)i;
    }

    return (uint8)ranges.size();
}

uint16 EclVM::readVmMemory(uint16 vmAddr) const {
    uint16 value = 0;
    if (_config && _config->onReadVmMemory(_memory, vmAddr, value)) {
        return value;
    }

    // Legacy rule: script region reads are byte-wide.
    // This mirrors x86/m68k VM_ReadVar/VM_ReadMemory behavior for bank 3.
    if (getMemoryRegion(vmAddr) == 3)
        return static_cast<uint16>(_memory.read8(vmAddr));

    return _memory.read16LE(vmAddr);
}

uint16 EclVM::readMemory(uint16 vmAddr) const {
    return readVmMemory(vmAddr);
}

void EclVM::onDatBankWrite(uint16 vmAddr, uint16 value,
        SyscallHandler *syscalls) {
    if (!_config)
        return;

    EclLayoutAccess layout = _config->getLayoutAccess();
    const uint16 charBase = _config->getCharacterBase();
    const uint16 localOffset = (uint16)(vmAddr - charBase);

    // Offset 0x0000: character selection changed
    if (localOffset == 0x0000) {
        if (value == 0) {
            uint16 flagAddr = layout.runtimeField(
                kEclRuntimeCharacterRedrawFlag);
            if (EclRuntimeLayout::isValidVmAddr(flagAddr))
                _memory.write8(flagAddr, 1);
        }
        return;
    }

    // Spell memorization range: offsets 0x20..0x70
    if (localOffset >= 0x20 && localOffset <= 0x70) {
        // Written directly to VM memory by caller; no extra side-effect.
        return;
    }

    Data::PlayerCharacter *pc = VmInterface::getSelectedCharacter();
    if (!pc)
        return;

    switch (localOffset) {
    case 0xB8: {
        // NPC index with wrap
        uint16 npcVal = value;
        if (npcVal > 0xB2)
            npcVal -= 0x32;
        // Store low byte into character NPC field via VM memory.
        break;
    }
    case 0x100: {
        // Status field
        if (value > 0x7F) {
            pc->enabled = false;
            if (value == 0x87)
                pc->healthStatus = 7; // S_STONED
        }
        if (value == 0) {
            uint16 flagAddr = layout.runtimeField(
                kEclRuntimeStatusRedrawFlag);
            if (EclRuntimeLayout::isValidVmAddr(flagAddr))
                _memory.write8(flagAddr, 1);
        }
        break;
    }
    case 0x10C: {
        // Combat mode: hostile/quickfight
        if (value == 0) {
            pc->hostile = false;
            pc->quickfight = false;
        } else if (value == 0x80) {
            pc->hostile = false;
            pc->quickfight = true;
        } else if (value == 0x81) {
            pc->hostile = true;
            pc->quickfight = true;
        }
        break;
    }
    case 0x322:
    case 0x324:
    case 0x326: {
        // Wallset loading (slots 1-3)
        if (value <= 0x80)
            break;
        SyscallHandler *active = syscalls ? syscalls : _syscalls;
        if (!active)
            break;
        uint8 slot = (uint8)((localOffset - 0x322) / 2 + 1);
        active->loadWallSet((uint8)(value & 0x7F), slot);
        break;
    }
    default:
        break;
    }
}

void EclVM::writeVmMemory(uint16 vmAddr, uint16 value,
        SyscallHandler *syscalls) {
    SyscallHandler *activeSyscalls = syscalls ? syscalls : _syscalls;
    const uint8 region = getMemoryRegion(vmAddr);

    uint16 writeValue = value;
    if (_config && _config->onWriteVmMemory(_memory, vmAddr, writeValue,
            region, activeSyscalls)) {
        return;
    }

    if (region == 3) {
        _memory.write8(vmAddr, (uint8)writeValue);
        return;
    }

    if (region == 4 && _config) {
        EclLayoutAccess layout = _config->getLayoutAccess();
        uint16 dirAddr = layout.vmGlobalField(
            kVmGlobalFieldDungeonDir).vmAddr;
        if (vmAddr == dirAddr)
            writeValue = (uint16)(writeValue & 0x3);
    }

    _memory.write16LE(vmAddr, writeValue);

    if (region == 0 && _config) {
        EclLayoutAccess layout = _config->getLayoutAccess();
        uint16 skyAddr = layout.vmField(kVmFieldSkyColor).vmAddr;
        uint16 ceilAddr = layout.vmField(kVmFieldCeilingColor).vmAddr;
        if (vmAddr == skyAddr || vmAddr == ceilAddr) {
            uint16 flagAddr = layout.runtimeField(
                kEclRuntimeSkyboxRedrawFlag);
            if (EclRuntimeLayout::isValidVmAddr(flagAddr))
                _memory.write8(flagAddr, 1);
        }
    }

    if (region == 4 && _config) {
        EclLayoutAccess layout = _config->getLayoutAccess();
        uint16 xAddr = layout.vmGlobalField(kVmGlobalFieldDungeonX).vmAddr;
        uint16 yAddr = layout.vmGlobalField(kVmGlobalFieldDungeonY).vmAddr;
        uint16 dirAddr = layout.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr;
        if (vmAddr == xAddr || vmAddr == yAddr || vmAddr == dirAddr) {
            uint16 flagAddr = layout.runtimeField(
                kEclRuntimePositionDirtyFlag);
            if (EclRuntimeLayout::isValidVmAddr(flagAddr))
                _memory.write8(flagAddr, 1);
        }
    }

    if (region == 1)
        onDatBankWrite(vmAddr, writeValue, activeSyscalls);
}

VmResult EclVM::checkMapDataReady() {
    if (screenRefresh && wallsetReady && geoReady) {
        VmResult result = onMapDataReady();
        screenRefresh = false;
        return result;
    }
    return VM_OK;
}

VmResult EclVM::onMapDataReady() {
    if (_syscalls)
        return _syscalls->onMapDataReady();
    return VM_OK;
}

} // namespace ECL
} // namespace Goldbox
