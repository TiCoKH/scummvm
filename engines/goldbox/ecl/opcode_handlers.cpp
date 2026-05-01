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
#include "common/hashmap.h"
#include "common/random.h"
#include "goldbox/vm_interface.h"
#include "goldbox/ecl/game_config.h"
#include "goldbox/ecl/runtime_layout.h"
#include "goldbox/poolrad/data/poolrad_vm_layout.h"

namespace Goldbox {
namespace ECL {

// Use a typedef alias for easier reference
using HandlerMap = Common::HashMap<uint8, OpcodeHandler>;
static HandlerMap g_handlers;
static Common::RandomSource g_random("eclvm");

static const EclLayoutAccess &getOpcodeLayout() {
    static EclLayoutAccess s_layout(
        Goldbox::Poolrad::Data::getPoolradVmLayout(),
        Goldbox::Poolrad::Data::getPoolradGlobalVmLayout(),
        Goldbox::Poolrad::Data::getPoolradEclRuntimeLayout()
    );
    return s_layout;
}

uint16 resolveVar(AddressSpace &mem, const EclOperand &operand) {
    if (operand.type == OperandType::VAL8) {
        return operand.u8;
    } else if (operand.type == OperandType::VAL16) {
        return operand.u16;
    } else if (operand.type == OperandType::ADDR16) {
        // Dereference address
        return mem.read16LE(operand.u16);
    }
    return 0;
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

static int handle_0x00_EXIT(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    return VM_HALTED;
}

static int handle_0x01_GOTO(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    OperandReader reader(insn);
    uint16 addr = reader.readAddr();
    nextPc = addr;
    return VM_OK;
}

static int handle_0x02_GOSUB(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    callStack.push_back(nextPc);
    OperandReader reader(insn);
    uint16 addr = reader.readAddr();
    nextPc = addr;
    return VM_OK;
}

// 0x03: COMPARE <var1> <var2>
// Sets comparison flags for subsequent IF opcodes.
// Original: VM_SetIntCompareFlags(arg1, arg0) sets 6 bool globals (EQ,NE,LT,GT,LE,GE).
// String variant: compares via Pascal string comparison operators.
static int handle_0x03_COMPARE(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 2)
        return VM_ERROR;
    uint16 a0 = resolveVar(mem, insn.operands[0]);
    uint16 a1 = resolveVar(mem, insn.operands[1]);
    // Unsigned word subtraction matching original word comparison semantics.
    setCmpResult(mem, (int32)a0 - (int32)a1);
    return VM_OK;
}

static int handle_0x04_ADD(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3) {
        return VM_ERROR;
    }
    uint16 var1 = resolveVar(mem, insn.operands[0]);
    uint16 var2 = resolveVar(mem, insn.operands[1]);
    uint16 addr = insn.operands[2].u16;
    mem.write16LE(addr, (var1 + var2) & 0xFFFF);
    return VM_OK;
}

static int handle_0x05_SUBTRACT(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3) {
        return VM_ERROR;
    }
    uint16 var1 = resolveVar(mem, insn.operands[0]);
    uint16 var2 = resolveVar(mem, insn.operands[1]);
    uint16 addr = insn.operands[2].u16;
    // Note: Pool of Radiance subtracts var1 from var2
    mem.write16LE(addr, (var2 - var1) & 0xFFFF);
    return VM_OK;
}

static int handle_0x06_DIVIDE(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3) {
        return VM_ERROR;
    }
    uint16 var1 = resolveVar(mem, insn.operands[0]);
    uint16 var2 = resolveVar(mem, insn.operands[1]);
    uint16 addr = insn.operands[2].u16;
    if (var2 == 0) {
        return VM_ERROR; // Division by zero
    }
    mem.write16LE(addr, var1 / var2);
    return VM_OK;
}

static int handle_0x07_MULTIPLY(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3) {
        return VM_ERROR;
    }
    uint16 var1 = resolveVar(mem, insn.operands[0]);
    uint16 var2 = resolveVar(mem, insn.operands[1]);
    uint16 addr = insn.operands[2].u16;
    mem.write16LE(addr, (var1 * var2) & 0xFFFF);
    return VM_OK;
}

