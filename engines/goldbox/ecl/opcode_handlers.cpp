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

#include "goldbox/ecl/opcode_handlers.h"
#include "goldbox/ecl/ecl_engine_host.h"
#include "goldbox/ecl/ecl_vm.h"
#include "common/hashmap.h"
#include "common/list.h"
#include "common/random.h"
#include "goldbox/vm_interface.h"
#include "goldbox/core/vm_layout.h"
#include "goldbox/ecl/runtime_layout.h"
#include "goldbox/events.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/damage_system.h"
#include "goldbox/data/rules/saving_throw.h"
#include "goldbox/ecl/rune_presentation.h"
#include "goldbox/ecl/ecl_string_writer.h"
#include "goldbox/ecl/scripted_damage.h"
#include "goldbox/ecl/encounter_rules.h"

namespace Goldbox {
namespace ECL {

// Engine-owned rune presentation state shared by PROTECTION (PRINT RUNES),
// CLEAR BOX, PRINT RETURN and EXIT (GB13-008). Lifetime matches the handler
// table; no runtime global constructor beyond the trivially-constructible POD
// state it holds.
static RunePresentationContext &getRunePresentation() {
	static RunePresentationContext s_runeCtx;
	return s_runeCtx;
}

// Use a typedef alias for easier reference
using HandlerMap = Common::HashMap<uint8, OpcodeHandler>;
static HandlerMap g_handlers;

static const Goldbox::VmLayout *g_vmLayout = nullptr;
static const Goldbox::VmGlobalLayout *g_vmGlobalLayout = nullptr;
static const EclRuntimeLayout *g_runtimeLayout = nullptr;
static uint16 g_characterBase = 0;
static uint16 g_characterSize = 0;

Common::RandomSource &getOpcodeRandom() {
    assert(Goldbox::g_engine);
    return Goldbox::g_engine->getRandomSource();
}

void setOpcodeLayout(const Goldbox::VmLayout &vmLayout,
        const Goldbox::VmGlobalLayout &vmGlobalLayout,
        const EclRuntimeLayout &runtimeLayout,
    uint16 characterBase, uint16 characterSize) {
    g_vmLayout = &vmLayout;
    g_vmGlobalLayout = &vmGlobalLayout;
    g_runtimeLayout = &runtimeLayout;
    g_characterBase = characterBase;
    g_characterSize = characterSize;
}

EclLayoutAccess getOpcodeLayout() {
    assert(g_vmLayout);
    assert(g_vmGlobalLayout);
    assert(g_runtimeLayout);
    return EclLayoutAccess(*g_vmLayout, *g_vmGlobalLayout, *g_runtimeLayout);
}

static uint16 getCharacterBlockBase(uint8 index) {
    return static_cast<uint16>(g_characterBase + index * g_characterSize);
}


// Static state for FOR loop (not nested; matches Java VirtualMachine behavior).
static uint16 g_forLoopBodyStart = 0;
static uint16 g_forLoopCount = 0;
static uint16 g_forLoopMax = 0;

// Legacy RANDOM semantics from original engine:
// - source max is treated as a byte
// - for max < 0xFF, range is [0, max]
// - for max == 0xFF, range is [0, 0xFE] (avoids byte overflow on max+1)
static uint8 getLegacyRandomByte(uint16 maxRaw) {
    const uint8 maxByte = static_cast<uint8>(maxRaw & 0xFF);
    const uint16 inclusiveUpper = (maxByte == 0xFF) ? 0xFE : maxByte;
    return static_cast<uint8>(getOpcodeRandom().getRandomNumber(inclusiveUpper));
}

static bool useM68kWriteMemSemantics() {
    if (!Goldbox::g_engine)
        return false;
    return Goldbox::g_engine->getPlatform() == Common::kPlatformAmiga;
}

static bool hasEffectInParty(uint8 effectId) {
    Common::List<Data::PlayerCharacter *> *party = VmInterface::getParty();
    if (!party)
        return false;

    for (Common::List<Data::PlayerCharacter *>::const_iterator it = party->begin(); it != party->end(); ++it) {
        Data::PlayerCharacter *character = *it;
        if (!character)
            continue;

        const Data::Effects::CharacterEffects *effects =
            character->getEffects();
        if (!effects)
            continue;

        const Common::List<Data::Effects::Effect> &list = effects->effects();
        for (Common::List<Data::Effects::Effect>::const_iterator jt =
                list.begin(); jt != list.end(); ++jt) {
            if ((*jt).id == effectId)
                return true;
        }
    }

    return false;
}

static void writeLegacyStringVar(AddressSpace &mem, uint16 destAddr,
        const Common::String &text) {
    // x86 VM_WriteStringVar behavior: copy bytes and append null terminator.
    const uint maxLen = MIN<uint>(text.size(), 0xFF);
    for (uint i = 0; i < maxLen; ++i)
        mem.write8(static_cast<uint16>(destAddr + i),
            static_cast<uint8>(text[i]));
    mem.write8(static_cast<uint16>(destAddr + maxLen), 0);
}

static Common::String getLegacyPrintText(EclVM &vm) {
    // x86 Inst_PRINT_PRCLEAR behavior:
    // - type < 0x80: print numeric operand value converted to decimal string
    // - type >= 0x80: print decoded string operand
    if (vm.getOpType(1) < 0x80)
        return Common::String::format("%u", static_cast<uint>(vm.readVar(1)));
    return vm.readString(1);
}

static int runLegacyPrint(EclVM &vm, SyscallHandler *syscalls,
    bool clearBox, uint8 opcode) {
    if (!syscalls)
        return VM_ERROR;

    vm.getOperand(1);
    const Common::String text = getLegacyPrintText(vm);

    if (clearBox && text.empty()) {
        if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls))
            host->clearTextBox();
        else
            syscalls->printText(text, true);
        syscalls->setTextDelayEnabled(false);
        return VM_OK;
    }

    syscalls->setTextDelayEnabled(true);

    if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls)) {
        const VmResult asyncStart = host->beginPrintAsync(text, clearBox);
        if (asyncStart == VM_YIELD || asyncStart == VM_ERROR) {
            syscalls->setTextDelayEnabled(false);
            return asyncStart;
        }
    }

    // Fallback: synchronous print.
    syscalls->printText(text, clearBox);
    if (g_events) {
        g_events->postEclSyscallMessage(vm.getPC(), opcode,
            EclVmMessage::SC_PRINT, static_cast<int16>(VM_OK));
    }
    syscalls->setTextDelayEnabled(false);
    return VM_OK;
}

// Opcode handlers

static int handle_0x00_EXIT(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    // GB13-008: EXIT ends the script; drop the transient rune presentation
    // cursor/box/redraw state so a later script starts from a clean box.
    getRunePresentation().reset();
    // GB13-006: Inst_EXIT restores the SAVED selection when a temporary
    // LOAD CHARACTER selection is active (BOOL_CHARACTER_INLOAD -> curChar :=
    // savedChar). The engine has no saved-selection pointer modeled yet, and
    // the current selection is already kept coherent (never dangling) by the
    // LOAD CHARACTER removal path. Wiring the savedChar/restoreChar restore
    // belongs with the WHO temporary-selection owner (GB13-003) and PROGRAM
    // selection restore (GB13-007, task 25); do not guess a saved pointer here.
    return VM_HALTED;
}

static int handle_0x01_GOTO(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)callStack; (void)syscalls;
    // VM_GetOprand(1); WORD_ECL_PC = toWord(HI_SAVE[1], LOW_SAVE[1]);
    vm.getOperand(1);
    nextPc = vm.getOpWord(1);
    return VM_OK;
}

static int handle_0x02_GOSUB(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)syscalls;
    vm.getOperand(1);
    callStack.push_back(vm.getNextInsnPc());
    nextPc = vm.getOpWord(1);
    return VM_OK;
}

// 0x03: COMPARE <var1> <var2>
static int handle_0x03_COMPARE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    const uint8 type1 = vm.getOpType(1);
    const uint8 type2 = vm.getOpType(2);

    if (type1 >= 0x80 || type2 >= 0x80) {
        // String compare path: case-insensitive strcmp
        Common::String s1 = vm.readString(1);
        Common::String s2 = vm.readString(2);
        s1.toUppercase();
        s2.toUppercase();
        int cmp = strcmp(s1.c_str(), s2.c_str());
        vm.setCmpResult(cmp);
    } else {
        // Unsigned integer compare
        const uint16 a0 = vm.readVar(1);
        const uint16 a1 = vm.readVar(2);
        int32 result = (a0 < a1) ? -1 : (a0 > a1) ? 1 : 0;
        vm.setCmpResult(result);
    }
    return VM_OK;
}

// TODO: Math opcodes (0x04-0x07) may have different operand order on
// Amiga and PC-98 platforms. Current implementation follows x86 (DOS)
// behavior. When adding Common::Platform support, verify SUB/DIV operand
// order per platform and dispatch accordingly.
static int handle_0x04_ADD(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 var1 = vm.readVar(1);
    const uint16 var2 = vm.readVar(2);
    const uint16 addr = vm.getOpWord(3);
    vm.writeVmMemory(addr, static_cast<uint16>((var1 + var2) & 0xFFFF),
        syscalls);
    return VM_OK;
}

static int handle_0x05_SUBTRACT(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 var1 = vm.readVar(1);
    const uint16 var2 = vm.readVar(2);
    const uint16 addr = vm.getOpWord(3);
    // Pool of Radiance: subtracts var1 from var2.
    vm.writeVmMemory(addr, static_cast<uint16>((var2 - var1) & 0xFFFF),
        syscalls);
    return VM_OK;
}

static int handle_0x06_DIVIDE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 var1 = vm.readVar(1);
    const uint16 var2 = vm.readVar(2);
    const uint16 addr = vm.getOpWord(3);
    if (var2 == 0)
        return VM_ERROR;
    vm.writeVmMemory(addr, static_cast<uint16>(var1 / var2), syscalls);
    return VM_OK;
}

static int handle_0x07_MULTIPLY(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 var1 = vm.readVar(1);
    const uint16 var2 = vm.readVar(2);
    const uint16 addr = vm.getOpWord(3);
    vm.writeVmMemory(addr, static_cast<uint16>((var1 * var2) & 0xFFFF),
        syscalls);
    return VM_OK;
}

