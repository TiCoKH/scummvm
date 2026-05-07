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
#include "goldbox/ecl/ecl_vm.h"
#include "common/hashmap.h"
#include "common/random.h"
#include "goldbox/vm_interface.h"
#include "goldbox/core/vm_layout.h"
#include "goldbox/ecl/runtime_layout.h"

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

uint16 resolveVar(AddressSpace &mem, const EclOperand &operand) {
    (void)mem; (void)operand;
    return 0; // Deprecated: use EclVM::readVar()
}

// Compare result helpers (mirrors original's BOOL_EQ_FLAG/BOOL_NE_FLAG/BOOL_LT_FLAG/BOOL_GT_FLAG/BOOL_LE_FLAG/BOOL_GE_FLAG).
// Original x86: VM_SetIntCompareFlags sets 6 separate bool globals; the IF opcodes each check one.
// Here we collapse them to a signed int8 stored at kEclRuntimeBreakFlag:
//   <0 (-1) = LT  (BOOL_LT_FLAG + BOOL_NE_FLAG + BOOL_LE_FLAG)
//    0      = EQ  (BOOL_EQ_FLAG + BOOL_LE_FLAG + BOOL_GE_FLAG)
//   >0 (+1) = GT  (BOOL_GT_FLAG + BOOL_NE_FLAG + BOOL_GE_FLAG)
static void setCmpResult(AddressSpace &mem, int32 result) {
    int8 sign = (result < 0) ? -1 : (result > 0) ? 1 : 0;
    mem.write8(getOpcodeLayout().runtimeField(kEclRuntimeBreakFlag), (uint8)sign);
}

static int8 getCmpResult(AddressSpace &mem) {
    return (int8)mem.read8(getOpcodeLayout().runtimeField(kEclRuntimeBreakFlag));
}

// Static state for FOR loop (not nested; matches Java VirtualMachine behavior).
static uint16 g_forLoopBodyStart = 0;
static uint16 g_forLoopCount = 0;
static uint16 g_forLoopMax = 0;

// Opcode handlers

static int handle_0x00_EXIT(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_HALTED;
}

static int handle_0x01_GOTO(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)insn; (void)callStack; (void)syscalls;
    // VM_GetOprand(1); WORD_ECL_PC = toWord(HI_SAVE[1], LOW_SAVE[1]);
    vm.getOperand(1);
    nextPc = vm.getOpWord(1);
    return VM_OK;
}

static int handle_0x02_GOSUB(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)insn; (void)syscalls;
    vm.getOperand(1);
    callStack.push_back(nextPc);
    nextPc = vm.getOpWord(1);
    return VM_OK;
}

// 0x03: COMPARE <var1> <var2>
static int handle_0x03_COMPARE(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    const uint16 a0 = vm.readVar(1);
    const uint16 a1 = vm.readVar(2);
    setCmpResult(mem, static_cast<int32>(a0) - static_cast<int32>(a1));
    return VM_OK;
}

static int handle_0x04_ADD(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 var1 = vm.readVar(1);
    const uint16 var2 = vm.readVar(2);
    const uint16 addr = vm.getOpWord(3);
    mem.write16LE(addr, static_cast<uint16>((var1 + var2) & 0xFFFF));
    return VM_OK;
}

static int handle_0x05_SUBTRACT(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 var1 = vm.readVar(1);
    const uint16 var2 = vm.readVar(2);
    const uint16 addr = vm.getOpWord(3);
    // Pool of Radiance: subtracts var1 from var2.
    mem.write16LE(addr, static_cast<uint16>((var2 - var1) & 0xFFFF));
    return VM_OK;
}

static int handle_0x06_DIVIDE(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 var1 = vm.readVar(1);
    const uint16 var2 = vm.readVar(2);
    const uint16 addr = vm.getOpWord(3);
    if (var2 == 0)
        return VM_ERROR;
    mem.write16LE(addr, var1 / var2);
    return VM_OK;
}

static int handle_0x07_MULTIPLY(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 var1 = vm.readVar(1);
    const uint16 var2 = vm.readVar(2);
    const uint16 addr = vm.getOpWord(3);
    mem.write16LE(addr, static_cast<uint16>((var1 * var2) & 0xFFFF));
    return VM_OK;
}

