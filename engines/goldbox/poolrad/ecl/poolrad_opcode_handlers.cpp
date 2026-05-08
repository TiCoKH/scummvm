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

#include "goldbox/poolrad/ecl/poolrad_opcode_handlers.h"
#include "goldbox/ecl/ecl_engine_host.h"
#include "goldbox/ecl/ecl_vm.h"
#include "goldbox/ecl/opcode_handlers.h"
#include "goldbox/ecl/opcode_table.h"
#include "goldbox/ecl/runtime_layout.h"
#include "goldbox/core/vm_layout.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {

using ECL::EclVM;

// Cast from the generic SyscallHandler* the VM passes to the richer EclEngineHost*.
// Safe because every poolrad opcode handler is only ever called with a
// PoolradEngineHostImpl, which derives from EclEngineHost.
static inline ECL::EclEngineHost *asHost(ECL::SyscallHandler *s) {
    return static_cast<ECL::EclEngineHost *>(s);
}

// -------------------------------------------------------------------------
// Static state for FOR loop (not nested; matches Java VirtualMachine behavior).
// -------------------------------------------------------------------------
static uint16 g_forLoopBodyStart = 0;
static uint16 g_forLoopCount = 0;
static uint16 g_forLoopMax = 0;

// -------------------------------------------------------------------------
// Legacy extension-opcode scaffold (0x3E-0x4C).
// -------------------------------------------------------------------------

// 0x3E: NPC REMOVE
static int handle_0x3E_NPC_REMOVE(EclVM &vm, ECL::AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    return asHost(syscalls)->removeNpc();
}

// 0x3F: HAS EFFECT <effectID>
static int handle_0x3F_HAS_EFFECT(EclVM &vm, ECL::AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(1);
    uint8 effectId = (uint8)vm.readVar(1);
    uint16 resultAddr = ECL::getOpcodeLayout().runtimeField(ECL::kEclRuntimeSelectedCharPtr);
    return asHost(syscalls)->hasEffect(effectId, resultAddr);
}

// 0x40: DESTROY ITEM <itemID>
static int handle_0x40_DESTROY_ITEM(EclVM &vm, ECL::AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(1);
    uint8 itemId = (uint8)vm.readVar(1);
    return asHost(syscalls)->destroyItem(itemId);
}

// 0x41: GIVE EXP <amount> <divideFlag>
static int handle_0x41_GIVE_EXP(EclVM &vm, ECL::AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(2);
    uint16 amount    = vm.readVar(1);
    uint8 divideFlag = (uint8)vm.readVar(2);
    return asHost(syscalls)->giveExperience(amount, divideFlag);
}

// 0x42: STOP MOVE
static int handle_0x42_STOP_MOVE(EclVM &vm, ECL::AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_HALTED;
    return asHost(syscalls)->stopMove();
}