static int handle_0x09_SAVE(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 2) {
        return VM_ERROR;
    }
    uint16 val = resolveVar(mem, insn.operands[0]);
    uint16 addr = insn.operands[1].u16;
    mem.write16LE(addr, val);
    return VM_OK;
}

static int handle_0x11_PRINT(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (!syscalls || insn.operands.empty()) {
        return VM_ERROR;
    }
    Common::String text = insn.operands[0].str;
    syscalls->printText(text, false);
    return VM_OK;
}

static int handle_0x12_PRINTCLEAR(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (!syscalls || insn.operands.empty()) {
        return VM_ERROR;
    }
    Common::String text = insn.operands[0].str;
    syscalls->printText(text, true);
    return VM_OK;
}

static int handle_0x13_RETURN(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (callStack.empty()) {
        return VM_ERROR; // Stack underflow
    }
    nextPc = callStack.back();
    callStack.pop_back();
    return VM_OK;
}

// 0x2F: AND <var1> <var2> <destAddr>
// Original: stores result AND sets compare flags (EQ if result==0, GT if result!=0).
static int handle_0x2F_AND(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3)
        return VM_ERROR;
    uint16 var1 = resolveVar(mem, insn.operands[0]);
    uint16 var2 = resolveVar(mem, insn.operands[1]);
    uint16 addr = insn.operands[2].u16;
    uint16 result = var1 & var2;
    mem.write16LE(addr, result);
    setCmpResult(mem, result == 0 ? 0 : 1);
    return VM_OK;
}

// 0x30: OR <var1> <var2> <destAddr>
// Original: stores result AND sets compare flags (EQ if result==0, GT if result!=0).
static int handle_0x30_OR(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3)
        return VM_ERROR;
    uint16 var1 = resolveVar(mem, insn.operands[0]);
    uint16 var2 = resolveVar(mem, insn.operands[1]);
    uint16 addr = insn.operands[2].u16;
    uint16 result = var1 | var2;
    mem.write16LE(addr, result);
    setCmpResult(mem, result == 0 ? 0 : 1);
    return VM_OK;
}

static int handle_0x08_RANDOM(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 2) {
        return VM_ERROR;
    }
    uint16 maxVal = resolveVar(mem, insn.operands[0]);
    uint16 addr = insn.operands[1].u16;
    uint16 randVal = (uint16)g_random.getRandomNumber(maxVal);
    mem.write16LE(addr, randVal);
    return VM_OK;
}

static int handle_0x0F_INPUT_NUMBER(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (!syscalls || insn.operands.size() < 2) {
        return VM_ERROR;
    }
    uint8 maxDigits = insn.operands[0].u8;
    uint16 addr = insn.operands[1].u16;
    syscalls->inputNumber(maxDigits, addr);
    return VM_OK;
}

static int handle_0x10_INPUT_STRING(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (!syscalls || insn.operands.size() < 2) {
        return VM_ERROR;
    }
    uint8 maxLength = insn.operands[0].u8;
    uint16 addr = insn.operands[1].u16;
    VmResult result = syscalls->inputString(maxLength, addr);
    return result;
}