static int handle_0x09_SAVE(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    const uint16 val  = vm.readVar(1);
    const uint16 addr = vm.getOpWord(2);
    mem.write16LE(addr, val);
    return VM_OK;
}

static int handle_0x11_PRINT(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(1);
    syscalls->printText(vm.readString(1), false);
    return VM_OK;
}

static int handle_0x12_PRINTCLEAR(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(1);
    syscalls->printText(vm.readString(1), true);
    return VM_OK;
}

static int handle_0x13_RETURN(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)insn; (void)syscalls;
    if (callStack.empty())
        return VM_ERROR;
    nextPc = callStack.back();
    callStack.pop_back();
    return VM_OK;
}

// 0x2F: AND <var1> <var2> <destAddr>
static int handle_0x2F_AND(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 result = vm.readVar(1) & vm.readVar(2);
    mem.write16LE(vm.getOpWord(3), result);
    setCmpResult(mem, result == 0 ? 0 : 1);
    return VM_OK;
}

// 0x30: OR <var1> <var2> <destAddr>
static int handle_0x30_OR(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 result = vm.readVar(1) | vm.readVar(2);
    mem.write16LE(vm.getOpWord(3), result);
    setCmpResult(mem, result == 0 ? 0 : 1);
    return VM_OK;
}

static int handle_0x08_RANDOM(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    const uint16 maxVal  = vm.readVar(1);
    const uint16 destAddr = vm.getOpWord(2);
    mem.write16LE(destAddr, static_cast<uint16>(getOpcodeRandom().getRandomNumber(maxVal)));
    return VM_OK;
}

static int handle_0x0F_INPUT_NUMBER(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(2);
    const uint8  maxDigits = static_cast<uint8>(vm.getOpWord(1));
    const uint16 addr      = vm.getOpWord(2);
    const int16  value     = syscalls->inputNumber(maxDigits);
    if (value >= 0)
        mem.write16LE(addr, static_cast<uint16>(value));
    return VM_OK;
}

static int handle_0x10_INPUT_STRING(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack;
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
static int handle_0x0A_LOAD_CHARACTER(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    const uint8 sel = static_cast<uint8>(vm.getOpWord(1));
    const uint8 partySize = mem.read8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldPartyCount).vmAddr);
    mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldSelectedPcIndex).vmAddr, sel);
    if (sel < 128) {
        if (sel < partySize) {
            mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeSelectedCharPtr),
                getCharacterBlockBase(sel));
        } else {
            const uint16 monsterIndex = sel - partySize;
            (void)monsterIndex;
            mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeSelectedCharPtr),
                getOpcodeLayout().runtimeField(kEclRuntimeMonsterData));
        }
    } else {
        mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeSelectedCharPtr),
            getOpcodeLayout().runtimeField(kEclRuntimeMonsterData));
        mem.write8(getOpcodeLayout().runtimeField(kEclRuntimeMenuCombatState), 1);
    }
    return VM_OK;
}

// 0x0B: LOAD MONSTER <monsterID> <count> <graphicID>
static int handle_0x0B_LOAD_MONSTER(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 monsterId = vm.readVar(1);
    const uint16 count     = vm.readVar(2);
    const uint16 currentCount = mem.read16LE(getOpcodeLayout().runtimeField(kEclRuntimeMonsterCount));
    mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeMonsterCount),
        static_cast<uint16>((currentCount + (count & 0xFF)) & 0xFFFF));
    mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeEncounterFlags), monsterId & 0xFF);
    return VM_OK;
}

// 0x0C: SETUP MONSTER <monsterID> <distance> <graphicID>
static int handle_0x0C_SETUP_MONSTER(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(3);
    const uint16 monsterId = vm.readVar(1);
    const uint16 distance  = vm.readVar(2);
    const uint8  dist      = (distance > 2) ? 2 : static_cast<uint8>(distance);
    mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldMonsterDistance).vmAddr, dist);
    mem.write8(getOpcodeLayout().runtimeField(kEclRuntimeGameState), static_cast<uint8>(GS_COMBAT));
    mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeEncounterFlags), monsterId & 0xFF);
    return syscalls->startCombat();
}