static int handle_0x09_SAVE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    vm.getOperand(2);
    const uint16 addr = vm.getOpWord(2);

    if (useM68kWriteMemSemantics()) {
        // m68k INSTR_WriteMem: always numeric, no string branch.
        vm.writeVmMemory(addr, vm.readVar(1), syscalls);
        return VM_OK;
    }

    // x86 Inst_WRITE_MEM:
    // - type < 0x80: numeric write using low byte of operand value
    // - type >= 0x80: write decoded string operand to destination
    if (vm.getOpType(1) < 0x80) {
        const uint16 val = static_cast<uint16>(vm.readVar(1) & 0xFF);
        vm.writeVmMemory(addr, val, syscalls);
    } else {
        writeLegacyStringVar(mem, addr, vm.readString(1));
    }
    return VM_OK;
}

static int handle_0x11_PRINT(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    return runLegacyPrint(vm, syscalls, false, 0x11);
}

static int handle_0x12_PRINTCLEAR(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    return runLegacyPrint(vm, syscalls, true, 0x12);
}

static int handle_0x13_RETURN(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)syscalls;
    // Original x86/m68k behavior: RETURN is a no-op when call stack is empty.
    // Keep nextPc at default (currentPc + 1) and continue execution.
    if (callStack.empty())
        return VM_OK;
    nextPc = callStack.back();
    callStack.pop_back();
    return VM_OK;
}

// 0x2F: AND <var1> <var2> <destAddr>
static int handle_0x2F_AND(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 result = vm.readVar(1) & vm.readVar(2);
    mem.write16LE(vm.getOpWord(3), result);
    vm.setCmpResult(result == 0 ? 0 : 1);
    return VM_OK;
}

// 0x30: OR <var1> <var2> <destAddr>
static int handle_0x30_OR(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 result = vm.readVar(1) | vm.readVar(2);
    mem.write16LE(vm.getOpWord(3), result);
    vm.setCmpResult(result == 0 ? 0 : 1);
    return VM_OK;
}

static int handle_0x08_RANDOM(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    const uint16 maxVal   = vm.readVar(1);
    const uint16 destAddr = vm.getOpWord(2);
    vm.writeVmMemory(destAddr, getLegacyRandomByte(maxVal), syscalls);
    return VM_OK;
}

// Bank cell width for a VM destination address: region 3 (ECL script bank,
// 0x9900+) is byte-addressed; all other word banks store one char per word
// cell. GB13-003 INPUT STRING / WriteGbvmString routing depends on this.
static uint8 vmCellWidthForAddr(EclVM &vm, uint16 addr) {
    return (vm.getMemoryRegion(addr) == 3) ? 1 : 2;
}

static int handle_0x0F_INPUT_NUMBER(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(2);
    const uint8  maxDigits = static_cast<uint8>(vm.getOpWord(1));
    const uint16 addr      = vm.getOpWord(2);

    // GB13-003: prefer the async path so the real input dialog yields the VM.
    // The host owns the result address and writes the unsigned 16-bit value on
    // finalize; invalid/cancelled input writes nothing.
    if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls)) {
        const VmResult asyncStart = host->beginInputNumberAsync(addr, maxDigits);
        if (asyncStart == VM_YIELD || asyncStart == VM_ERROR)
            return asyncStart;
    }

    // Synchronous fallback (non-Poolrad hosts). inputNumber returns -1 when no
    // value was entered; only a non-negative value mutates VM memory. The full
    // 0..65535 domain is only available through the async typed path above;
    // the int16 fallback is limited to 0..32767 and never writes on cancel.
    const int16 value = syscalls->inputNumber(maxDigits);
    if (value >= 0)
        vm.writeVmMemory(addr, static_cast<uint16>(value), syscalls);
    return VM_OK;
}

static int handle_0x10_INPUT_STRING(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(2);
    const uint8  maxLength = static_cast<uint8>(vm.getOpWord(1));
    const uint16 addr      = vm.getOpWord(2);

    // GB13-003: prefer the async path; the host writes the string through the
    // bank-aware VM string writer on finalize.
    if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls)) {
        const VmResult asyncStart = host->beginInputStringAsync(addr, maxLength);
        if (asyncStart == VM_YIELD || asyncStart == VM_ERROR)
            return asyncStart;
    }

    // Synchronous fallback: write through the bank-aware VM string writer
    // (NOT a flat byte memcpy). Cell width follows the destination bank, the
    // length cap honors the $6B00 character-name 15-char limit, and empty
    // input becomes a single space.
    const Common::String result = syscalls->inputString(maxLength);
    const uint8 cap = (addr == kCharacterNameVmAddr)
        ? kCharacterNameMaxLength : kInputStringMaxLength;
    writeVmString(mem, addr, result, vmCellWidthForAddr(vm, addr), cap);
    return VM_OK;
}

// 0x0A: LOAD CHARACTER <var>
static int handle_0x0A_LOAD_CHARACTER(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    const uint8 rawSel = static_cast<uint8>(vm.readVar(1));
    const uint8 selectCount = static_cast<uint8>(rawSel & 0x7F);
    const bool npcFlag = ((rawSel & 0x80) != 0);
    const uint8 partySize = mem.read8(
        getOpcodeLayout().vmGlobalField(kVmGlobalFieldPartyCount).vmAddr);

    // Legacy behavior uses PTR_NEXT_CHARACTER as the traversal start.
    // In the flat VM model, use runtime next-char pointer when valid;
    // otherwise default to party head (index 0).
    uint8 startIndex = 0;
    const uint16 nextCharAddr = mem.read16LE(
        getOpcodeLayout().runtimeField(kEclRuntimeNextCharPtr));
    if (nextCharAddr >= g_characterBase && g_characterSize != 0) {
        const uint16 rel = static_cast<uint16>(nextCharAddr - g_characterBase);
        const uint8 idx = static_cast<uint8>(rel / g_characterSize);
        if (idx < partySize && getCharacterBlockBase(idx) == nextCharAddr)
            startIndex = idx;
    }

    // Walk selectCount entries from startIndex; if out of bounds, leave current
    // selection unchanged (matching original null-pointer guard).
    const uint16 resolvedIndex = static_cast<uint16>(startIndex + selectCount);
    if (resolvedIndex < partySize) {
        const uint8 selectedIndex = static_cast<uint8>(resolvedIndex);
        const uint8 selectedIndexWithFlags = static_cast<uint8>(selectedIndex |
            (npcFlag ? 0x80 : 0x00));
        mem.write8(getOpcodeLayout().vmGlobalField(
            kVmGlobalFieldSelectedPcIndex).vmAddr, selectedIndexWithFlags);
        mem.write16LE(getOpcodeLayout().runtimeField(
            kEclRuntimeSelectedCharPtr), getCharacterBlockBase(selectedIndex));

        Common::List<Data::PlayerCharacter *> *party =
            VmInterface::getParty();
        if (party && selectedIndex < (uint8)party->size()) {
            Common::List<Data::PlayerCharacter *>::const_iterator it = party->begin();
            for (uint8 k = 0; k < selectedIndex; ++k) ++it;
            VmInterface::setSelectedCharacter(*it);
        }
    }

    // GB13-006: Inst_LOAD_CHAR's bit-7 branch is a REMOVAL control, not merely
    // a redraw request. When bit 7 is set and both redraw flags are true, the
    // original runs PARTY_removeMember(true,false) (free the icon slot,
    // decrement PARTY_SIZE), DIALOG_ShowParty, then clears both redraw flags.
    // The removal is the party-member action only; the host performs the
    // stable capture-next-before-erase removal, invalidates pending references
    // for the departed id (Requirement 4.4) and resynchronizes the VM window.
    // It is deliberately distinct from temporary monster teardown
    // (clearMonsters) and disk-character deletion.
    const uint16 statusRedrawAddr = getOpcodeLayout().runtimeField(
        kEclRuntimeStatusRedrawFlag);
    const uint16 charRedrawAddr = getOpcodeLayout().runtimeField(
        kEclRuntimeCharacterRedrawFlag);
    if (npcFlag && EclRuntimeLayout::isValidVmAddr(statusRedrawAddr)
            && EclRuntimeLayout::isValidVmAddr(charRedrawAddr)
            && mem.read8(statusRedrawAddr) != 0
            && mem.read8(charRedrawAddr) != 0) {
        if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls)) {
            Data::PlayerCharacter *departing =
                VmInterface::getSelectedCharacter();
            if (departing)
                host->removeLoadedCharacter(departing);
        }
        mem.write8(statusRedrawAddr, 0);
        mem.write8(charRedrawAddr, 0);
    }
    return VM_OK;
}

// 0x0B: LOAD MONSTER <monsterID> <count> <graphicID>
static int handle_0x0B_LOAD_MONSTER(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint8 monsterId = static_cast<uint8>(vm.readVar(1));
    uint8 count           = static_cast<uint8>(vm.readVar(2));
    const uint8 graphicId = static_cast<uint8>(vm.readVar(3));
    if (count == 0)
        count = 1;

    if (syscalls) {
        EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls);
        if (host) {
            VmResult result = host->loadMonster(monsterId, count, graphicId);
            if (result != VM_OK)
                return result;
        }
    }

    const EclLayoutAccess layout = getOpcodeLayout();
    const uint16 currentCount = mem.read16LE(layout.runtimeField(kEclRuntimeMonsterCount));
    mem.write16LE(layout.runtimeField(kEclRuntimeMonsterCount),
        static_cast<uint16>((currentCount + count) & 0xFFFF));
    mem.write16LE(layout.runtimeField(kEclRuntimeEncounterFlags), monsterId);
    mem.write8(layout.runtimeField(kEclRuntimeMonsterLoadReady), 1);
    return VM_OK;
}

// 0x0C: SPRITE START <ID> <distance> <graphicID>
// Load sprite resource and variant, draw encounter stage with calculated distance
static int handle_0x0C_SPRITE_START(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(3);
    // GB13-008: EclSetupMon reads its three arguments through GetEclVar, so a
    // variable operand is dereferenced to its value rather than treated as a
    // literal address. All three are VALUE operands (see getOperandRole).
    const uint8 resourceId = static_cast<uint8>(vm.readVar(1));
    const uint8 distanceCap = static_cast<uint8>(vm.readVar(2));
    const uint8 variantId = static_cast<uint8>(vm.readVar(3));
    return syscalls->drawEncounterStage(resourceId, distanceCap, variantId);
}

// 0x0D: SPRITE ADVANCE
// Decrement monster distance and redraw encounter stage
static int handle_0x0D_SPRITE_ADVANCE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    uint16 distAddr = getOpcodeLayout().vmGlobalField(kVmGlobalFieldMonsterDistance).vmAddr;
    uint8 dist = mem.read8(distAddr);
    if (dist > 0) {
        dist--;
        mem.write8(distAddr, dist);
        return syscalls->redrawEncounterStage(dist);
    }
    return VM_OK;
}

