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
#include "common/random.h"
#include "goldbox/vm_interface.h"
#include "goldbox/core/vm_layout.h"
#include "goldbox/ecl/runtime_layout.h"
#include "goldbox/events.h"
#include "goldbox/data/effects/character_effects.h"

namespace Goldbox {
namespace ECL {

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
    Common::Array<Data::PlayerCharacter *> *party = VmInterface::getParty();
    if (!party)
        return false;

    for (uint i = 0; i < party->size(); ++i) {
        Data::PlayerCharacter *character = (*party)[i];
        if (!character)
            continue;

        const Data::Effects::CharacterEffects *effects =
            character->getEffects();
        if (!effects)
            continue;

        const Common::Array<Data::Effects::Effect> &list = effects->effects();
        for (uint j = 0; j < list.size(); ++j) {
            if (list[j].type == effectId)
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

static int handle_0x0F_INPUT_NUMBER(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(2);
    const uint8  maxDigits = static_cast<uint8>(vm.getOpWord(1));
    const uint16 addr      = vm.getOpWord(2);
    const int16  value     = syscalls->inputNumber(maxDigits);
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
    const Common::String result = syscalls->inputString(maxLength);
    for (uint i = 0; i < result.size(); ++i)
        mem.write8(static_cast<uint16>(addr + i), static_cast<uint8>(result[i]));
    mem.write8(static_cast<uint16>(addr + result.size()), 0);
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

        Common::Array<Data::PlayerCharacter *> *party =
            VmInterface::getParty();
        if (party && selectedIndex < party->size())
            VmInterface::setSelectedCharacter((*party)[selectedIndex]);
    }

    // Poolrad originals (x86/m68k): when bit 7 is set and both redraw flags
    // are true, perform party UI/update flow and then clear redraw flags.
    // We mirror the observable VM side effect (flag clear) here.
    const uint16 statusRedrawAddr = getOpcodeLayout().runtimeField(
        kEclRuntimeStatusRedrawFlag);
    const uint16 charRedrawAddr = getOpcodeLayout().runtimeField(
        kEclRuntimeCharacterRedrawFlag);
    if (npcFlag && EclRuntimeLayout::isValidVmAddr(statusRedrawAddr)
            && EclRuntimeLayout::isValidVmAddr(charRedrawAddr)
            && mem.read8(statusRedrawAddr) != 0
            && mem.read8(charRedrawAddr) != 0) {
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

    const uint16 currentCount = mem.read16LE(getOpcodeLayout().runtimeField(kEclRuntimeMonsterCount));
    mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeMonsterCount),
        static_cast<uint16>((currentCount + count) & 0xFFFF));
    mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeEncounterFlags), monsterId);
    return VM_OK;
}

// 0x0C: SETUP MONSTER <monsterID> <distance> <graphicID>
// 0x0C: SPRITE START
// Load sprite resource and variant, draw encounter stage with calculated distance
static int handle_0x0C_SPRITE_START(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(3);
    const uint8 resourceId = static_cast<uint8>(vm.getOpWord(1));
    const uint8 distanceCap = static_cast<uint8>(vm.getOpWord(2));
    const uint8 variantId = static_cast<uint8>(vm.getOpWord(3));
    return syscalls->drawEncounterStage(resourceId, distanceCap, variantId);
}

// 0x0D: APPROACH
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
    const uint8 picId = static_cast<uint8>(vm.getOpWord(1));

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
    (void)vm; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
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
    const VmResult result =
        syscalls->executeProgram(static_cast<uint8>(vm.getOpWord(1)));
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
    uint16 addr = vm.getOpWord(1);
    Common::String runes;
    uint8 ch;
    while ((ch = mem.read8(addr++)) != 0)
        runes += static_cast<char>(ch);
    syscalls->printText(runes, false);
    return VM_OK;
}