// 0x0A: LOAD CHARACTER <var>
// Selects a party member or monster for subsequent character-stat references.
static int handle_0x0A_LOAD_CHARACTER(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 1) {
        return VM_ERROR;
    }
    uint8 sel = insn.operands[0].u8;
        uint8 partySize = mem.read8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldPartyCount).vmAddr);

    // Store selection index for engine/UI reference
        mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldSelectedPcIndex).vmAddr, sel);

    if (sel < 128) {
        if (sel < partySize) {
            // Point selected character pointer to this member's data block
                uint16 base = ECLMemoryLayout::OFFSET_CHARACTER_DATA + sel * ECLMemoryLayout::CHARACTER_DATA_SIZE;
                mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeSelectedCharPtr), base);
        } else {
            // Select a monster from the list (index after party members)
            uint16 monsterIndex = sel - partySize;
            // Without a defined monster record size, point to start for now
                uint16 base = getOpcodeLayout().runtimeField(kEclRuntimeMonsterData); // TODO: add record size and index into list
            (void)monsterIndex;
                mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeSelectedCharPtr), base);
        }
    } else {
        // Monster (sel - 128) will be placed on party side in next combat
        // Record selection; engine should interpret this when starting combat
            uint16 base = getOpcodeLayout().runtimeField(kEclRuntimeMonsterData); // Placeholder pointer
            mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeSelectedCharPtr), base);
            mem.write8(getOpcodeLayout().runtimeField(kEclRuntimeMenuCombatState), 1); // Mark pending special placement
    }
    return VM_OK;
}

// 0x0B: LOAD MONSTER <monsterID> <count> <graphicID>
// Adds <count> copies of monster <monsterID> to the monster list.
static int handle_0x0B_LOAD_MONSTER(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3) {
        return VM_ERROR;
    }
    uint16 monsterId = resolveVar(mem, insn.operands[0]);
    uint16 count = resolveVar(mem, insn.operands[1]);
    uint16 graphicId = resolveVar(mem, insn.operands[2]);

    // Update monster count; actual list population will be handled by the engine
    uint16 currentCount = mem.read16LE(getOpcodeLayout().runtimeField(kEclRuntimeMonsterCount));
    uint16 newCount = (currentCount + (count & 0xFF)) & 0xFFFF;
    mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeMonsterCount), newCount);

    // Stash last loaded monster info in encounter flags area for engine consumption
    mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeEncounterFlags), (monsterId & 0xFF));
    (void)graphicId; // Graphic mapping handled by picture system later
    return VM_OK;
}

// 0x0C: SETUP MONSTER <monsterID> <distance> <graphicID>
// Starts a monster encounter at <distance> (0-2) squares away.
static int handle_0x0C_SETUP_MONSTER(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3 || !syscalls) {
        return VM_ERROR;
    }
    uint16 monsterId = resolveVar(mem, insn.operands[0]);
    uint16 distance = resolveVar(mem, insn.operands[1]);
    uint16 graphicId = resolveVar(mem, insn.operands[2]);

    // Clamp distance to 0..2
    uint8 dist = (distance > 2) ? 2 : (uint8)distance;
    mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldMonsterDistance).vmAddr, dist);

    // Mark game state as combat and store primary monster ID
    mem.write8(getOpcodeLayout().runtimeField(kEclRuntimeGameState), (uint8)GS_COMBAT);
    mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeEncounterFlags), (monsterId & 0xFF));
    (void)graphicId;

    VmResult r = syscalls->startCombat();
    return r;
}

// 0x0D: APPROACH
// Monsters from SETUP MONSTER close distance by 1 square.
static int handle_0x0D_APPROACH(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
        uint8 dist = mem.read8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldMonsterDistance).vmAddr);
    if (dist > 0) {
        dist -= 1;
            mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldMonsterDistance).vmAddr, dist);
    }
    return VM_OK;
}

// 0x0E: PICTURE <pictureID>
// Displays a picture or ends graphical display (255).
static int handle_0x0E_PICTURE(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 1 || !syscalls) {
        return VM_ERROR;
    }
    uint8 picId = insn.operands[0].u8;
    mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldPictureHeadId).vmAddr, picId);

    VmResult r = syscalls->displayPicture(picId);
    return r;
}

static int handle_0x33_PRINT_RETURN(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (!syscalls) {
        return VM_ERROR;
    }
    syscalls->printText(Common::String(""), false);
    return VM_OK;
}

static int handle_0x3A_DELAY(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    return VM_YIELD;
}

static int handle_0x38_PROGRAM(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 1 || !syscalls) {
        return VM_ERROR;
    }
    uint8 program = insn.operands[0].u8;
    VmResult result = syscalls->executeProgram(program);
    return result;
}