// 0x0E: PICTURE <pictureID>
// Original behavior (x86 Inst_PICTURE / m68k INSTR_Picture):
// - picId == 0xFF: restore 3D viewport if BOOL_3D_REDRAW or SPRITE_LOAD_FLAG
//   is set, then clear both flags and ARRAY_DRAW_STATE.
// - picId != 0xFF: set BOOL_3D_REDRAW, mark draw state, then draw picture
//   or portrait depending on D_PictureHeadId.
static int handle_0x0E_PICTURE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(1);
    // GB13-008: EclPicture reads picId through GetEclVar (value), so a variable
    // containing the id behaves identically to a literal id.
    const uint8 picId = static_cast<uint8>(vm.readVar(1));

    const EclLayoutAccess layout = getOpcodeLayout();
    const uint16 spriteLoadAddr = layout.runtimeField(kEclRuntimeSpriteState);
    const uint16 skyboxRedrawAddr = layout.runtimeField(kEclRuntimeSkyboxRedrawFlag);

    if (picId == 0xFF) {
        // Always clear the picture display cache.
        VmResult r = syscalls->displayPicture(picId);
        if (r != VM_OK)
            return r;

        // Original guard: only redraw 3D viewport if skybox or sprite was dirty.
        const bool skyboxDirty = EclRuntimeLayout::isValidVmAddr(skyboxRedrawAddr)
            && mem.read8(skyboxRedrawAddr) != 0;
        const bool spriteDirty = EclRuntimeLayout::isValidVmAddr(spriteLoadAddr)
            && mem.read8(spriteLoadAddr) != 0;

        if (skyboxDirty || spriteDirty) {
            if (g_events) {
                g_events->postEclSyscallMessage(vm.getPC(), 0x0E,
                    EclVmMessage::SC_DISPLAY_PICTURE,
                    static_cast<int16>(VM_OK));
            }
            if (EclRuntimeLayout::isValidVmAddr(skyboxRedrawAddr))
                mem.write8(skyboxRedrawAddr, 0);
            if (EclRuntimeLayout::isValidVmAddr(spriteLoadAddr))
                mem.write8(spriteLoadAddr, 0);
        }
        return VM_OK;
    }

    // picId != 0xFF: mark 3D redraw flag and draw picture/portrait.
    // Original x86/m68k: D_PictureHeadId was set by a prior SAVE opcode
    // (e.g. SAVE(headId, 0x6DE1)) to select portrait mode. PICTURE's picId
    // parameter is the body/scene resource ID, NOT the head portrait ID.
    // Do NOT overwrite PictureHeadId here — the script controls it.
    if (EclRuntimeLayout::isValidVmAddr(skyboxRedrawAddr))
        mem.write8(skyboxRedrawAddr, 1);
    const VmResult picResult = syscalls->displayPicture(picId);
    if (g_events) {
        g_events->postEclSyscallMessage(vm.getPC(), 0x0E,
            EclVmMessage::SC_DISPLAY_PICTURE,
            static_cast<int16>(picResult));
    }
    return picResult;
}

static int handle_0x33_PRINT_RETURN(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    // GB13-008: carriage return + line feed in the rune presentation context
    // (EclPrintRet) so the next PROTECTION run starts on a fresh line.
    getRunePresentation().onPrintReturn();
    syscalls->printText(Common::String(), false);
    return VM_OK;
}

// 0x3A: DELAY
// Async delay scaled by host game speed setting (speed * 5 milliseconds)
static int handle_0x3A_DELAY(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    // Host accesses its own CFG_GAME_SPEED configuration
    const VmResult delayResult = syscalls->beginDelay();
    if (g_events) {
        g_events->postEclSyscallMessage(vm.getPC(), 0x3A,
            EclVmMessage::SC_DELAY, static_cast<int16>(delayResult));
    }
    return delayResult;
}

static int handle_0x38_PROGRAM(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(1);
    // GB13-008: EclProgram reads the program id through GetEclVar (value), so a
    // variable containing the id behaves identically to a literal id.
    const VmResult result =
        syscalls->executeProgram(static_cast<uint8>(vm.readVar(1)));
    if (g_events) {
        g_events->postEclSyscallMessage(vm.getPC(), 0x38,
            EclVmMessage::SC_EXECUTE_PROGRAM,
            static_cast<int16>(result));
    }
    return result;
}

static int handle_0x3C_PROTECTION(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(1);
    // GB13-008: operand 1 is the literal glyph source address (not evaluated),
    // matching EclProtection's CombineBytesToWord of the raw operand bytes.
    const uint16 addr = vm.getOpWord(1);
    // Each encoded cell is advanced by one (modulo the byte width) and the run
    // terminates on the decoded terminator; cursor/box/redraw state is carried
    // across the CLEAR BOX -> PROTECTION -> PRINT RETURN / EXIT sequence.
    const Common::String runes = getRunePresentation().decodeRunes(mem, addr);
    syscalls->printText(runes, false);
    if (g_events) {
        g_events->postEclSyscallMessage(vm.getPC(), 0x3C,
            EclVmMessage::SC_PRINT, static_cast<int16>(VM_OK));
    }
    return VM_OK;
}

static int handle_0x3D_CLEAR_BOX(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    // GB13-008: arm the box-clear flag so a following PROTECTION run reflows the
    // cursor if it overflowed the box bottom (EclClearBox).
    getRunePresentation().onClearBox();
    syscalls->clearTextBox();
    if (g_events) {
        g_events->postEclSyscallMessage(vm.getPC(), 0x3D,
            EclVmMessage::SC_CLEAR_TEXTBOX, static_cast<int16>(VM_OK));
    }
    return VM_OK;
}

// 0x14: COMPARE AND <var1> <var2> <var3> <var4>
// Original: calls VM_SetIntCompareFlags twice; result is EQ if BOTH pairs are equal.
// Only EQ/NE flags are meaningful after this opcode (no magnitude comparison).
static int handle_0x14_COMPARE_AND(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(4);
    const uint16 v1 = vm.readVar(1);
    const uint16 v2 = vm.readVar(2);
    const uint16 v3 = vm.readVar(3);
    const uint16 v4 = vm.readVar(4);
    debug(5, "ECL: COMPARE AND: (%u == %u) && (%u == %u)", v1, v2, v3, v4);
    const bool bothEqual = (v1 == v2) && (v3 == v4);
    vm.setCmpResult(bothEqual ? 0 : 1);
    return VM_OK;
}

// 0x15: VERTICAL MENU <address> <message> <count> <stringVarargs>
static int handle_0x15_VERTICAL_MENU(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    // Decode fixed 3 args first to learn count, then re-decode all.
    vm.getOperand(3);
    const uint8 count       = static_cast<uint8>(vm.getOpWord(3));
    vm.getOperand(static_cast<uint8>(3 + count));
    const uint16 resultAddr = vm.getOpWord(1);
    Common::String message  = vm.readString(2);

    Common::Array<Common::String> options;
    for (uint8 i = 0; i < count; ++i)
        options.push_back(vm.readString(static_cast<uint8>(4 + i)));

    // GB13-003: prefer the async path so the real VerticalMenu yields the VM.
    // The host owns resultAddr and writes the selected index on finalize;
    // cancel/quit write nothing (explicit outcomes, not a blanket -1 write).
    if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls)) {
        const VmResult asyncStart =
            host->beginVerticalMenuAsync(resultAddr, message, options);
        if (asyncStart == VM_YIELD || asyncStart == VM_ERROR)
            return asyncStart;
    }

    // Synchronous fallback (non-Poolrad hosts).
    const int16 selection = syscalls->verticalMenu(message, options);
    if (g_events) {
        g_events->postEclSyscallMessage(vm.getPC(), 0x15,
            EclVmMessage::SC_VERTICAL_MENU, selection);
    }
    if (selection >= 0)
        vm.writeVmMemory(resultAddr, static_cast<uint16>(selection),
            syscalls);
    return VM_OK;
}

// 0x16-0x1B: IF commands
static int handleIF(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, uint8 opcode) {
    (void)mem; (void)callStack;
    const int8 cmp = vm.getCmpResult();
    bool cond = false;
    switch (opcode) {
    case 0x16: cond = (cmp == 0); break;
    case 0x17: cond = (cmp != 0); break;
    case 0x18: cond = (cmp <  0); break;
    case 0x19: cond = (cmp >  0); break;
    case 0x1A: cond = (cmp <= 0); break;
    case 0x1B: cond = (cmp >= 0); break;
    }
    if (!cond)
        nextPc = vm.skipLegacyInstructionOperands(nextPc);
    return VM_OK;
}

static int handle_0x16_IF_EQUAL(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)syscalls;
    return handleIF(vm, mem, nextPc, callStack, 0x16);
}

static int handle_0x17_IF_NOT_EQUAL(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)syscalls;
    return handleIF(vm, mem, nextPc, callStack, 0x17);
}

static int handle_0x18_IF_LESS(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)syscalls;
    return handleIF(vm, mem, nextPc, callStack, 0x18);
}

static int handle_0x19_IF_GREATER(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)syscalls;
    return handleIF(vm, mem, nextPc, callStack, 0x19);
}

static int handle_0x1A_IF_LESS_EQUAL(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)syscalls;
    return handleIF(vm, mem, nextPc, callStack, 0x1A);
}

static int handle_0x1B_IF_GREATER_EQUAL(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)syscalls;
    return handleIF(vm, mem, nextPc, callStack, 0x1B);
}

// 0x1C: CLEARMONSTERS
static int handle_0x1C_CLEARMONSTERS(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)nextPc; (void)callStack;
    if (syscalls) {
        EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls);
        if (host) {
            VmResult result = host->clearMonsters();
            if (result != VM_OK)
                return result;
        }
    }
    const EclLayoutAccess layout = getOpcodeLayout();
    mem.write8(layout.runtimeField(kEclRuntimeMonsterCount), 0);
    mem.write8(layout.runtimeField(kEclRuntimeMonsterLoadReady), 0);
    mem.write8(layout.runtimeField(kEclRuntimeMonsterSlotId), 8);
    return VM_OK;
}

