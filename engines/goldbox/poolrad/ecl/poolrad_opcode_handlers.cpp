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
#include "goldbox/ecl/opcode_handlers.h"
#include "goldbox/ecl/opcode_table.h"
#include "goldbox/ecl/runtime_layout.h"
#include "goldbox/core/vm_layout.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {

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
// Not currently registered by PoolradGameConfig because Pool of Radiance ends
// at opcode 0x3D.
// -------------------------------------------------------------------------

// 0x3E: NPC REMOVE
// Removes the currently loaded NPC from the party roster.
static int handle_0x3E_NPC_REMOVE(ECL::AddressSpace &mem, const ECL::EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    if (!syscalls)
        return VM_ERROR;
    return asHost(syscalls)->removeNpc();
}

// 0x3F: HAS EFFECT <effectID>
// PoR variant (1 arg): tests whether a party-wide effect is active.
// Sets compare EQ if effect present, NE if not.
static int handle_0x3F_HAS_EFFECT(ECL::AddressSpace &mem, const ECL::EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    if (insn.operands.empty() || !syscalls)
        return VM_ERROR;
    uint8 effectId = insn.operands[0].u8;
    uint16 resultAddr = ECL::getOpcodeLayout().runtimeField(ECL::kEclRuntimeSelectedCharPtr);
    return asHost(syscalls)->hasEffect(effectId, resultAddr);
}

// 0x40: DESTROY ITEM <itemID>
// Removes a specific item from the party inventory.
static int handle_0x40_DESTROY_ITEM(ECL::AddressSpace &mem, const ECL::EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    if (insn.operands.empty() || !syscalls)
        return VM_ERROR;
    uint8 itemId = insn.operands[0].u8;
    return asHost(syscalls)->destroyItem(itemId);
}

// 0x41: GIVE EXP <amount> <divideFlag>
// Awards experience to all party members.
// arg0 = base XP amount, arg1 = how to split (0 = each, 1 = divide by party size).
static int handle_0x41_GIVE_EXP(ECL::AddressSpace &mem, const ECL::EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    if (insn.operands.size() < 2 || !syscalls)
        return VM_ERROR;
    uint16 amount     = ECL::resolveVar(mem, insn.operands[0]);
    uint8 divideFlag  = (uint8)ECL::resolveVar(mem, insn.operands[1]);
    return asHost(syscalls)->giveExperience(amount, divideFlag);
}

// 0x42: STOP MOVE (variant B, 0 args)
// Identical to 0x23 STOP_MOVE: halts VM, updates position, clears display.
static int handle_0x42_STOP_MOVE(ECL::AddressSpace &mem, const ECL::EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    if (!syscalls)
        return VM_HALTED;
    return asHost(syscalls)->stopMove();
}

// 0x43: SOUND EVENT <soundID>
static int handle_0x43_SOUND(ECL::AddressSpace &mem, const ECL::EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    if (insn.operands.empty() || !syscalls)
        return VM_ERROR;
    uint8 soundId = insn.operands[0].u8;
    return asHost(syscalls)->playSoundEvent(soundId);
}

// 0x44: (unknown 0 args)
static int handle_0x44_UNKNOWN(ECL::AddressSpace &mem, const ECL::EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    return VM_OK;
}

// 0x45: RANDOM0 <destAddr> <maxVal>
// Writes random(0..maxVal) to destAddr; writes 0 if maxVal == 0.
// Differs from 0x08 RANDOM in that dest is arg0 and max is arg1 (reversed).
static int handle_0x45_RANDOM0(ECL::AddressSpace &mem, const ECL::EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    if (insn.operands.size() < 2)
        return VM_ERROR;
    uint16 destAddr = insn.operands[0].u16;
    uint16 maxVal = ECL::resolveVar(mem, insn.operands[1]);
    uint16 result = (maxVal > 0) ? (uint16)ECL::getOpcodeRandom().getRandomNumber(maxVal) : 0;
    mem.write16LE(destAddr, result);
    return VM_OK;
}

// 0x46: FOR START <initVal> <maxVal>
// Starts a counted loop. Loop body begins at the instruction immediately following.
// Original: stores loop counter in a dedicated var; loop runs while counter <= maxVal.
static int handle_0x46_FOR_START(ECL::AddressSpace &mem, const ECL::EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    if (insn.operands.size() < 2)
        return VM_ERROR;
    g_forLoopCount = (uint16)ECL::resolveVar(mem, insn.operands[0]);
    g_forLoopMax   = (uint16)ECL::resolveVar(mem, insn.operands[1]);
    g_forLoopBodyStart = nextPc;
    return VM_OK;
}

// 0x47: FOR REPEAT
// Increments counter; jumps back to loop body if counter <= maxVal.
static int handle_0x47_FOR_REPEAT(ECL::AddressSpace &mem, const ECL::EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    g_forLoopCount++;
    if (g_forLoopCount <= g_forLoopMax)
        nextPc = g_forLoopBodyStart;
    return VM_OK;
}

// 0x48: (unknown, 1 arg)
static int handle_0x48_UNKNOWN(ECL::AddressSpace &mem, const ECL::EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    return VM_OK;
}

// 0x49: (unknown, 6 args)
static int handle_0x49_UNKNOWN(ECL::AddressSpace &mem, const ECL::EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    return VM_OK;
}

// 0x4A: (unknown, 0 args)
static int handle_0x4A_UNKNOWN(ECL::AddressSpace &mem, const ECL::EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    return VM_OK;
}

// 0x4B: (unknown, 1 arg)
static int handle_0x4B_UNKNOWN(ECL::AddressSpace &mem, const ECL::EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    return VM_OK;
}

// 0x4C: PICTURE 2 <pictureID> <variant>
// Extended picture display with variant parameter.
static int handle_0x4C_PICTURE2(ECL::AddressSpace &mem, const ECL::EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, ECL::SyscallHandler *syscalls) {
    if (insn.operands.size() < 2 || !syscalls)
        return VM_ERROR;
    uint8 picId = insn.operands[0].u8;
    uint8 variant = (uint8)ECL::resolveVar(mem, insn.operands[1]);
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