static int handle_0x3C_PROTECTION(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 1 || !syscalls) {
        return VM_ERROR;
    }
    uint16 addr = insn.operands[0].u16;
    Common::String runes;
    uint8 ch;
    while ((ch = mem.read8(addr++)) != 0) {
        runes += (char)ch;
    }
    syscalls->printText(runes, false);
    return VM_OK;
}

static int handle_0x3D_CLEAR_BOX(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (!syscalls) {
        return VM_ERROR;
    }
    VmResult result = syscalls->clearTextBox();
    return result;
}

// 0x14: COMPARE AND <var1> <var2> <var3> <var4>
// Original: calls VM_SetIntCompareFlags twice; result is EQ if BOTH pairs are equal.
// Only EQ/NE flags are meaningful after this opcode (no magnitude comparison).
static int handle_0x14_COMPARE_AND(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 4)
        return VM_ERROR;
    uint16 var1 = resolveVar(mem, insn.operands[0]);
    uint16 var2 = resolveVar(mem, insn.operands[1]);
    uint16 var3 = resolveVar(mem, insn.operands[2]);
    uint16 var4 = resolveVar(mem, insn.operands[3]);
    bool bothEqual = (var1 == var2) && (var3 == var4);
    setCmpResult(mem, bothEqual ? 0 : 1);
    return VM_OK;
}

// 0x15: VERTICAL MENU <address> <message> <count> <stringVarargs>
static int handle_0x15_VERTICAL_MENU(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3 || !syscalls) {
        return VM_ERROR;
    }
    uint16 resultAddr = insn.operands[0].u16;
    Common::String message = insn.operands[1].str;
    uint8 count = insn.operands[2].u8;
    
    Common::Array<Common::String> options;
    for (int i = 0; i < count && (3 + i) < (int)insn.operands.size(); ++i) {
        options.push_back(insn.operands[3 + i].str);
    }
    
    return syscalls->verticalMenu(message, options, resultAddr);
}

// 0x16-0x1B: IF commands — execute the next instruction only if condition holds.
// Original: each opcode tests one of BOOL_EQ/NE/LT/GT/LE/GE_FLAG set by COMPARE.
// When condition is false the NEXT decoded instruction is skipped (nextPc += 1).
static int handleIF(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, uint8 opcode) {
    int8 cmp = getCmpResult(mem);
    bool cond = false;
    switch (opcode) {
    case 0x16: cond = (cmp == 0); break;  // IF =  : EQ_FLAG
    case 0x17: cond = (cmp != 0); break;  // IF <> : NE_FLAG
    case 0x18: cond = (cmp <  0); break;  // IF <  : LT_FLAG
    case 0x19: cond = (cmp >  0); break;  // IF >  : GT_FLAG
    case 0x1A: cond = (cmp <= 0); break;  // IF <= : LE_FLAG
    case 0x1B: cond = (cmp >= 0); break;  // IF >= : GE_FLAG
    }
    if (!cond)
        nextPc += 1; // skip next instruction
    return VM_OK;
}

static int handle_0x16_IF_EQUAL(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    return handleIF(mem, insn, nextPc, callStack, 0x16);
}

static int handle_0x17_IF_NOT_EQUAL(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    return handleIF(mem, insn, nextPc, callStack, 0x17);
}

static int handle_0x18_IF_LESS(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    return handleIF(mem, insn, nextPc, callStack, 0x18);
}

static int handle_0x19_IF_GREATER(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    return handleIF(mem, insn, nextPc, callStack, 0x19);
}

static int handle_0x1A_IF_LESS_EQUAL(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    return handleIF(mem, insn, nextPc, callStack, 0x1A);
}

static int handle_0x1B_IF_GREATER_EQUAL(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    return handleIF(mem, insn, nextPc, callStack, 0x1B);
}

// 0x1C: CLEARMONSTERS
static int handle_0x1C_CLEARMONSTERS(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    mem.write16LE(getOpcodeLayout().runtimeField(kEclRuntimeMonsterCount), 0);
    return VM_OK;
}