// 0x1D: PARTYSTRENGTH <address>
// Calculates a weighted party strength score from HP, class levels, AC, and
// THAC0. AC and THAC0 are stored in 60-offset form (60 - actualValue).
static int handle_0x1D_PARTYSTRENGTH(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)mem;
    vm.getOperand(1);

    uint8 totalScore = 0;
    Common::List<Data::PlayerCharacter *> *party = VmInterface::getParty();
    if (party) {
        for (Common::List<Data::PlayerCharacter *>::const_iterator it = party->begin(); it != party->end(); ++it) {
            Data::ADnDCharacter *ch =
                dynamic_cast<Data::ADnDCharacter *>(*it);
            if (!ch)
                continue;

            const uint8 acRaw = ch->armorClass.current;
            const uint8 acBonus = (acRaw < 61) ? 0 : (acRaw - 60);

            const uint8 thac0Raw = ch->thac0.current;
            const uint8 thac0Bonus = (thac0Raw < 40) ? 0 : (thac0Raw - 39);

            const uint8 charScore = (
                ch->hitPoints.current * 1
                + ch->levels[Data::C_CLERIC] * 4
                + ch->levels[Data::C_MAGICUSER] * 8
                + acBonus * 5
                + thac0Bonus * 5
            ) / 10;

            totalScore += charScore;
        }
    }

    vm.writeVmMemory(vm.getOpWord(1), totalScore, syscalls);
    return VM_OK;
}

// 0x1E: CHECKPARTY <attributeAddress> <effectID> <unknown> <address1> <unknown> <address2>
static int handle_0x1E_CHECKPARTY(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    vm.getOperand(6);
    const uint16 attributeAddr = vm.readVar(1);
    const uint16 effectID      = vm.readVar(2);
    const uint16 addr1         = vm.getOpWord(4);
    const uint16 addr2         = vm.getOpWord(6);

    if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls)) {
        const VmResult result = host->checkParty(attributeAddr, effectID,
            addr1, addr2);
        if (result != VM_OK)
            return result;
        return VM_OK;
    }

    if (attributeAddr != 0 && effectID == 0) {
        vm.writeVmMemory(addr1, 18, syscalls); // Placeholder highest
        vm.writeVmMemory(addr2, 3, syscalls);  // Placeholder lowest
    } else if (attributeAddr == 0 && effectID != 0) {
        vm.writeVmMemory(addr2, 0, syscalls); // Placeholder: no effect
    }
    return VM_OK;
}

// 0x1F: UNDEFINED (no-op)
static int handle_0x1F_UNDEFINED(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x20: NEWECL <script>
// Original INSTR_NewECL behavior:
//   G_SaveEclId = BYTE_ECL_SCRIPT_ID;
//   BYTE_ECL_SCRIPT_ID = new_ecl_id;
//   ECL_LoadBlock(new_ecl_id);
//   ECL_LoadHeader();
//   BOOL_MAPDATA_INLOAD = false;
//   BOOL_ECL_READY = true;
//   ECL_EXIT = true;
static int handle_0x20_NEWECL(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(1);
    const uint8 newScriptId = static_cast<uint8>(vm.getOpWord(1));

    debug(3, "ECL: 0x%04X  NEWECL(0:%u)", vm.getPC(), (unsigned)newScriptId);

    // Update G_SaveEclId to the new active script ID.
    // Original: BYTE_ECL_SCRIPT_ID = new_ecl_id, then ENGINE_Run writes
    // G_SaveEclId = BYTE_ECL_SCRIPT_ID. We do it here directly.
    const EclLayoutAccess layout = getOpcodeLayout();
    mem.write8(layout.vmField(kVmFieldSavedEclId).vmAddr, newScriptId);

    // Load new ECL block into VM memory (host zeroes VMBANK3, copies bytes).
    const VmResult loadResult = syscalls->loadScript(newScriptId);
    if (g_events) {
        g_events->postEclSyscallMessage(vm.getPC(), 0x20,
            EclVmMessage::SC_LOAD_SCRIPT,
            static_cast<int16>(loadResult));
    }
    if (loadResult == VM_ERROR)
        return VM_ERROR;

    // Parse header from newly loaded bytecode (extracts entry points,
    // clears call stack). Does NOT reset geo/wallset/screenRefresh flags
    // since the sub-script shares the same map context.
    vm.reloadHeader(newScriptId);

    // Mirror original: set flags and halt current execution loop.
    // The engine dispatches ON_INIT via getEntryPointPc(4) when it
    // sees eclReady == true after the run loop exits.
    vm.mapdataInload = false;
    vm.eclReady = true;
    return VM_HALTED;
}

// 0x21: LOAD_AREA_GEO <geoBlockId> <unused> <iconTrigger>
static int handle_0x21_LOAD_AREA_GEO(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(3);
    const uint8 geoBlockId  = static_cast<uint8>(vm.readVar(1));
    const uint8 iconTrigger = static_cast<uint8>(vm.readVar(3));
    const bool  indoorMode  = (mem.read8(
        getOpcodeLayout().vmField(kVmFieldIndoorModeFlag).vmAddr) != 0);

    if (geoBlockId != 0xFF && geoBlockId != 0x7F && indoorMode) {
        mem.write8(getOpcodeLayout().vmField(kVmFieldGeoBlockId).vmAddr, geoBlockId);
        VmResult geoResult = syscalls->loadGeoBlock(geoBlockId);
        if (g_events) {
            g_events->postEclSyscallMessage(vm.getPC(), 0x21,
                EclVmMessage::SC_LOAD_GEO,
                static_cast<int16>(geoResult));
        }
        if (geoResult != VM_OK)
            return geoResult;
        mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldMovementBlock).vmAddr, 0);
    }
    if (iconTrigger != 0xFF && !indoorMode) {
        VmResult iconResult = syscalls->loadIconBlock();
        if (g_events) {
            g_events->postEclSyscallMessage(vm.getPC(), 0x21,
                EclVmMessage::SC_LOAD_ICON,
                static_cast<int16>(iconResult));
        }
        if (iconResult != VM_OK)
            return iconResult;
    }
    vm.geoReady = true;
    return vm.checkMapDataReady();
}

// 0x22: PARTY SURPRISE <address1> <address2>
// GB13-002: port EclPartySurprise (eclvm.pas:784). Scans the party for a ranger
// or cleric-ranger with NO status filter (the original has no status check, so
// an incapacitated/dead ranger still counts). Writes mod1 (1 when a ranger is
// present, else 0) to operand 1 and mod2 (always 0) to operand 2 — the 1/0 or
// 0/0 pair, replacing the old fixed 2/2. The ranger predicate lives in the pure
// encounter_rules core so it is unit-provable without a live party.
static int handle_0x22_PARTY_SURPRISE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    vm.getOperand(2);
    const uint16 addr1 = vm.getOpWord(1);
    const uint16 addr2 = vm.getOpWord(2);

    Common::Array<uint8> classIds;
    if (Common::List<Data::PlayerCharacter *> *party = VmInterface::getParty()) {
        for (Common::List<Data::PlayerCharacter *>::const_iterator it =
                party->begin(); it != party->end(); ++it) {
            if (*it)
                classIds.push_back((*it)->classType);
        }
    }

    const PartySurpriseResult result = resolvePartySurprise(classIds);
    vm.writeVmMemory(addr1, result.mod1, syscalls);
    vm.writeVmMemory(addr2, result.mod2, syscalls);
    return VM_OK;
}

// GB13-002: live two-d6 source for the surprise roll. Both rolls go through the
// engine RNG (FxEngine.RollDice(1, 6) parity); the deterministic draw order and
// the both-success overwrite live entirely in resolveSurprise(), which the
// tests drive with a recorded source.
class LiveSurpriseRollSource : public ISurpriseRollSource {
public:
    uint8 rollD6() override {
        const int r = VmInterface::rollDice(1, 6);
        return (r < 1) ? 1 : static_cast<uint8>(r);
    }
};

// 0x23: SURPRISE <var1> <var2> <var3> <var4>
// GB13-002: port EclSurprise (eclvm.pas:814). All four operands are value
// operands (GetEclVar). The signed threshold arithmetic, the exact r1-then-r2
// draw order and the ORIGINAL both-success overwrite (a double success of 3 is
// overwritten with 2) live in the pure encounter_rules core so they are
// unit-provable; the result is written to the ambush global, matching the
// original WriteGbvm(C_Gbvm.SURPRISE, result) — it does NOT write the operand
// addresses.
static int handle_0x23_SURPRISE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(4);

    SurpriseOperands ops;
    for (uint8 i = 0; i < 4; ++i)
        ops.v[i] = static_cast<uint8>(vm.readVar(static_cast<uint8>(i + 1)));

    LiveSurpriseRollSource source;
    const uint8 result = resolveSurprise(ops, source);

    mem.write8(getOpcodeLayout().vmGlobalField(
        kVmGlobalFieldCombatIsAmbush).vmAddr, result);
    return VM_OK;
}

// Shared post-encounter cleanup tail for opcode 0x24 (GB13-009). Runs inline on
// the non-yield path and once on the resume path (host finalize) so both paths
// perform byte-identical mutations. Post-encounter: reset game state from the
// indoor-mode flag, clear search flags (keep bit 0), clear the skybox redraw.
void runEncounterCleanupTail(AddressSpace &mem) {
    const EclLayoutAccess layout = getOpcodeLayout();

    // Reset game state based on map type.
    const uint8 indoorMode = mem.read8(
        layout.vmField(kVmFieldIndoorModeFlag).vmAddr);
    mem.write8(layout.runtimeField(kEclRuntimeGameState),
        indoorMode == 1 ? 1 : 0);

    // Clear search flags (keep only bit 0).
    const uint16 searchAddr = layout.vmGlobalField(
        kVmGlobalFieldSearchFlags).vmAddr;
    mem.write8(searchAddr,
        static_cast<uint8>(mem.read8(searchAddr) & 1));

    // Clear draw state and skybox redraw flag.
    const uint16 skyboxAddr = layout.runtimeField(
        kEclRuntimeSkyboxRedrawFlag);
    if (EclRuntimeLayout::isValidVmAddr(skyboxAddr))
        mem.write8(skyboxAddr, 0);
}