// 0x0D: APPROACH
static int handle_0x0D_APPROACH(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    uint8 dist = mem.read8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldMonsterDistance).vmAddr);
    if (dist > 0)
        mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldMonsterDistance).vmAddr, dist - 1);
    return VM_OK;
}

// 0x0E: PICTURE <pictureID>
static int handle_0x0E_PICTURE(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(1);
    const uint8 picId = static_cast<uint8>(vm.getOpWord(1));
    mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldPictureHeadId).vmAddr, picId);
    return syscalls->displayPicture(picId);
}

static int handle_0x33_PRINT_RETURN(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)insn; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    syscalls->printText(Common::String(), false);
    return VM_OK;
}

static int handle_0x3A_DELAY(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

static int handle_0x38_PROGRAM(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(1);
    return syscalls->executeProgram(static_cast<uint8>(vm.getOpWord(1)));
}

static int handle_0x3C_PROTECTION(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack;
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

static int handle_0x3D_CLEAR_BOX(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)insn; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    syscalls->clearTextBox();
    return VM_OK;
}

// 0x14: COMPARE AND <var1> <var2> <var3> <var4>
// Original: calls VM_SetIntCompareFlags twice; result is EQ if BOTH pairs are equal.
// Only EQ/NE flags are meaningful after this opcode (no magnitude comparison).
static int handle_0x14_COMPARE_AND(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(4);
    const bool bothEqual = (vm.readVar(1) == vm.readVar(2)) && (vm.readVar(3) == vm.readVar(4));
    setCmpResult(mem, bothEqual ? 0 : 1);
    return VM_OK;
}

// 0x15: VERTICAL MENU <address> <message> <count> <stringVarargs>
static int handle_0x15_VERTICAL_MENU(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
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
    if (selection >= 0)
        mem.write16LE(resultAddr, static_cast<uint16>(selection));
    return VM_OK;
}

// 0x16-0x1B: IF commands
static int handleIF(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, uint8 opcode) {
    (void)vm; (void)insn; (void)callStack;
    const int8 cmp = getCmpResult(mem);
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
        nextPc += 1;
    return VM_OK;
}

static int handle_0x16_IF_EQUAL(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)syscalls;
    return handleIF(vm, mem, insn, nextPc, callStack, 0x16);
}

static int handle_0x17_IF_NOT_EQUAL(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)syscalls;
    return handleIF(vm, mem, insn, nextPc, callStack, 0x17);
}

static int handle_0x18_IF_LESS(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)syscalls;
    return handleIF(vm, mem, insn, nextPc, callStack, 0x18);
}

static int handle_0x19_IF_GREATER(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)syscalls;
    return handleIF(vm, mem, insn, nextPc, callStack, 0x19);
}

static int handle_0x1A_IF_LESS_EQUAL(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)syscalls;
    return handleIF(vm, mem, insn, nextPc, callStack, 0x1A);
}

static int handle_0x1B_IF_GREATER_EQUAL(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)syscalls;
    return handleIF(vm, mem, insn, nextPc, callStack, 0x1B);
}

// 0x1C: CLEARMONSTERS
static int handle_0x1C_CLEARMONSTERS(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeMonsterCount), 0);
    return VM_OK;
}

// 0x1D: PARTYSTRENGTH <address>
static int handle_0x1D_PARTYSTRENGTH(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    // TODO: Calculate party strength based on character levels/stats
    mem.write16LE(vm.getOpWord(1), 100);
    return VM_OK;
}

// 0x1E: CHECKPARTY <attributeAddress> <effectID> <unknown> <address1> <unknown> <address2>
static int handle_0x1E_CHECKPARTY(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(6);
    const uint16 attributeAddr = vm.readVar(1);
    const uint16 effectID      = vm.readVar(2);
    const uint16 addr1         = vm.getOpWord(4);
    const uint16 addr2         = vm.getOpWord(6);

    if (attributeAddr != 0 && effectID == 0) {
        mem.write16LE(addr1, 18); // Placeholder highest
        mem.write16LE(addr2, 3);  // Placeholder lowest
    } else if (attributeAddr == 0 && effectID != 0) {
        mem.write16LE(addr2, 0); // Placeholder: no characters have effect
    }
    return VM_OK;
}