// 0x1D: PARTYSTRENGTH <address>
static int handle_0x1D_PARTYSTRENGTH(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 1) {
        return VM_ERROR;
    }
    uint16 addr = insn.operands[0].u16;
    // TODO: Calculate party strength based on character levels/stats
    // For now, placeholder value
    mem.write16LE(addr, 100);
    return VM_OK;
}

// 0x1E: CHECKPARTY <attributeAddress> <effectID> <unknown> <address1> <unknown> <address2>
static int handle_0x1E_CHECKPARTY(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 6) {
        return VM_ERROR;
    }
    uint16 attributeAddr = resolveVar(mem, insn.operands[0]);
    uint16 effectID = resolveVar(mem, insn.operands[1]);
    uint16 addr1 = insn.operands[3].u16;
    uint16 addr2 = insn.operands[5].u16;
    
    if (attributeAddr != 0 && effectID == 0) {
        // Check attribute values
        mem.write16LE(addr1, 18); // Placeholder highest
        mem.write16LE(addr2, 3);  // Placeholder lowest
    } else if (attributeAddr == 0 && effectID != 0) {
        // Check for effect
        mem.write16LE(addr2, 0); // Placeholder: no characters have effect
    }
    return VM_OK;
}

// 0x20: NEWECL <script>
static int handle_0x20_NEWECL(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 1 || !syscalls) {
        return VM_ERROR;
    }
    uint8 scriptId = insn.operands[0].u8;
    return syscalls->loadScript(scriptId);
}

// 0x21: LOAD FILES <geoBlockId> <unused> <iconTrigger>
// Loads the dungeon GEO map block (indoor) or outdoor icon strip (outdoor).
// Mirrors the GEO branch of INSTR_LoadAreaDeco from the original.
static int handle_0x21_LOAD_FILES(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3 || !syscalls)
        return VM_ERROR;

    // Three params match abStack_8[1..3] in INSTR_LoadAreaDeco
    uint8 geoBlockId  = (uint8)resolveVar(mem, insn.operands[0]); // abStack_8[1]
    // insn.operands[1] (abStack_8[2]) is fetched but unused in the GEO branch
    uint8 iconTrigger = (uint8)resolveVar(mem, insn.operands[2]); // abStack_8[3]

    // IndoorModeFlag at +0x1CC: non-zero = dungeon/indoor, zero = outdoor/city
    bool indoorMode = (mem.read8(
        getOpcodeLayout().vmField(kVmFieldIndoorModeFlag).vmAddr) != 0);

    // Load GEO block only in indoor mode; 0xFF = skip, 0x7F = special stub
    if (geoBlockId != 0xFF && geoBlockId != 0x7F && indoorMode) {
        mem.write8(getOpcodeLayout().vmField(kVmFieldGeoBlockId).vmAddr,
            geoBlockId);
        syscalls->loadGeoBlock(geoBlockId);
        // Reset movement-block counter after loading new map
        mem.write8(
            getOpcodeLayout().vmGlobalField(kVmGlobalFieldMovementBlock).vmAddr,
            0);
    }

    // Load outdoor icon strip when not in indoor mode and trigger is valid
    if (iconTrigger != 0xFF && !indoorMode)
        syscalls->loadIconBlock();

    return VM_OK;
}

// 0x22: PARTY SURPRISE <address1> <address2>
static int handle_0x22_PARTY_SURPRISE(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 2) {
        return VM_ERROR;
    }
    uint16 addr1 = insn.operands[0].u16;
    uint16 addr2 = insn.operands[1].u16;
    mem.write16LE(addr1, 2); // Monster surprise roll
    mem.write16LE(addr2, 2); // Party surprise roll
    return VM_OK;
}

// 0x23: SURPRISE <address1> <address2> <var1> <var2>
static int handle_0x23_SURPRISE(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 4) {
        return VM_ERROR;
    }
    uint16 addr1 = insn.operands[0].u16;
    uint16 addr2 = insn.operands[1].u16;
    uint16 mod1 = resolveVar(mem, insn.operands[2]);
    uint16 mod2 = resolveVar(mem, insn.operands[3]);
    
    // TODO: Roll dice and calculate surprise
    // For now, set to 0 (neither side surprised)
    mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldCombatIsAmbush).vmAddr, 0);
    return VM_OK;
}