// 0x24: ENCOUNTER
// Primary encounter dispatcher. Branch logic:
// - If monsters are loaded (MONST_LOAD_READY or COMBAT_TRIGGER): run combat
// - Else if ShopFlag: open shop dialog
// - Else if TemplePending: open temple dialog
// - Else: just run battle-end cleanup (no-op encounter)
// Post-encounter cleanup runs via runEncounterCleanupTail(): inline on the
// non-yield path, or in the host finalize seam on the yield->resume path.
// ECL scripts call CLEARMONSTERS (0x1C) before this opcode to ensure the
// else-branch (shop/temple) is taken instead of combat.
static int handle_0x24_ENCOUNTER(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    const EclLayoutAccess layout = getOpcodeLayout();
    EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls);

    const uint8 monsterReady = mem.read8(
        layout.runtimeField(kEclRuntimeMonsterLoadReady));
    const uint8 combatTrigger = mem.read8(
        layout.runtimeField(kEclRuntimeMenuCombatState));

    if (monsterReady || combatTrigger) {
        // Combat path
        const VmResult combatResult = syscalls->startCombat();
        if (g_events) {
            g_events->postEclSyscallMessage(vm.getPC(), 0x24,
                EclVmMessage::SC_START_COMBAT,
                static_cast<int16>(combatResult));
        }
        if (combatResult != VM_OK)
            return combatResult;

        if (combatTrigger)
            mem.write8(layout.runtimeField(kEclRuntimeMenuCombatState), 0);
    } else {
        // Non-combat path: shop, temple, or empty encounter
        const uint8 shopFlag = mem.read8(
            layout.vmGlobalField(kVmGlobalFieldShopFlag).vmAddr);
        const uint8 templeFlag = mem.read8(
            layout.vmGlobalField(kVmGlobalFieldEnterTempleFlag).vmAddr);

        if (shopFlag == 1) {
            mem.write8(layout.vmGlobalField(kVmGlobalFieldShopFlag).vmAddr, 0);
            if (host) {
                VmResult r = host->enterShop();
                if (r != VM_OK)
                    return r;
            }
        } else if (templeFlag == 1) {
            mem.write8(layout.vmGlobalField(
                kVmGlobalFieldEnterTempleFlag).vmAddr, 0);
            if (host) {
                VmResult r = host->enterTemple();
                if (r != VM_OK)
                    return r;
            }
        }
    }

    // GB13-009: the post-encounter cleanup tail. This runs here ONLY on the
    // non-yield (VM_OK) path — the empty/no-monster encounter and the
    // shop/temple branches whose host call returned VM_OK. When the call
    // yields (combat view, or an async shop/temple dialog) the handler returns
    // early above and the host runs runEncounterCleanupTail() exactly once
    // during finalizePendingAsync(), before the VM fetches the resume PC.
    runEncounterCleanupTail(mem);

    return VM_OK;
}

// 0x25: ON GOTO <var> <count> <addressVarargs>
static int handle_0x25_ON_GOTO(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)callStack; (void)syscalls;
    // First pass: read var and count.
    vm.getOperand(2);
    const uint8 selector = static_cast<uint8>(vm.readVar(1));
    const uint8 count = static_cast<uint8>(vm.readVar(2));
    static const uint8 kVmMaxOperands = 16;

    // Fast path: full decode fits VM operand scratch buffer.
    if (static_cast<uint16>(2 + count) <= kVmMaxOperands) {
        vm.getOperand(static_cast<uint8>(2 + count));
        // Fall-through after full ON GOTO instruction if selector is out of range.
        nextPc = vm.getNextInsnPc();
        if (selector < count)
            nextPc = vm.getOpWord(static_cast<uint8>(3 + selector));
        return VM_OK;
    }

    // Slow path: count exceeds fixed decode buffer; scan encoded operands
    // directly to compute full instruction end and selected jump target.
    uint16 pos = static_cast<uint16>(vm.getPC() + 1); // skip opcode
    uint16 targetPc = 0;
    const uint16 targetIndex = static_cast<uint16>(3 + selector);
    const uint16 totalOperands = static_cast<uint16>(2 + count);

    for (uint16 i = 1; i <= totalOperands; ++i) {
        const uint8 typeTag = mem.read8(pos++);
        const uint8 lo = mem.read8(pos++);
        uint16 value = lo;

        switch (typeTag) {
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x81: {
            const uint8 hi = mem.read8(pos++);
            value = static_cast<uint16>(lo | (hi << 8));
            break;
        }
        case 0x80:
            pos = static_cast<uint16>(pos + lo);
            break;
        default:
            break;
        }

        if (i == targetIndex)
            targetPc = value;
    }

    // Default: continue after full instruction; ON GOTO overrides when valid.
    nextPc = pos;
    if (selector < count)
        nextPc = targetPc;
    return VM_OK;
}

// 0x26: ON GOSUB <var> <count> <addressVarargs>
static int handle_0x26_ON_GOSUB(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)syscalls;
    // First pass: read var and count.
    vm.getOperand(2);
    const uint8 selector = static_cast<uint8>(vm.readVar(1));
    const uint8 count = static_cast<uint8>(vm.readVar(2));
    static const uint8 kVmMaxOperands = 16;

    // Fast path: full decode fits VM operand scratch buffer.
    if (static_cast<uint16>(2 + count) <= kVmMaxOperands) {
        vm.getOperand(static_cast<uint8>(2 + count));
        const uint16 returnPc = vm.getNextInsnPc();
        nextPc = returnPc;
        if (selector < count) {
            callStack.push_back(returnPc);
            nextPc = vm.getOpWord(static_cast<uint8>(3 + selector));
        }
        return VM_OK;
    }

    // Slow path: count exceeds fixed decode buffer; scan encoded operands
    // directly to compute full instruction end and selected jump target.
    uint16 pos = static_cast<uint16>(vm.getPC() + 1); // skip opcode
    uint16 targetPc = 0;
    const uint16 targetIndex = static_cast<uint16>(3 + selector);
    const uint16 totalOperands = static_cast<uint16>(2 + count);

    for (uint16 i = 1; i <= totalOperands; ++i) {
        const uint8 typeTag = mem.read8(pos++);
        const uint8 lo = mem.read8(pos++);
        uint16 value = lo;

        switch (typeTag) {
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x81: {
            const uint8 hi = mem.read8(pos++);
            value = static_cast<uint16>(lo | (hi << 8));
            break;
        }
        case 0x80:
            pos = static_cast<uint16>(pos + lo);
            break;
        default:
            break;
        }

        if (i == targetIndex)
            targetPc = value;
    }

    const uint16 returnPc = pos;
    nextPc = returnPc;
    if (selector < count) {
        callStack.push_back(returnPc);
        nextPc = targetPc;
    }
    return VM_OK;
}

// 0x27: TREASURE <copper> <silver> <electrum> <gold> <platinum> <gems> <jewelry> <treasureID>
// Loads treasure/item data into the shop item pool. Does NOT open a dialog.
// The dialog is opened by ENCOUNTER (0x24) when ShopFlag is set.
static int handle_0x27_TREASURE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    vm.getOperand(8);

    EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls);
    if (!host)
        return VM_OK;

    const uint8 copper    = static_cast<uint8>(vm.readVar(1));
    const uint8 silver    = static_cast<uint8>(vm.readVar(2));
    const uint8 electrum  = static_cast<uint8>(vm.readVar(3));
    const uint8 gold      = static_cast<uint8>(vm.readVar(4));
    const uint8 platinum  = static_cast<uint8>(vm.readVar(5));
    const uint8 gems      = static_cast<uint8>(vm.readVar(6));
    const uint8 jewelry   = static_cast<uint8>(vm.readVar(7));
    const uint8 itemSetId = static_cast<uint8>(vm.readVar(8));

    return host->setupTreasure(copper, silver, electrum, gold,
        platinum, gems, jewelry, itemSetId);
}

// 0x28: ROB <isWholeParty> <percentMoney> <itemChance>
// Original EclRob (eclvm.pas:1027) loads three operands but reads operand 2
// for BOTH the currency percentage and the item-theft probability; operand 3
// is decoded for framing only and left unused. Decoding all three gives the
// correct next PC (getNextInsnPc).
static int handle_0x28_ROB(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    vm.getOperand(3);

    const uint8 wholeParty   = static_cast<uint8>(vm.readVar(1));
    const uint8 percentMoney = static_cast<uint8>(vm.readVar(2));
    const uint8 itemChance   = percentMoney; // operand 2 drives item theft too

    if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls))
        return host->robParty(wholeParty, percentMoney, itemChance);

    return VM_OK;
}

// 0x29: ENCOUNTER MENU (14 operands)
// Original EclEncounter (eclvm.pas:1057) loads fourteen operands:
//   1 monster id, 2 requested distance, 3 monster picture id,
//   4 result destination address, 5..9 response table entries,
//   10..12 three message strings, 13 party-flee threshold, 14 monster-flee
//   threshold. GB13-001 frames all fourteen so the VM resumes at the correct
//   PC; the encounter decision loop itself is implemented under GB13-002.
static int handle_0x29_ENCOUNTER_MENU(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    vm.getOperand(14);

    // Decode all fourteen operand roles (GB13-001 framing / GB13-008 roles):
    //   1  monster id        (value)
    //   2  requested distance (value)
    //   3  monster picture id (value)
    //   4  destination        (literal VM address word)
    //   5..9  response-table values (value)
    //   10..12 three message strings (packed strings)
    //   13 party-flee threshold (value)
    //   14 monster-flee threshold (value)
    const uint8 monsterId     = static_cast<uint8>(vm.readVar(1));
    const uint8 requestedDist = static_cast<uint8>(vm.readVar(2));
    const uint8 monsterPicId  = static_cast<uint8>(vm.readVar(3));
    const uint16 destAddr     = vm.getOpWord(4);
    const uint8 fleePar       = static_cast<uint8>(vm.readVar(13));
    const uint8 fleeMon       = static_cast<uint8>(vm.readVar(14));

    // Pack the decoded request into an owned byte array for the host, using a
    // fixed, documented layout. The host (PoolradEngineHostImpl::encounterMenu)
    // reconstructs the owned EncounterOperation from this, so the ECL layer
    // never passes a pointer into decoder scratch and never depends on poolrad.
    //
    //   [0]    monster id
    //   [1]    requested distance
    //   [2]    monster picture id
    //   [3]    destination address low byte
    //   [4]    destination address high byte
    //   [5..9] response-table values (operands 5..9)
    //   [10]   party-flee threshold
    //   [11]   monster-flee threshold
    //   [12]   message-string count that follow (always 3)
    // followed by the three NUL-terminated message strings.
    Common::Array<uint8> operands;
    operands.push_back(monsterId);
    operands.push_back(requestedDist);
    operands.push_back(monsterPicId);
    operands.push_back(static_cast<uint8>(destAddr & 0xFF));
    operands.push_back(static_cast<uint8>((destAddr >> 8) & 0xFF));
    for (uint8 i = 0; i < 5; ++i)
        operands.push_back(static_cast<uint8>(vm.readVar(static_cast<uint8>(5 + i))));
    operands.push_back(fleePar);
    operands.push_back(fleeMon);
    operands.push_back(3);
    for (uint8 i = 0; i < 3; ++i) {
        const Common::String msg = vm.readString(static_cast<uint8>(10 + i));
        for (uint j = 0; j < msg.size(); ++j)
            operands.push_back(static_cast<uint8>(msg[j]));
        operands.push_back(0);
    }

    if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls)) {
        const VmResult result = host->encounterMenu(operands);
        if (g_events) {
            g_events->postEclSyscallMessage(vm.getPC(), 0x29,
                EclVmMessage::SC_ENCOUNTER_MENU,
                static_cast<int16>(result));
        }
        return result;
    }

    if (g_events) {
        g_events->postEclSyscallMessage(vm.getPC(), 0x29,
            static_cast<uint16>(EclVmMessage::SC_ENCOUNTER_MENU),
            static_cast<int16>(VM_OK));
    }
    return VM_OK;
}