// 0x20: NEWECL <script>
static int handle_0x20_NEWECL(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(1);
    return syscalls->loadScript(static_cast<uint8>(vm.getOpWord(1)));
}

// 0x21: LOAD FILES <geoBlockId> <unused> <iconTrigger>
static int handle_0x21_LOAD_FILES(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(3);
    const uint8 geoBlockId  = static_cast<uint8>(vm.readVar(1));
    const uint8 iconTrigger = static_cast<uint8>(vm.readVar(3));
    const bool  indoorMode  = (mem.read8(
        getOpcodeLayout().vmField(kVmFieldIndoorModeFlag).vmAddr) != 0);

    if (geoBlockId != 0xFF && geoBlockId != 0x7F && indoorMode) {
        mem.write8(getOpcodeLayout().vmField(kVmFieldGeoBlockId).vmAddr, geoBlockId);
        syscalls->loadGeoBlock(geoBlockId);
        mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldMovementBlock).vmAddr, 0);
    }
    if (iconTrigger != 0xFF && !indoorMode)
        syscalls->loadIconBlock();
    return VM_OK;
}

// 0x22: PARTY SURPRISE <address1> <address2>
static int handle_0x22_PARTY_SURPRISE(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    mem.write16LE(vm.getOpWord(1), 2);
    mem.write16LE(vm.getOpWord(2), 2);
    return VM_OK;
}

// 0x23: SURPRISE <address1> <address2> <var1> <var2>
static int handle_0x23_SURPRISE(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(4);
    // TODO: Roll dice using readVar(3)/readVar(4) and calculate surprise
    mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldCombatIsAmbush).vmAddr, 0);
    return VM_OK;
}

// 0x24: COMBAT
static int handle_0x24_COMBAT(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)insn; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    const uint8 templeFlag = mem.read8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldEnterTemplePending).vmAddr);
    const uint8 shopFlag   = mem.read8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldShopFlag).vmAddr);
    if (templeFlag == 1) {
        mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldEnterTemplePending).vmAddr, 0);
        return VM_OK;
    } else if (shopFlag == 1) {
        mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldShopFlag).vmAddr, 0);
        return VM_OK;
    }
    return syscalls->startCombat();
}

// 0x25: ON GOTO <var> <count> <addressVarargs>
static int handle_0x25_ON_GOTO(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    const uint16 var   = vm.readVar(1);
    const uint8  count = static_cast<uint8>(vm.getOpWord(2));
    if (var < count && (2 + var) < (uint16)insn.operands.size())
        nextPc = insn.operands[2 + var].u16;
    return VM_OK;
}

// 0x26: ON GOSUB <var> <count> <addressVarargs>
static int handle_0x26_ON_GOSUB(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)syscalls;
    vm.getOperand(2);
    const uint16 var   = vm.readVar(1);
    const uint8  count = static_cast<uint8>(vm.getOpWord(2));
    if (var < count && (2 + var) < (uint16)insn.operands.size()) {
        callStack.push_back(nextPc);
        nextPc = insn.operands[2 + var].u16;
    }
    return VM_OK;
}