// 0x24: COMBAT
static int handle_0x24_COMBAT(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (!syscalls) {
        return VM_ERROR;
    }
    
    uint8 templeFlag = mem.read8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldEnterTemplePending).vmAddr);
    uint8 shopFlag = mem.read8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldShopFlag).vmAddr);
    
    if (templeFlag == 1) {
        mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldEnterTemplePending).vmAddr, 0);
        // TODO: Enter temple
        return VM_OK;
    } else if (shopFlag == 1) {
        mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldShopFlag).vmAddr, 0);
        // TODO: Enter shop
        return VM_OK;
    } else {
        return syscalls->startCombat();
    }
}

// 0x25: ON GOTO <var> <count> <addressVarargs>
static int handle_0x25_ON_GOTO(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 2) {
        return VM_ERROR;
    }
    uint16 var = resolveVar(mem, insn.operands[0]);
    uint8 count = insn.operands[1].u8;
    
    if (var < count && (2 + var) < insn.operands.size()) {
        nextPc = insn.operands[2 + var].u16;
    }
    return VM_OK;
}

// 0x26: ON GOSUB <var> <count> <addressVarargs>
static int handle_0x26_ON_GOSUB(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 2) {
        return VM_ERROR;
    }
    uint16 var = resolveVar(mem, insn.operands[0]);
    uint8 count = insn.operands[1].u8;
    
    if (var < count && (2 + var) < insn.operands.size()) {
        callStack.push_back(nextPc);
        nextPc = insn.operands[2 + var].u16;
    }
    return VM_OK;
}

// 0x27: TREASURE <copper> <silver> <electrum> <gold> <platinum> <gems> <jewelry> <treasureID>
static int handle_0x27_TREASURE(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    // TODO: Store treasure data for next combat
    return VM_OK;
}

// 0x28: ROB <isWholeParty> <percentMoney> <itemChance>
static int handle_0x28_ROB(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3) {
        return VM_ERROR;
    }
    // TODO: Implement party robbery logic
    return VM_OK;
}

// 0x29: ENCOUNTER MENU (varargs)
static int handle_0x29_ENCOUNTER_MENU(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    // TODO: Implement encounter menu with distance and messages
    return VM_OK;
}

// 0x2A: GETTABLE <address1> <var> <address2>
static int handle_0x2A_GETTABLE(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3) {
        return VM_ERROR;
    }
    uint16 baseAddr = insn.operands[0].u16;
    uint16 index = resolveVar(mem, insn.operands[1]);
    uint16 destAddr = insn.operands[2].u16;
    
    uint16 value = mem.read16LE(baseAddr + index);
    mem.write16LE(destAddr, value);
    // Set compare flags: EQ if zero, GT if nonzero (mirrors AND/OR convention).
    setCmpResult(mem, value == 0 ? 0 : 1);
    return VM_OK;
}

// 0x2B: HORIZONTAL MENU <address> <count> <stringVarargs>
static int handle_0x2B_HORIZONTAL_MENU(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 2 || !syscalls) {
        return VM_ERROR;
    }
    uint16 resultAddr = insn.operands[0].u16;
    uint8 count = insn.operands[1].u8;
    
    Common::Array<Common::String> options;
    for (int i = 0; i < count && (2 + i) < (int)insn.operands.size(); ++i) {
        options.push_back(insn.operands[2 + i].str);
    }
    
    return syscalls->horizontalMenu(options, resultAddr);
}

// 0x2C: PARLAY <haughty> <sly> <nice> <meek> <abusive> <address>
static int handle_0x2C_PARLAY(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 6) {
        return VM_ERROR;
    }
    uint16 resultAddr = insn.operands[5].u16;
    // TODO: Show parlay dialog, store selected attitude value
    mem.write16LE(resultAddr, 0);
    return VM_OK;
}