// 0x2A: COPY MEM <baseAddr> <offsetVar> <destAddr>
static int handle_0x2A_COPY_MEM(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    vm.getOperand(3);
    const uint16 baseAddr  = vm.getOpWord(1);
    const uint8 offset     = static_cast<uint8>(vm.readVar(2));
    const uint16 srcAddr   = static_cast<uint16>(baseAddr + offset);
    const uint16 destAddr  = vm.getOpWord(3);
    const uint16 value     = vm.readMemory(srcAddr);
    vm.writeVmMemory(destAddr, value, syscalls);
    return VM_OK;
}

// 0x2B: HORIZONTAL MENU <address> <count> <stringVarargs>
static int handle_0x2B_HORIZONTAL_MENU(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(2);
    const uint8  count      = static_cast<uint8>(vm.getOpWord(2));
    vm.getOperand(static_cast<uint8>(2 + count));
    const uint16 resultAddr = vm.getOpWord(1);

    Common::Array<Common::String> options;
    for (uint8 i = 0; i < count; ++i)
        options.push_back(vm.readString(static_cast<uint8>(3 + i)));

    // Preferred path: host starts menu asynchronously and VM yields.
    if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls)) {
        const VmResult asyncStart =
            host->beginHorizontalMenuAsync(resultAddr, options);
        if (g_events) {
            g_events->postEclSyscallMessage(vm.getPC(), 0x2B,
                EclVmMessage::SC_HORIZONTAL_MENU,
                static_cast<int16>(asyncStart));
        }
        if (asyncStart == VM_YIELD || asyncStart == VM_ERROR)
            return asyncStart;
    }

    const int16 selection = syscalls->horizontalMenu(options);
    if (g_events) {
        g_events->postEclSyscallMessage(vm.getPC(), 0x2B,
            EclVmMessage::SC_HORIZONTAL_MENU, selection);
    }
    if (selection >= 0)
        vm.writeVmMemory(resultAddr, static_cast<uint16>(selection),
            syscalls);
    return VM_OK;
}

// 0x2C: PARLAY <haughty> <sly> <nice> <meek> <abusive> <address>
// GB13-002: port EclParlay (eclvm.pas:1250). Operands 1..5 are value operands
// carrying the attitude result value for each menu entry (HAUGHTY, SLY, NICE,
// MEEK, ABUSIVE order); operand 6 is the destination address. The player picks
// an attitude from the five-entry horizontal menu and the chosen operand value
// v[sel] is written to the destination. These attitude VALUES are deliberately
// DISTINCT from the menu indexes and from the encounter loop's outcome code.
static int handle_0x2C_PARLAY(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    vm.getOperand(6);

    uint8 attitudeValues[5];
    for (uint8 i = 0; i < 5; ++i)
        attitudeValues[i] = static_cast<uint8>(vm.readVar(static_cast<uint8>(i + 1)));
    const uint16 destAddr = vm.getOpWord(6);

    // Fixed attitude labels in the original HAUGHTY/SLY/NICE/MEEK/ABUSIVE order.
    Common::Array<Common::String> attitudes;
    attitudes.push_back("HAUGHTY");
    attitudes.push_back("SLY");
    attitudes.push_back("NICE");
    attitudes.push_back("MEEK");
    attitudes.push_back("ABUSIVE");

    int16 sel = 0;
    if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls))
        sel = host->parlayMenu(attitudes);

    // The original always resolves to a chosen attitude (the menu has no
    // cancel); clamp defensively to the 0..4 range and write the chosen value.
    if (sel < 0)
        sel = 0;
    if (sel > 4)
        sel = 4;

    vm.writeVmMemory(destAddr, attitudeValues[sel], syscalls);
    return VM_OK;
}

// 0x2D: CALL <address>
// Platform note:
// - x86 compares raw call IDs directly (0x2C90, 0x8000, 0x8001, 0xBA03, 0xC01E, 0xC018)
// - m68k compares (int16(callId) - 0x7FFF), which maps to the same raw IDs.
static int handle_0x2D_CALL(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    vm.getOperand(1);
    const uint16 callId = vm.getOpWord(1);

    EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls);
    if (!host)
        return VM_OK;

    if (callId == 0x2C90) {
        // Step 1: Always read geo cell at current position.
        // Mirrors: STRUCT_POSITION.geo_id = MAP_getGEOData(y, x)
        VmResult r = host->readGeoAtPosition();
        if (r != VM_OK)
            return r;

        // Step 2: Viewport refresh (GFX_ViewPortUpdate + DIALOG_StateArea).
        // Original gates on BOOL_MAPDATA_INLOAD which is true once geo+wallsets
        // are loaded. We always refresh here — the host's refreshViewport()
        // already guards on areaMapCache.isBuilt().
        r = host->refreshViewport();
        if (r != VM_OK)
            return r;

        return VM_OK;
    }

    if (callId == 0xC018) {
        // Only when indoor mode is active (BYTE_VM_MAP_TYPE == 1).
        if (mem.read8(getOpcodeLayout().vmField(kVmFieldIndoorModeFlag).vmAddr) != 1)
            return VM_OK;
    }

    // All other call IDs dispatch to the host's generic handler.
    return host->handleCallOpcode(callId);
}

// GB13-004: live-engine roll/save source for the scripted-damage core. Dice
// and target-index draws go through the engine RNG (FxEngine.RollDice parity);
// saves go through the shared saving-throw rules and the attack roll ports
// AttackRollNoAttacker (fxengine.pas:404). Keeping this adapter thin means the
// deterministic sequence lives entirely in resolveScriptedDamage(), which the
// tests drive with a recorded source.
class LiveScriptedDamageSource : public IScriptedDamageSource {
public:
    uint16 rollDice(uint8 count, uint8 sides) override {
        if (count == 0 || sides == 0)
            return 0;
        const int r = VmInterface::rollDice(count, sides);
        return (r < 0) ? 0 : static_cast<uint16>(r);
    }

    uint8 rollTargetIndex(uint8 partySize) override {
        // Original RollDice(1, PARTY_SIZE): 1-based position.
        if (partySize == 0)
            return 1;
        return static_cast<uint8>(VmInterface::rollDice(1, partySize));
    }

    bool rollSave(Data::PlayerCharacter &target,
            Data::Spells::SaveVerseType saveType, uint8 saveMod) override {
        Data::ADnDCharacter *adnd =
            dynamic_cast<Data::ADnDCharacter *>(&target);
        if (!adnd)
            return false;
        return Data::Rules::checkSavingThrow(*adnd, nullptr, nullptr, nullptr,
            saveType, static_cast<int8>(saveMod));
    }

    bool attackHits(Data::PlayerCharacter &target,
            uint8 sixtyMinusThac0) override {
        // AttackRollNoAttacker (fxengine.pas:404): natural-1 misses; natural-20
        // becomes 100; hit when roll + sixtyMinusThac0 exceeds acCur (stored
        // 60-based, matching armorClass.current).
        int roll = VmInterface::rollDice(1, 20);
        if (roll <= 1)
            return false;
        if (roll == 20)
            roll = 100;
        const int acCur = static_cast<int>(target.armorClass.current);
        return (roll + static_cast<int>(sixtyMinusThac0)) > acCur;
    }
};

// GB13-004: live damage sink. Routes every connecting hit through the real
// DamageSystem (EclInflictDamage behavior: enabled/status/effect changes and
// the down/death message), never subtracting HP directly in the handler. The
// original skips already-dead creatures (eclmenu.pas:1022); DamageSystem's
// resolution already no-ops on a disabled/dead target, so a second hit cannot
// re-kill it.
class LiveScriptedDamageSink : public IScriptedDamageSink {
public:
    void inflict(Data::PlayerCharacter &target, uint16 dmg) override {
        Data::DamageSystem damageSystem(nullptr);
        damageSystem.applyLegacy(target, static_cast<uint8>(dmg),
            Data::DAMAGE_NORMAL, false);
    }
};

// 0x2E: DAMAGE <attack> <dice> <sides> <bonus> <value>
// GB13-004: port EclDamage (eclvm.pas:1311). All five operands are value
// operands read via readVar (GB13-008). The deterministic target-selection and
// roll sequence lives in resolveScriptedDamage() so the exact draw order/count
// (including the final unused reroll in attack mode) is unit-provable; the live
// path wires it to the engine RNG, the real DamageSystem and the selection
// accessors. The mutation (damage + defeat scan + selection restore) completes
// synchronously BEFORE the acknowledgment presentation, so a yielding "press
// <enter>" / "entire party killed" message cannot re-apply damage on resume
// (GB13-009 exactly-once).
static int handle_0x2E_DAMAGE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    vm.getOperand(5);

    ScriptedDamageOperands ops;
    ops.attack = static_cast<uint8>(vm.readVar(1));
    ops.dice   = static_cast<uint8>(vm.readVar(2));
    ops.sides  = static_cast<uint8>(vm.readVar(3));
    ops.bonus  = static_cast<uint8>(vm.readVar(4));
    ops.value  = static_cast<uint8>(vm.readVar(5));

    // Snapshot the party in list order and the selected character up front.
    Common::List<Data::PlayerCharacter *> *partyList = VmInterface::getParty();
    Common::Array<Data::PlayerCharacter *> party;
    if (partyList) {
        for (Common::List<Data::PlayerCharacter *>::const_iterator it =
                partyList->begin(); it != partyList->end(); ++it) {
            if (*it)
                party.push_back(*it);
        }
    }
    Data::PlayerCharacter *savedSelection = VmInterface::getSelectedCharacter();

    LiveScriptedDamageSource source;
    LiveScriptedDamageSink sink;
    const ScriptedDamageOutcome outcome =
        resolveScriptedDamage(ops, party, savedSelection, source, sink);

    // Empty-party / malformed-operand diagnostic (Req 2.3 defensive). The core
    // applied no damage; surface the degenerate condition using the shared
    // diagnostic tag (GB13-001) so it is visible, without claiming an original
    // DOS result for the empty-party edge.
    if (outcome.degenerate && g_events) {
        g_events->postEclSyscallMessage(vm.getPC(), 0x2E,
            static_cast<uint16>(EclVmMessage::SC_UNIMPLEMENTED_OPCODE),
            static_cast<int16>(VM_OK));
    }

    // Restore the selected character exactly once (eclvm.pas:1395). The
    // mutation above already ran, so this is safe even if the acknowledgment
    // below yields.
    VmInterface::setSelectedCharacter(savedSelection);

    // Acknowledgment / total-party-defeat presentation (eclvm.pas:1388-1396).
    // This is the only part that may yield; it runs AFTER the mutation and the
    // selection restore, so a resume never re-applies damage. The host print
    // path owns the "entire party killed" window and the press-enter pause;
    // when no async print path exists the VM simply continues.
    if (syscalls && outcome.partyDefeated) {
        const VmResult asyncStart =
            syscalls->beginPrintAsync("The entire party is killed!", false);
        if (g_events) {
            g_events->postEclSyscallMessage(vm.getPC(), 0x2E,
                EclVmMessage::SC_PRINT, static_cast<int16>(asyncStart));
        }
        if (asyncStart == VM_YIELD || asyncStart == VM_ERROR)
            return asyncStart;
    }

    return VM_OK;
}