static int handle_0x3D_CLEAR_BOX(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
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
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(4);
    const bool bothEqual = (vm.readVar(1) == vm.readVar(2)) && (vm.readVar(3) == vm.readVar(4));
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
static int handle_0x1D_PARTYSTRENGTH(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    // TODO: Calculate party strength based on character levels/stats
    vm.writeVmMemory(vm.getOpWord(1), 100, syscalls);
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
static int handle_0x20_NEWECL(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(1);
    const VmResult result =
        syscalls->loadScript(static_cast<uint8>(vm.getOpWord(1)));
    if (g_events) {
        g_events->postEclSyscallMessage(vm.getPC(), 0x20,
            EclVmMessage::SC_LOAD_SCRIPT,
            static_cast<int16>(result));
    }
    return result;
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
static int handle_0x22_PARTY_SURPRISE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    vm.writeVmMemory(vm.getOpWord(1), 2, syscalls);
    vm.writeVmMemory(vm.getOpWord(2), 2, syscalls);
    return VM_OK;
}

// 0x23: SURPRISE <address1> <address2> <var1> <var2>
static int handle_0x23_SURPRISE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(4);
    // TODO: Roll dice using readVar(3)/readVar(4) and calculate surprise
    mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldCombatIsAmbush).vmAddr, 0);
    return VM_OK;
}

// 0x24: ENCOUNTER
// Primary encounter dispatcher. Branch logic:
// - If monsters are loaded (MONST_LOAD_READY or COMBAT_TRIGGER): run combat
// - Else if ShopFlag: open shop dialog
// - Else if TemplePending: open temple dialog
// - Else: just run battle-end cleanup (no-op encounter)
// Post-encounter: reset game state, clear search flags, clear draw state.
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

    // Post-encounter cleanup: reset game state based on map type
    const uint8 indoorMode = mem.read8(
        layout.vmField(kVmFieldIndoorModeFlag).vmAddr);
    mem.write8(layout.runtimeField(kEclRuntimeGameState),
        indoorMode == 1 ? 1 : 0);

    // Clear search flags (keep only bit 0)
    const uint16 searchAddr = layout.vmGlobalField(
        kVmGlobalFieldSearchFlags).vmAddr;
    mem.write8(searchAddr,
        static_cast<uint8>(mem.read8(searchAddr) & 1));

    // Clear draw state and skybox redraw flag
    const uint16 skyboxAddr = layout.runtimeField(
        kEclRuntimeSkyboxRedrawFlag);
    if (EclRuntimeLayout::isValidVmAddr(skyboxAddr))
        mem.write8(skyboxAddr, 0);

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
static int handle_0x27_TREASURE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x28: ROB <isWholeParty> <percentMoney> <itemChance>
static int handle_0x28_ROB(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x29: ENCOUNTER MENU (varargs)
static int handle_0x29_ENCOUNTER_MENU(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
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
static int handle_0x2C_PARLAY(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(6);
    vm.writeVmMemory(vm.getOpWord(6), 0, syscalls); // TODO: parlay dialog
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

// 0x2E: DAMAGE <var1> <dice> <sides> <bonus> <var2>
static int handle_0x2E_DAMAGE(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(5);
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
static int handle_0x32_FIND_ITEM(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    vm.setCmpResult(1); // TODO: search inventory
    return VM_OK;
}

// 0x34: ECL CLOCK <var> <timeunit>
static int handle_0x34_ECL_CLOCK(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    vm.getOperand(2);

    // Operand 1 is amount; operand 2 is legacy time-unit selector.
    // Current host path advances minute units (x86 TIME_AddUnits(1, amount))
    // and ignores operand 2 until full selector parity is wired.
    const uint8 amount = static_cast<uint8>(vm.readVar(1));

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
    (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(2);
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
static int handle_0x39_WHO(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    return VM_OK;
}

// 0x3B: SPELL <spellID> <address1> <address2>
static int handle_0x3B_SPELL(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    vm.writeVmMemory(vm.getOpWord(2), 255, syscalls);
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