// 0x2D: CALL <address>
static int handle_0x2D_CALL(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    // Machine code calls not supported in ScummVM
    return VM_OK;
}

// 0x2E: DAMAGE <var1> <dice> <sides> <bonus> <var2>
static int handle_0x2E_DAMAGE(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 5) {
        return VM_ERROR;
    }
    // TODO: Implement damage calculation and application
    return VM_OK;
}

// 0x31: SPRITE OFF
static int handle_0x31_SPRITE_OFF(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    mem.write8(getOpcodeLayout().runtimeField(kEclRuntimeSpriteState), 0);
    return VM_OK;
}

// 0x32: FIND ITEM <itemID>
static int handle_0x32_FIND_ITEM(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 1) {
        return VM_ERROR;
    }
    uint8 itemId = insn.operands[0].u8;
    // TODO: Search party inventory for item.
    // Original: sets NE_FLAG (not-found = cmp != 0); founder sets EQ_FLAG.
    setCmpResult(mem, 1); // not found (GT): IF_EQUAL skips, IF_NOT_EQUAL continues
    return VM_OK;
}

// 0x34: ECL CLOCK <var> <timeunit>
static int handle_0x34_ECL_CLOCK(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    // Not used in Pool of Radiance
    return VM_OK;
}

// 0x35: SAVE TABLE <var1> <address> <var2>
static int handle_0x35_SAVE_TABLE(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3) {
        return VM_ERROR;
    }
    uint16 value = resolveVar(mem, insn.operands[0]);
    uint16 baseAddr = insn.operands[1].u16;
    uint16 index = resolveVar(mem, insn.operands[2]);
    
    mem.write16LE(baseAddr + index, value);
    return VM_OK;
}

// 0x36: ADD NPC <monsterID> <morale>
static int handle_0x36_ADD_NPC(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 2) {
        return VM_ERROR;
    }
    // TODO: Add NPC to party with specified morale
    return VM_OK;
}

// 0x37: LOAD PIECES <primaryBlockId> <middleBlockId> <secondaryBlockId>
// Loads walldef geometry and 8x8 tile graphics into dynamic cache slots 1-3.
// Mirrors the walldef branch of INSTR_LoadAreaDeco from the original.
// Slots 0 and 4 are fixed (loaded at game init) and never touched here.
static int handle_0x37_LOAD_PIECES(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3 || !syscalls)
        return VM_ERROR;

    // Three params match abStack_8[1..3] in INSTR_LoadAreaDeco
    uint8 primaryBlockId   = (uint8)resolveVar(mem, insn.operands[0]); // slot 1
    uint8 middleBlockId    = (uint8)resolveVar(mem, insn.operands[1]); // slot 2
    uint8 secondaryBlockId = (uint8)resolveVar(mem, insn.operands[2]); // slot 3

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
static int handle_0x39_WHO(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 1) {
        return VM_ERROR;
    }
    // TODO: Show message and let player select party member
    // Then execute LOAD CHARACTER on selected member
    return VM_OK;
}

// 0x3B: SPELL <spellID> <address1> <address2>
static int handle_0x3B_SPELL(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3) {
        return VM_ERROR;
    }
    uint8 spellId = insn.operands[0].u8;
    uint16 addr1 = insn.operands[1].u16;
    uint16 addr2 = insn.operands[2].u16;
    
    // TODO: Search party for character with spell
    // For now, assume not found
    mem.write16LE(addr1, 255);
    return VM_OK;
}

// 0x3E: NPC REMOVE
// Removes the currently loaded NPC from the party roster.
static int handle_0x3E_NPC_REMOVE(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
  //  if (syscalls)
   //     syscalls->removeNpc();
    return VM_OK;
}

// 0x3F: HAS EFFECT <effectID>  /  LOGBOOK ENTRY <string> <index>
// PoR variant (1 arg): tests whether a party-wide effect is active.
// Sets compare EQ if effect present, NE if not.
static int handle_0x3F_HAS_EFFECT(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    // TODO: check active effects list
    setCmpResult(mem, 1); // not present (NE)
    return VM_OK;
}