// 0x31: SPRITE OFF
static int handle_0x31_SPRITE_OFF(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)nextPc; (void)callStack;
    const EclLayoutAccess layout = getOpcodeLayout();
    const uint16 spriteLoadAddr = layout.runtimeField(kEclRuntimeSpriteState);
    const uint16 skyboxRedrawAddr = layout.runtimeField(kEclRuntimeSkyboxRedrawFlag);

    if (EclRuntimeLayout::isValidVmAddr(spriteLoadAddr)
            && mem.read8(spriteLoadAddr) != 0) {
        if (syscalls) {
            VmResult r = syscalls->spriteOff();
            if (g_events) {
                g_events->postEclSyscallMessage(vm.getPC(), 0x31,
                    EclVmMessage::SC_SPRITE_OFF,
                    static_cast<int16>(r));
            }
            if (r != VM_OK)
                return r;
        }

        mem.write8(spriteLoadAddr, 0);
        if (EclRuntimeLayout::isValidVmAddr(skyboxRedrawAddr))
            mem.write8(skyboxRedrawAddr, 0);
    }

    return VM_OK;
}

// 0x32: FIND ITEM <itemID>
// Pascal EclFindItem walks every party inventory in list order and sets the
// equality/not-equality compare flag by item type, with no status filters.
// The item ID is a value operand (GB13-008): read it via readVar. The compare
// polarity matches HAS EFFECT and the IF opcodes (handleIF): 0 => found so a
// following IF EQUAL branches, 1 => absent so IF NOT EQUAL branches.
static int handle_0x32_FIND_ITEM(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    vm.getOperand(1);

    const uint8 itemId = static_cast<uint8>(vm.readVar(1));
    bool found = false;

    if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls))
        found = host->findItemInParty(itemId);

    vm.setCmpResult(found ? 0 : 1);
    return VM_OK;
}

// 0x34: ECL CLOCK <var>
// Original EclClock (eclvm.pas:1411) loads exactly ONE operand (v1.3). The
// older DOS decompile reads VM_GetOprand(1) in Inst_CLOCK as well.
//
// OPEN (trace-gated, GB13-001.3/.6): which decoded operand supplies the amount
// is unresolved. The reconstructed Pascal reads GetEclVar(v) with v uninitialized,
// so the index cannot be proven without a binary trace of real CLOCK callers.
// We frame one operand and advance using operand 1, and emit a diagnostic so
// the open decision is visible. We do NOT claim parity on the amount value.
static int handle_0x34_ECL_CLOCK(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    vm.getOperand(1);

    // Framing is fixed at one operand and the clock IS advanced here. The
    // diagnostic flags only the OPEN amount-index decision (not a no-op): the
    // reconstructed Pascal reads an uninitialized index, so the operand that
    // supplies the amount is unresolved. We read operand 1 until a trace exists.
    const uint8 amount = static_cast<uint8>(vm.readVar(1));

    if (g_events) {
        g_events->postEclSyscallMessage(vm.getPC(), 0x34,
            static_cast<uint16>(EclVmMessage::SC_UNIMPLEMENTED_OPCODE),
            static_cast<int16>(amount));
    }

    if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls))
        return host->advanceClock(amount);

    return VM_OK;
}

// 0x35: SAVE TABLE <var1> <address> <var2>
static int handle_0x35_SAVE_TABLE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 value    = vm.readVar(1);
    const uint16 baseAddr = vm.getOpWord(2);
    const uint16 index    = vm.readVar(3);
    vm.writeVmMemory(static_cast<uint16>(baseAddr + index), value,
        syscalls);
    return VM_OK;
}

// 0x36: ADD NPC <monsterID> <morale>
static int handle_0x36_ADD_NPC(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    // GB13-006: Inst_NPC_ADD decodes two value operands via ReadOperandValue
    // (GB13-008): operand 1 = monster id, operand 2 = morale. The host loads
    // the creature + owned inventory/effects, appends it through the one party
    // ownership API, sets npc=(morale div 2)|0x80 and hostility (id 24 hostile,
    // else friendly), then recalculates derived stats.
    vm.getOperand(2);
    const uint8 monsterId = static_cast<uint8>(vm.readVar(1));
    const uint8 morale    = static_cast<uint8>(vm.readVar(2));

    if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls))
        return host->addNpc(monsterId, morale);
    return VM_OK;
}

// 0x37: LOAD_AREA_WALLDEF <primaryBlockId> <middleBlockId> <secondaryBlockId>
// Loads walldef geometry and 8x8 tile graphics into dynamic cache slots 1-3.
// Mirrors the walldef branch of INSTR_LoadAreaDeco from the original.
// Slots 0 and 4 are fixed (loaded at game init) and never touched here.
static int handle_0x37_LOAD_AREA_WALLDEF(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;

    vm.getOperand(3);
    const uint8 primaryBlockId   = static_cast<uint8>(vm.readVar(1));
    const uint8 middleBlockId    = static_cast<uint8>(vm.readVar(2));
    const uint8 secondaryBlockId = static_cast<uint8>(vm.readVar(3));

    // WallSetPrimary (+0x1CE) and WallSetSecondary (+0x1D0): when both are
    // non-zero the engine uses paired-wallset mode; otherwise independent mode.
    bool hasPrimary   = (mem.read8(
        getOpcodeLayout().vmField(kVmFieldWallSetPrimary).vmAddr) != 0);
    bool hasSecondary = (mem.read8(
        getOpcodeLayout().vmField(kVmFieldWallSetSecondary).vmAddr) != 0);

    if (primaryBlockId == 0x7F) {
        // Special default: load empty/default wallset block 0 into slot 1
        VmResult result = syscalls->loadWallSet(0, 1);
        if (result != VM_OK)
            return result;
    } else if (!hasPrimary || !hasSecondary) {
        // Independent mode: each param drives its own slot directly.
        // 0xFF values mean "invalidate slot" in the original flow.
        for (uint8 slot = 1; slot <= 3; ++slot) {
            uint8 blockId = (slot == 1) ? primaryBlockId
                          : (slot == 2) ? middleBlockId
                                        : secondaryBlockId;
            VmResult result = syscalls->loadWallSet(blockId, slot);
            if (g_events) {
                g_events->postEclSyscallMessage(vm.getPC(), 0x37,
                    EclVmMessage::SC_LOAD_WALLSET,
                    static_cast<int16>(result));
            }
            if (result != VM_OK)
                return result;
        }
    } else {
        // Paired-wallset mode: primary block fills slot 1 (and implicitly
        // slot 2 if the walldef has multiple 780-byte chunks); secondary
        // fills slot 3.  Middle param is unused in this mode.
        VmResult result = syscalls->loadWallSet(primaryBlockId, 1);
        if (g_events) {
            g_events->postEclSyscallMessage(vm.getPC(), 0x37,
                EclVmMessage::SC_LOAD_WALLSET,
                static_cast<int16>(result));
        }
        if (result != VM_OK)
            return result;
        result = syscalls->loadWallSet(secondaryBlockId, 3);
        if (g_events) {
            g_events->postEclSyscallMessage(vm.getPC(), 0x37,
                EclVmMessage::SC_LOAD_WALLSET,
                static_cast<int16>(result));
        }
        if (result != VM_OK)
            return result;
    }

    vm.wallsetReady = true;
    return vm.checkMapDataReady();
}

// 0x39: WHO <message>
//
// GB13-003: EclWho (eclvm.pas:1443) prompts the player to pick a party member
// and makes that member the current selection. It does NOT write a numeric VM
// result cell -- the selection itself is the output. We route through the
// host's PartySelector (async when available) and update the current selection
// by stable character identity; a cancel leaves the selection unchanged.
static int handle_0x39_WHO(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    vm.getOperand(1);
    const Common::String message = vm.readString(1);

    EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls);
    if (!host)
        return VM_OK;

    // Prefer the async path so the selection dialog yields the VM; the host
    // applies the new current selection on finalize.
    const VmResult asyncStart = host->beginSelectPartyMemberAsync(message);
    if (asyncStart == VM_YIELD || asyncStart == VM_ERROR)
        return asyncStart;

    // Synchronous fallback: selectPartyMember returns the chosen 0-based party
    // index or -1 on cancel. The host owns mapping index -> current selection;
    // a negative result leaves the selection unchanged (no VM destination).
    host->selectPartyMember(message);
    return VM_OK;
}