// 0x27: TREASURE <copper> <silver> <electrum> <gold> <platinum> <gems> <jewelry> <treasureID>
static int handle_0x27_TREASURE(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x28: ROB <isWholeParty> <percentMoney> <itemChance>
static int handle_0x28_ROB(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x29: ENCOUNTER MENU (varargs)
static int handle_0x29_ENCOUNTER_MENU(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x2A: GETTABLE <address1> <var> <address2>
static int handle_0x2A_GETTABLE(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 baseAddr = vm.getOpWord(1);
    const uint16 index    = vm.readVar(2);
    const uint16 destAddr = vm.getOpWord(3);
    const uint16 value    = mem.read16LE(static_cast<uint16>(baseAddr + index));
    mem.write16LE(destAddr, value);
    setCmpResult(mem, value == 0 ? 0 : 1);
    return VM_OK;
}

// 0x2B: HORIZONTAL MENU <address> <count> <stringVarargs>
static int handle_0x2B_HORIZONTAL_MENU(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
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

    const int16 selection = syscalls->horizontalMenu(options);
    if (selection >= 0)
        mem.write16LE(resultAddr, static_cast<uint16>(selection));
    return VM_OK;
}

// 0x2C: PARLAY <haughty> <sly> <nice> <meek> <abusive> <address>
static int handle_0x2C_PARLAY(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(6);
    mem.write16LE(vm.getOpWord(6), 0); // TODO: parlay dialog
    return VM_OK;
}

// 0x2D: CALL <address>
static int handle_0x2D_CALL(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x2E: DAMAGE <var1> <dice> <sides> <bonus> <var2>
static int handle_0x2E_DAMAGE(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(5);
    return VM_OK;
}

// 0x31: SPRITE OFF
static int handle_0x31_SPRITE_OFF(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    mem.write8(getOpcodeLayout().runtimeField(kEclRuntimeSpriteState), 0);
    return VM_OK;
}

// 0x32: FIND ITEM <itemID>
static int handle_0x32_FIND_ITEM(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    setCmpResult(mem, 1); // TODO: search inventory
    return VM_OK;
}

// 0x34: ECL CLOCK <var> <timeunit>
static int handle_0x34_ECL_CLOCK(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x35: SAVE TABLE <var1> <address> <var2>
static int handle_0x35_SAVE_TABLE(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    const uint16 value    = vm.readVar(1);
    const uint16 baseAddr = vm.getOpWord(2);
    const uint16 index    = vm.readVar(3);
    mem.write16LE(static_cast<uint16>(baseAddr + index), value);
    return VM_OK;
}

// 0x36: ADD NPC <monsterID> <morale>
static int handle_0x36_ADD_NPC(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    return VM_OK;
}

// 0x37: LOAD PIECES <primaryBlockId> <middleBlockId> <secondaryBlockId>
// Loads walldef geometry and 8x8 tile graphics into dynamic cache slots 1-3.
// Mirrors the walldef branch of INSTR_LoadAreaDeco from the original.
// Slots 0 and 4 are fixed (loaded at game init) and never touched here.
static int handle_0x37_LOAD_PIECES(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack;
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
        syscalls->loadWallSet(0, 1);
    } else if (!hasPrimary || !hasSecondary) {
        // Independent mode: each param drives its own slot directly.
        // Slot 2 may be explicitly filled here (param2 != 0xFF).
        for (uint8 slot = 1; slot <= 3; ++slot) {
            uint8 blockId = (slot == 1) ? primaryBlockId
                          : (slot == 2) ? middleBlockId
                                        : secondaryBlockId;
            // 0xFF = invalidate/clear this slot
            syscalls->loadWallSet(blockId, slot);
        }
    } else {
        // Paired-wallset mode: primary block fills slot 1 (and implicitly
        // slot 2 if the walldef has multiple 780-byte chunks); secondary
        // fills slot 3.  Middle param is unused in this mode.
        syscalls->loadWallSet(primaryBlockId, 1);
        syscalls->loadWallSet(secondaryBlockId, 3);
    }

    return VM_OK;
}

// 0x39: WHO <message>
static int handle_0x39_WHO(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    return VM_OK;
}

// 0x3B: SPELL <spellID> <address1> <address2>
static int handle_0x3B_SPELL(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(3);
    mem.write16LE(vm.getOpWord(2), 255); // TODO: search for spell caster
    return VM_OK;
}

// 0x3E: NPC REMOVE
static int handle_0x3E_NPC_REMOVE(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x3F: HAS EFFECT <effectID>
static int handle_0x3F_HAS_EFFECT(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    setCmpResult(mem, 1); // TODO: check active effects
    return VM_OK;
}

// 0x40: DESTROY ITEM <itemID>
static int handle_0x40_DESTROY_ITEM(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    return VM_OK;
}

// 0x41: GIVE EXP <amount> <divideFlag>
static int handle_0x41_GIVE_EXP(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    return VM_OK;
}

// 0x42: STOP MOVE
static int handle_0x42_STOP_MOVE(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_HALTED;
}

// 0x43: SOUND EVENT <soundID>
static int handle_0x43_SOUND(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    return VM_OK;
}

// 0x44: (unknown 0 args)
static int handle_0x44_UNKNOWN(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x45: RANDOM0 <destAddr> <maxVal>
static int handle_0x45_RANDOM0(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    const uint16 destAddr = vm.getOpWord(1);
    const uint16 maxVal   = vm.readVar(2);
    const uint16 result   = (maxVal > 0) ?
        static_cast<uint16>(getOpcodeRandom().getRandomNumber(maxVal)) : 0;
    mem.write16LE(destAddr, result);
    return VM_OK;
}

// 0x46: FOR START <initVal> <maxVal>
static int handle_0x46_FOR_START(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)insn; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    g_forLoopCount     = vm.readVar(1);
    g_forLoopMax       = vm.readVar(2);
    g_forLoopBodyStart = nextPc;
    return VM_OK;
}

// 0x47: FOR REPEAT
static int handle_0x47_FOR_REPEAT(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)insn; (void)callStack; (void)syscalls;
    g_forLoopCount++;
    if (g_forLoopCount <= g_forLoopMax)
        nextPc = g_forLoopBodyStart;
    return VM_OK;
}

// 0x48: (unknown, 1 arg)
static int handle_0x48_UNKNOWN(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    return VM_OK;
}

// 0x49: (unknown, 6 args)
static int handle_0x49_UNKNOWN(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(6);
    return VM_OK;
}

// 0x4A: (unknown, 0 args)
static int handle_0x4A_UNKNOWN(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x4B: (unknown, 1 arg)
static int handle_0x4B_UNKNOWN(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)mem; (void)insn; (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(1);
    return VM_OK;
}

// 0x4C: PICTURE 2 <pictureID> <variant>
static int handle_0x4C_PICTURE2(EclVM &vm, AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    (void)insn; (void)nextPc; (void)callStack;
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
    registerOpcodeHandler(0x0C, handle_0x0C_SETUP_MONSTER);
    registerOpcodeHandler(0x0D, handle_0x0D_APPROACH);
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
    registerOpcodeHandler(0x20, handle_0x20_NEWECL);
    registerOpcodeHandler(0x21, handle_0x21_LOAD_FILES);
    registerOpcodeHandler(0x22, handle_0x22_PARTY_SURPRISE);
    registerOpcodeHandler(0x23, handle_0x23_SURPRISE);
    registerOpcodeHandler(0x24, handle_0x24_COMBAT);
    registerOpcodeHandler(0x25, handle_0x25_ON_GOTO);
    registerOpcodeHandler(0x26, handle_0x26_ON_GOSUB);
    registerOpcodeHandler(0x27, handle_0x27_TREASURE);
    registerOpcodeHandler(0x28, handle_0x28_ROB);
    registerOpcodeHandler(0x29, handle_0x29_ENCOUNTER_MENU);
    registerOpcodeHandler(0x2A, handle_0x2A_GETTABLE);
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
    registerOpcodeHandler(0x37, handle_0x37_LOAD_PIECES);
    registerOpcodeHandler(0x38, handle_0x38_PROGRAM);
    registerOpcodeHandler(0x39, handle_0x39_WHO);
    registerOpcodeHandler(0x3A, handle_0x3A_DELAY);
    registerOpcodeHandler(0x3B, handle_0x3B_SPELL);
    registerOpcodeHandler(0x3C, handle_0x3C_PROTECTION);
    registerOpcodeHandler(0x3D, handle_0x3D_CLEAR_BOX);
}

OpcodeHandler getOpcodeHandler(uint8 opcode) {
    if (g_handlers.contains(opcode)) {
        return g_handlers[opcode];
    }
    return nullptr;
}

} // namespace ECL
} // namespace Goldbox