// 0x40: DESTROY ITEM <itemID>
// Removes a specific item from the party inventory.
static int handle_0x40_DESTROY_ITEM(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    // TODO: remove item from party inventory
    return VM_OK;
}

// 0x41: GIVE EXP <amount> <divideFlag>
// Awards experience to all party members.
// arg0 = base XP amount, arg1 = how to split (0 = each, 1 = divide by party size).
static int handle_0x41_GIVE_EXP(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    // TODO: distribute experience points
    return VM_OK;
}

// 0x42: STOP MOVE (variant B, 0 args)
// Identical to 0x23 STOP_MOVE: halts VM, updates position, clears display.
static int handle_0x42_STOP_MOVE(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (syscalls) {
//        syscalls->updatePosition();
//        syscalls->clearDisplay();
    }
    return VM_HALTED;
}

// 0x43: SOUND EVENT <soundID>
static int handle_0x43_SOUND(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    // TODO: trigger sound effect
    return VM_OK;
}

// 0x44: (unknown 0 args)
static int handle_0x44_UNKNOWN(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    return VM_OK;
}

// 0x45: RANDOM0 <destAddr> <maxVal>
// Writes random(0..maxVal) to destAddr; writes 0 if maxVal == 0.
// Differs from 0x08 RANDOM in that dest is arg0 and max is arg1 (reversed).
static int handle_0x45_RANDOM0(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 2)
        return VM_ERROR;
    uint16 destAddr = insn.operands[0].u16;
    uint16 maxVal = resolveVar(mem, insn.operands[1]);
    uint16 result = (maxVal > 0) ? (uint16)g_random.getRandomNumber(maxVal) : 0;
    mem.write16LE(destAddr, result);
    return VM_OK;
}

// 0x46: FOR START <initVal> <maxVal>
// Starts a counted loop. Loop body begins at the instruction immediately following.
// Original: stores loop counter in a dedicated var; loop runs while counter <= maxVal.
static int handle_0x46_FOR_START(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 2)
        return VM_ERROR;
    g_forLoopCount = (uint16)resolveVar(mem, insn.operands[0]);
    g_forLoopMax   = (uint16)resolveVar(mem, insn.operands[1]);
    g_forLoopBodyStart = nextPc; // decoded instruction index of loop body's first instruction
    return VM_OK;
}

// 0x47: FOR REPEAT
// Increments counter; jumps back to loop body if counter <= maxVal.
static int handle_0x47_FOR_REPEAT(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    g_forLoopCount++;
    if (g_forLoopCount <= g_forLoopMax)
        nextPc = g_forLoopBodyStart;
    return VM_OK;
}

// 0x48: (unknown, 1 arg)
static int handle_0x48_UNKNOWN(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    return VM_OK;
}

// 0x49: (unknown, 6 args)
static int handle_0x49_UNKNOWN(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    return VM_OK;
}

// 0x4A: (unknown, 0 args)
static int handle_0x4A_UNKNOWN(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    return VM_OK;
}

// 0x4B: (unknown, 1 arg)
static int handle_0x4B_UNKNOWN(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    return VM_OK;
}

// 0x4C: PICTURE 2 <pictureID> <variant>
// Extended picture display with variant parameter.
static int handle_0x4C_PICTURE2(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 2 || !syscalls)
        return VM_ERROR;
    uint8 picId = insn.operands[0].u8;
    uint8 variant = (uint8)resolveVar(mem, insn.operands[1]);
    mem.write8(getOpcodeLayout().vmGlobalField(kVmGlobalFieldPictureHeadId).vmAddr, picId);
    return syscalls->displayPicture(picId);
}

void clearOpcodeHandlers() {
    g_handlers.clear();
}

void registerOpcodeHandler(uint8 opcode, OpcodeHandler handler) {
    g_handlers[opcode] = handler;
}

void registerDefaultOpcodeHandlers() {
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