// 0x3B: SPELL <spellID> <address1> <address2>
//
// Pascal EclSpell (eclvm.pas:1479) reads a spell id value and two destination
// addresses, scans every party member's memorized spells, and writes a
// slot/index value to address1 and the member index to address2
// (WriteGbvm(addr1, slot); WriteGbvm(addr2, idx)). Operand roles (GB13-008):
// operand 1 is a value (readVar); operands 2 and 3 are literal VM destination
// addresses (getOpWord).
//
// TRACE-GATED (GB13-005): the original starts from an uninitialized found flag,
// scans past the 21 memorized slots (slot up to 81), and truncates both
// destinations to a byte. Those exact anomalies are NOT reproduced here and
// remain OPEN until executable/caller data exists; a diagnostic is emitted so
// the open decision is visible. The handler frames all three operands and
// routes to the host, which runs a BOUNDED memorized-spell scan via the real
// spell book and writes BOTH outputs with no out-of-range access. The old code
// wrote a constant 255 to address1 and never wrote address2 -- that is removed.
static int handle_0x3B_SPELL(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    vm.getOperand(3);

    const uint8 spellId = static_cast<uint8>(vm.readVar(1));
    const uint16 slotAddr   = vm.getOpWord(2); // address1: slot/index output
    const uint16 memberAddr = vm.getOpWord(3); // address2: member index output

    if (g_events) {
        g_events->postEclSyscallMessage(vm.getPC(), 0x3B,
            static_cast<uint16>(EclVmMessage::SC_UNIMPLEMENTED_OPCODE),
            static_cast<int16>(spellId));
    }

    if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls))
        return host->findSpell(spellId, memberAddr, slotAddr);

    return VM_OK;
}

// 0x3E: NPC REMOVE
static int handle_0x3E_NPC_REMOVE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x3F: HAS EFFECT <effectID>
static int handle_0x3F_HAS_EFFECT(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    vm.getOperand(1);

    const uint8 effectId = static_cast<uint8>(vm.readVar(1));
    bool isActive = false;

    if (EclEngineHost *host = dynamic_cast<EclEngineHost *>(syscalls))
        isActive = host->hasEffectActive(effectId);
    else
        isActive = hasEffectInParty(effectId);

    // Legacy IF_EQUAL after HAS EFFECT means present.
    vm.setCmpResult(isActive ? 0 : 1);
    return VM_OK;
}

// 0x40: DESTROY ITEM <itemID>
static int handle_0x40_DESTROY_ITEM(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    return VM_OK;
}

// 0x41: GIVE EXP <amount> <divideFlag>
static int handle_0x41_GIVE_EXP(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    return VM_OK;
}

// 0x42: STOP MOVE
static int handle_0x42_STOP_MOVE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_HALTED;
}

// 0x43: SOUND EVENT <soundID>
static int handle_0x43_SOUND(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    return VM_OK;
}

// 0x44: (unknown 0 args)
static int handle_0x44_UNKNOWN(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x45: RANDOM0 <destAddr> <maxVal>
static int handle_0x45_RANDOM0(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    const uint16 destAddr = vm.getOpWord(1);
    const uint16 maxVal   = vm.readVar(2);
    vm.writeVmMemory(destAddr, getLegacyRandomByte(maxVal), syscalls);
    return VM_OK;
}

// 0x46: FOR START <initVal> <maxVal>
static int handle_0x46_FOR_START(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    g_forLoopCount     = vm.readVar(1);
    g_forLoopMax       = vm.readVar(2);
    g_forLoopBodyStart = nextPc;
    return VM_OK;
}

// 0x47: FOR REPEAT
static int handle_0x47_FOR_REPEAT(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)callStack; (void)syscalls;
    g_forLoopCount++;
    if (g_forLoopCount <= g_forLoopMax)
        nextPc = g_forLoopBodyStart;
    return VM_OK;
}

// 0x48: (unknown, 1 arg)
static int handle_0x48_UNKNOWN(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    return VM_OK;
}

// 0x49: (unknown, 6 args)
static int handle_0x49_UNKNOWN(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(6);
    return VM_OK;
}

// 0x4A: (unknown, 0 args)
static int handle_0x4A_UNKNOWN(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x4B: (unknown, 1 arg)
static int handle_0x4B_UNKNOWN(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    return VM_OK;
}

// 0x4C: PICTURE 2 <pictureID> <variant>
static int handle_0x4C_PICTURE2(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(2);
    const uint8 picId = static_cast<uint8>(vm.getOpWord(1));
    mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldPictureHeadId).vmAddr, picId);
    return syscalls->displayPicture(picId);
}

void clearOpcodeHandlers() {
    g_handlers.clear();
}

void registerOpcodeHandler(uint8 opcode, OpcodeHandler handler) {
    g_handlers[opcode] = handler;
}

void registerBaselineOpcodeHandlers() {
    clearOpcodeHandlers();
    registerOpcodeHandler(0x00, handle_0x00_EXIT);
    registerOpcodeHandler(0x01, handle_0x01_GOTO);
    registerOpcodeHandler(0x02, handle_0x02_GOSUB);
    registerOpcodeHandler(0x03, handle_0x03_COMPARE);
    registerOpcodeHandler(0x04, handle_0x04_ADD);
    registerOpcodeHandler(0x05, handle_0x05_SUBTRACT);
    registerOpcodeHandler(0x06, handle_0x06_DIVIDE);
    registerOpcodeHandler(0x07, handle_0x07_MULTIPLY);
    registerOpcodeHandler(0x08, handle_0x08_RANDOM);
    registerOpcodeHandler(0x09, handle_0x09_SAVE);
    registerOpcodeHandler(0x0A, handle_0x0A_LOAD_CHARACTER);
    registerOpcodeHandler(0x0B, handle_0x0B_LOAD_MONSTER);
    registerOpcodeHandler(0x0C, handle_0x0C_SPRITE_START);
    registerOpcodeHandler(0x0D, handle_0x0D_SPRITE_ADVANCE);
    registerOpcodeHandler(0x0E, handle_0x0E_PICTURE);
    registerOpcodeHandler(0x0F, handle_0x0F_INPUT_NUMBER);
    registerOpcodeHandler(0x10, handle_0x10_INPUT_STRING);
    registerOpcodeHandler(0x11, handle_0x11_PRINT);
    registerOpcodeHandler(0x12, handle_0x12_PRINTCLEAR);
    registerOpcodeHandler(0x13, handle_0x13_RETURN);
    registerOpcodeHandler(0x14, handle_0x14_COMPARE_AND);
    registerOpcodeHandler(0x15, handle_0x15_VERTICAL_MENU);
    registerOpcodeHandler(0x16, handle_0x16_IF_EQUAL);
    registerOpcodeHandler(0x17, handle_0x17_IF_NOT_EQUAL);
    registerOpcodeHandler(0x18, handle_0x18_IF_LESS);
    registerOpcodeHandler(0x19, handle_0x19_IF_GREATER);
    registerOpcodeHandler(0x1A, handle_0x1A_IF_LESS_EQUAL);
    registerOpcodeHandler(0x1B, handle_0x1B_IF_GREATER_EQUAL);
    registerOpcodeHandler(0x1C, handle_0x1C_CLEARMONSTERS);
    registerOpcodeHandler(0x1D, handle_0x1D_PARTYSTRENGTH);
    registerOpcodeHandler(0x1E, handle_0x1E_CHECKPARTY);
    registerOpcodeHandler(0x1F, handle_0x1F_UNDEFINED);
    registerOpcodeHandler(0x20, handle_0x20_NEWECL);
    registerOpcodeHandler(0x21, handle_0x21_LOAD_AREA_GEO);
    registerOpcodeHandler(0x22, handle_0x22_PARTY_SURPRISE);
    registerOpcodeHandler(0x23, handle_0x23_SURPRISE);
    registerOpcodeHandler(0x24, handle_0x24_ENCOUNTER);
    registerOpcodeHandler(0x25, handle_0x25_ON_GOTO);
    registerOpcodeHandler(0x26, handle_0x26_ON_GOSUB);
    registerOpcodeHandler(0x27, handle_0x27_TREASURE);
    registerOpcodeHandler(0x28, handle_0x28_ROB);
    registerOpcodeHandler(0x29, handle_0x29_ENCOUNTER_MENU);
    registerOpcodeHandler(0x2A, handle_0x2A_COPY_MEM);
    registerOpcodeHandler(0x2B, handle_0x2B_HORIZONTAL_MENU);
    registerOpcodeHandler(0x2C, handle_0x2C_PARLAY);
    registerOpcodeHandler(0x2D, handle_0x2D_CALL);
    registerOpcodeHandler(0x2E, handle_0x2E_DAMAGE);
    registerOpcodeHandler(0x2F, handle_0x2F_AND);
    registerOpcodeHandler(0x30, handle_0x30_OR);
    registerOpcodeHandler(0x31, handle_0x31_SPRITE_OFF);
    registerOpcodeHandler(0x32, handle_0x32_FIND_ITEM);
    registerOpcodeHandler(0x33, handle_0x33_PRINT_RETURN);
    registerOpcodeHandler(0x34, handle_0x34_ECL_CLOCK);
    registerOpcodeHandler(0x35, handle_0x35_SAVE_TABLE);
    registerOpcodeHandler(0x36, handle_0x36_ADD_NPC);
    registerOpcodeHandler(0x37, handle_0x37_LOAD_AREA_WALLDEF);
    registerOpcodeHandler(0x38, handle_0x38_PROGRAM);
    registerOpcodeHandler(0x39, handle_0x39_WHO);
    registerOpcodeHandler(0x3A, handle_0x3A_DELAY);
    registerOpcodeHandler(0x3B, handle_0x3B_SPELL);
    registerOpcodeHandler(0x3C, handle_0x3C_PROTECTION);
    registerOpcodeHandler(0x3D, handle_0x3D_CLEAR_BOX);
    registerOpcodeHandler(0x3E, handle_0x3E_NPC_REMOVE);
    registerOpcodeHandler(0x3F, handle_0x3F_HAS_EFFECT);
    registerOpcodeHandler(0x40, handle_0x40_DESTROY_ITEM);
    registerOpcodeHandler(0x41, handle_0x41_GIVE_EXP);
    registerOpcodeHandler(0x42, handle_0x42_STOP_MOVE);
    registerOpcodeHandler(0x43, handle_0x43_SOUND);
    registerOpcodeHandler(0x44, handle_0x44_UNKNOWN);
    registerOpcodeHandler(0x45, handle_0x45_RANDOM0);
    registerOpcodeHandler(0x46, handle_0x46_FOR_START);
    registerOpcodeHandler(0x47, handle_0x47_FOR_REPEAT);
    registerOpcodeHandler(0x48, handle_0x48_UNKNOWN);
    registerOpcodeHandler(0x49, handle_0x49_UNKNOWN);
    registerOpcodeHandler(0x4A, handle_0x4A_UNKNOWN);
    registerOpcodeHandler(0x4B, handle_0x4B_UNKNOWN);
    registerOpcodeHandler(0x4C, handle_0x4C_PICTURE2);
}

OpcodeHandler getOpcodeHandler(uint8 opcode) {
    if (g_handlers.contains(opcode)) {
        return g_handlers[opcode];
    }
    return nullptr;
}

} // namespace ECL
} // namespace Goldbox