// 0x43: SOUND EVENT <soundID>
static int handle_0x43_SOUND(EclVM &vm, ECL::AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    (void)mem; (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(1);
    uint8 soundId = (uint8)vm.readVar(1);
    return asHost(syscalls)->playSoundEvent(soundId);
}

// 0x44: (unknown 0 args)
static int handle_0x44_UNKNOWN(EclVM &vm, ECL::AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x45: RANDOM0 <destAddr> <maxVal>
static int handle_0x45_RANDOM0(EclVM &vm, ECL::AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    uint16 destAddr = vm.getOpWord(1);
    uint16 maxVal = vm.readVar(2);
    uint16 result = (maxVal > 0) ? (uint16)ECL::getOpcodeRandom().getRandomNumber(maxVal) : 0;
    mem.write16LE(destAddr, result);
    return VM_OK;
}

// 0x46: FOR START <initVal> <maxVal>
static int handle_0x46_FOR_START(EclVM &vm, ECL::AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    (void)mem; (void)callStack; (void)syscalls;
    vm.getOperand(2);
    g_forLoopCount = vm.readVar(1);
    g_forLoopMax   = vm.readVar(2);
    g_forLoopBodyStart = nextPc;
    return VM_OK;
}

// 0x47: FOR REPEAT
static int handle_0x47_FOR_REPEAT(EclVM &vm, ECL::AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)callStack; (void)syscalls;
    g_forLoopCount++;
    if (g_forLoopCount <= g_forLoopMax)
        nextPc = g_forLoopBodyStart;
    return VM_OK;
}

// 0x48: (unknown, 1 arg)
static int handle_0x48_UNKNOWN(EclVM &vm, ECL::AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x49: (unknown, 6 args)
static int handle_0x49_UNKNOWN(EclVM &vm, ECL::AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x4A: (unknown, 0 args)
static int handle_0x4A_UNKNOWN(EclVM &vm, ECL::AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x4B: (unknown, 1 arg)
static int handle_0x4B_UNKNOWN(EclVM &vm, ECL::AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    (void)vm; (void)mem; (void)nextPc; (void)callStack; (void)syscalls;
    return VM_OK;
}

// 0x4C: PICTURE 2 <pictureID> <variant>
static int handle_0x4C_PICTURE2(EclVM &vm, ECL::AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    (void)nextPc; (void)callStack;
    if (!syscalls)
        return VM_ERROR;
    vm.getOperand(2);
    uint8 picId = (uint8)vm.readVar(1);
    uint8 variant = (uint8)vm.readVar(2);
    mem.write8(ECL::getOpcodeLayout().vmGlobalField(Goldbox::kVmGlobalFieldPictureHeadId).vmAddr, picId);
    return asHost(syscalls)->displayPicture2(picId, variant);
}

// -------------------------------------------------------------------------
// Opcode table entries for Pool of Radiance-specific opcodes
// -------------------------------------------------------------------------
static ECL::OperandType kOpsNone[]      = { ECL::OperandType::NONE };
static ECL::OperandType kOpsVal8[]      = { ECL::OperandType::VAL8, ECL::OperandType::NONE };
static ECL::OperandType kOpsVal8Val8[]  = { ECL::OperandType::VAL8, ECL::OperandType::VAL8,
    ECL::OperandType::NONE };
static ECL::OperandType kOpsVal8x5[]    = { ECL::OperandType::VAL8, ECL::OperandType::VAL8,
    ECL::OperandType::VAL8, ECL::OperandType::VAL8, ECL::OperandType::VAL8,
    ECL::OperandType::NONE };

static ECL::OpcodeInfo poolradOpcodeTable[] = {
    { 0x3E, "NPC REMOVE",    kOpsNone,     "Remove NPC from party" },
    { 0x3F, "HAS EFFECT",    kOpsVal8,     "Check active party-wide effect" },
    { 0x40, "DESTROY ITEM",  kOpsVal8,     "Remove item from inventory" },
    { 0x41, "GIVE EXP",      kOpsVal8Val8, "Award experience to party" },
    { 0x42, "STOP MOVE",     kOpsNone,     "Stop movement and redraw" },
    { 0x43, "SOUND EVENT",   kOpsVal8,     "Play sound effect" },
    { 0x44, "UNKNOWN",       kOpsNone,     "Unknown opcode" },
    { 0x45, "RANDOM0",       kOpsVal8Val8, "Random range to destination" },
    { 0x46, "FOR START",     kOpsVal8Val8, "Start counted loop" },
    { 0x47, "FOR REPEAT",    kOpsNone,     "Repeat loop body" },
    { 0x48, "UNKNOWN",       kOpsVal8,     "Unknown opcode" },
    { 0x49, "UNKNOWN",       kOpsVal8x5,   "Unknown opcode" },
    { 0x4A, "UNKNOWN",       kOpsNone,     "Unknown opcode" },
    { 0x4B, "UNKNOWN",       kOpsVal8,     "Unknown opcode" },
    { 0x4C, "PICTURE2",      kOpsVal8Val8, "Extended picture display with variant parameter." }
};

// -------------------------------------------------------------------------
// Registration
// -------------------------------------------------------------------------

void registerPoolradOpcodeHandlers() {
    // Register opcode table entries so the decoder can decode these opcodes.
    for (uint32 i = 0; i < ARRAYSIZE(poolradOpcodeTable); ++i)
        ECL::registerOpcodeInfo(poolradOpcodeTable[i].opcode, &poolradOpcodeTable[i]);

    // Register handler functions.
    ECL::registerOpcodeHandler(0x3E, handle_0x3E_NPC_REMOVE);
    ECL::registerOpcodeHandler(0x3F, handle_0x3F_HAS_EFFECT);
    ECL::registerOpcodeHandler(0x40, handle_0x40_DESTROY_ITEM);
    ECL::registerOpcodeHandler(0x41, handle_0x41_GIVE_EXP);
    ECL::registerOpcodeHandler(0x42, handle_0x42_STOP_MOVE);
    ECL::registerOpcodeHandler(0x43, handle_0x43_SOUND);
    ECL::registerOpcodeHandler(0x44, handle_0x44_UNKNOWN);
    ECL::registerOpcodeHandler(0x45, handle_0x45_RANDOM0);
    ECL::registerOpcodeHandler(0x46, handle_0x46_FOR_START);
    ECL::registerOpcodeHandler(0x47, handle_0x47_FOR_REPEAT);
    ECL::registerOpcodeHandler(0x48, handle_0x48_UNKNOWN);
    ECL::registerOpcodeHandler(0x49, handle_0x49_UNKNOWN);
    ECL::registerOpcodeHandler(0x4A, handle_0x4A_UNKNOWN);
    ECL::registerOpcodeHandler(0x4B, handle_0x4B_UNKNOWN);
    ECL::registerOpcodeHandler(0x4C, handle_0x4C_PICTURE2);
}

} // namespace Poolrad
} // namespace Goldbox
