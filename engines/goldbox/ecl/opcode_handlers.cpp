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

namespace Goldbox {
namespace ECL {

// Use a typedef alias for easier reference
using HandlerMap = Common::HashMap<uint8, OpcodeHandler>;
static HandlerMap g_handlers;
static Common::RandomSource g_random("eclvm");

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

static int handle_0x03_COMPARE(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    // Stub for now; full implementation would update comparison flags
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

static int handle_0x2F_AND(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3) {
        return VM_ERROR;
    }
    uint16 var1 = resolveVar(mem, insn.operands[0]);
    uint16 var2 = resolveVar(mem, insn.operands[1]);
    uint16 addr = insn.operands[2].u16;
    mem.write16LE(addr, var1 & var2);
    return VM_OK;
}

static int handle_0x30_OR(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 3) {
        return VM_ERROR;
    }
    uint16 var1 = resolveVar(mem, insn.operands[0]);
    uint16 var2 = resolveVar(mem, insn.operands[1]);
    uint16 addr = insn.operands[2].u16;
    mem.write16LE(addr, var1 | var2);
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
    uint8 partySize = mem.read8(ECL::ECLMemoryLayout::OFFSET_PARTY_SIZE);

    // Store selection index for engine/UI reference
    mem.write8(ECL::ECLMemoryLayout::OFFSET_SELECTED_CHAR, sel);

    if (sel < 128) {
        if (sel < partySize) {
            // Point selected character pointer to this member's data block
            uint16 base = ECL::ECLMemoryLayout::OFFSET_CHARACTER_DATA + sel * ECL::ECLMemoryLayout::CHARACTER_DATA_SIZE;
            mem.write16LE(ECL::ECLMemoryLayout::OFFSET_SEL_CHAR_PTR, base);
        } else {
            // Select a monster from the list (index after party members)
            uint16 monsterIndex = sel - partySize;
            // Without a defined monster record size, point to start for now
            uint16 base = ECL::ECLMemoryLayout::OFFSET_MONSTER_DATA; // TODO: add record size and index into list
            (void)monsterIndex;
            mem.write16LE(ECL::ECLMemoryLayout::OFFSET_SEL_CHAR_PTR, base);
        }
    } else {
        // Monster (sel - 128) will be placed on party side in next combat
        // Record selection; engine should interpret this when starting combat
        uint16 base = ECL::ECLMemoryLayout::OFFSET_MONSTER_DATA; // Placeholder pointer
        mem.write16LE(ECL::ECLMemoryLayout::OFFSET_SEL_CHAR_PTR, base);
        mem.write8(ECL::ECLMemoryLayout::OFFSET_MENU_COMBAT_STATE, 1); // Mark pending special placement
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
    uint16 currentCount = mem.read16LE(ECL::ECLMemoryLayout::OFFSET_MONSTER_COUNT);
    uint16 newCount = (currentCount + (count & 0xFF)) & 0xFFFF;
    mem.write16LE(ECL::ECLMemoryLayout::OFFSET_MONSTER_COUNT, newCount);

    // Stash last loaded monster info in encounter flags area for engine consumption
    mem.write16LE(ECL::ECLMemoryLayout::OFFSET_ENCOUNTER_FLAGS, (monsterId & 0xFF));
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
    mem.write8(ECL::ECLMemoryLayout::OFFSET_MONSTER_DISTANCE, dist);

    // Mark game state as combat and store primary monster ID
    mem.write8(ECL::ECLMemoryLayout::OFFSET_GAME_STATE, (uint8)GS_COMBAT);
    mem.write16LE(ECL::ECLMemoryLayout::OFFSET_ENCOUNTER_FLAGS, (monsterId & 0xFF));
    (void)graphicId;

    VmResult r = syscalls->startCombat();
    return r;
}

// 0x0D: APPROACH
// Monsters from SETUP MONSTER close distance by 1 square.
static int handle_0x0D_APPROACH(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    uint8 dist = mem.read8(ECL::ECLMemoryLayout::OFFSET_MONSTER_DISTANCE);
    if (dist > 0) {
        dist -= 1;
        mem.write8(ECL::ECLMemoryLayout::OFFSET_MONSTER_DISTANCE, dist);
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
    mem.write8(ECL::ECLMemoryLayout::OFFSET_PICTURE_ID, picId);

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
static int handle_0x14_COMPARE_AND(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 4) {
        return VM_ERROR;
    }
    uint16 var1 = resolveVar(mem, insn.operands[0]);
    uint16 var2 = resolveVar(mem, insn.operands[1]);
    uint16 var3 = resolveVar(mem, insn.operands[2]);
    uint16 var4 = resolveVar(mem, insn.operands[3]);
    
    // Store comparison flags for subsequent IF commands
    bool cmp1 = (var1 == var2);
    bool cmp2 = (var3 == var4);
    mem.write8(ECL::ECLMemoryLayout::OFFSET_BREAK_FLAG, (cmp1 && cmp2) ? 1 : 0);
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

// 0x16-0x1B: IF commands (skip next instruction if comparison fails)
static int handleIF(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, uint8 opcode) {
    uint8 comparisonFlag = mem.read8(ECL::ECLMemoryLayout::OFFSET_BREAK_FLAG);
    bool shouldSkip = false;
    
    switch (opcode) {
    case 0x16: // IF =
        shouldSkip = (comparisonFlag == 0);
        break;
    case 0x17: // IF <>
        shouldSkip = (comparisonFlag != 0);
        break;
    case 0x18: // IF <
    case 0x19: // IF >
    case 0x1A: // IF <=
    case 0x1B: // IF >=
        // For these, we need the actual comparison values stored
        shouldSkip = (comparisonFlag == 0);
        break;
    }
    
    if (shouldSkip) {
        nextPc += 1; // Skip next instruction
    }
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
    mem.write16LE(ECL::ECLMemoryLayout::OFFSET_MONSTER_COUNT, 0);
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

static int handle_0x21_LOAD_FILES(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    // Load GEO map and related map resources (0x21: LOAD FILES)
    // TODO: Parse operands from VARARGS to determine which map/resources to load
    // Expected: Load GEO file, initialize map graphics, set up encounter zones
    // For now, return VM_OK to continue execution
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
    mem.write8(ECL::ECLMemoryLayout::OFFSET_SURPRISE_FLAGS, 0);
    return VM_OK;
}

// 0x24: COMBAT
static int handle_0x24_COMBAT(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (!syscalls) {
        return VM_ERROR;
    }
    
    uint8 templeFlag = mem.read8(ECL::ECLMemoryLayout::OFFSET_TEMPLE_FLAG);
    uint8 shopFlag = mem.read8(ECL::ECLMemoryLayout::OFFSET_SHOP_FLAG);
    
    if (templeFlag == 1) {
        mem.write8(ECL::ECLMemoryLayout::OFFSET_TEMPLE_FLAG, 0);
        // TODO: Enter temple
        return VM_OK;
    } else if (shopFlag == 1) {
        mem.write8(ECL::ECLMemoryLayout::OFFSET_SHOP_FLAG, 0);
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
    
    // Store comparison flag for IF commands
    mem.write8(ECL::ECLMemoryLayout::OFFSET_BREAK_FLAG, (value == 0) ? 1 : 0);
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
    mem.write8(ECL::ECLMemoryLayout::OFFSET_SPRITE_STATE, 0);
    return VM_OK;
}

// 0x32: FIND ITEM <itemID>
static int handle_0x32_FIND_ITEM(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    if (insn.operands.size() < 1) {
        return VM_ERROR;
    }
    uint8 itemId = insn.operands[0].u8;
    // TODO: Search party inventory for item
    // For now, set comparison flag to 0 (not found)
    mem.write8(ECL::ECLMemoryLayout::OFFSET_BREAK_FLAG, 0);
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

static int handle_0x37_LOAD_PIECES(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls) {
    // Load wall definitions and piece graphics (0x37: LOAD PIECES)
    // TODO: Parse operands from VARARGS to determine which wallset/pieces to load
    // Expected: Load wallset definitions, sprite data, enable piece rendering
    // For now, return VM_OK to continue execution
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

void registerOpcodeHandlers() {
    g_handlers[0x00] = handle_0x00_EXIT;
    g_handlers[0x01] = handle_0x01_GOTO;
    g_handlers[0x02] = handle_0x02_GOSUB;
    g_handlers[0x03] = handle_0x03_COMPARE;
    g_handlers[0x04] = handle_0x04_ADD;
    g_handlers[0x05] = handle_0x05_SUBTRACT;
    g_handlers[0x06] = handle_0x06_DIVIDE;
    g_handlers[0x07] = handle_0x07_MULTIPLY;
    g_handlers[0x08] = handle_0x08_RANDOM;
    g_handlers[0x09] = handle_0x09_SAVE;
    g_handlers[0x0A] = handle_0x0A_LOAD_CHARACTER;
    g_handlers[0x0B] = handle_0x0B_LOAD_MONSTER;
    g_handlers[0x0C] = handle_0x0C_SETUP_MONSTER;
    g_handlers[0x0D] = handle_0x0D_APPROACH;
    g_handlers[0x0E] = handle_0x0E_PICTURE;
    g_handlers[0x0F] = handle_0x0F_INPUT_NUMBER;
    g_handlers[0x10] = handle_0x10_INPUT_STRING;
    g_handlers[0x11] = handle_0x11_PRINT;
    g_handlers[0x12] = handle_0x12_PRINTCLEAR;
    g_handlers[0x13] = handle_0x13_RETURN;
    g_handlers[0x14] = handle_0x14_COMPARE_AND;
    g_handlers[0x15] = handle_0x15_VERTICAL_MENU;
    g_handlers[0x16] = handle_0x16_IF_EQUAL;
    g_handlers[0x17] = handle_0x17_IF_NOT_EQUAL;
    g_handlers[0x18] = handle_0x18_IF_LESS;
    g_handlers[0x19] = handle_0x19_IF_GREATER;
    g_handlers[0x1A] = handle_0x1A_IF_LESS_EQUAL;
    g_handlers[0x1B] = handle_0x1B_IF_GREATER_EQUAL;
    g_handlers[0x1C] = handle_0x1C_CLEARMONSTERS;
    g_handlers[0x1D] = handle_0x1D_PARTYSTRENGTH;
    g_handlers[0x1E] = handle_0x1E_CHECKPARTY;
    g_handlers[0x20] = handle_0x20_NEWECL;
    g_handlers[0x21] = handle_0x21_LOAD_FILES;
    g_handlers[0x22] = handle_0x22_PARTY_SURPRISE;
    g_handlers[0x23] = handle_0x23_SURPRISE;
    g_handlers[0x24] = handle_0x24_COMBAT;
    g_handlers[0x25] = handle_0x25_ON_GOTO;
    g_handlers[0x26] = handle_0x26_ON_GOSUB;
    g_handlers[0x27] = handle_0x27_TREASURE;
    g_handlers[0x28] = handle_0x28_ROB;
    g_handlers[0x29] = handle_0x29_ENCOUNTER_MENU;
    g_handlers[0x2A] = handle_0x2A_GETTABLE;
    g_handlers[0x2B] = handle_0x2B_HORIZONTAL_MENU;
    g_handlers[0x2C] = handle_0x2C_PARLAY;
    g_handlers[0x2D] = handle_0x2D_CALL;
    g_handlers[0x2E] = handle_0x2E_DAMAGE;
    g_handlers[0x2F] = handle_0x2F_AND;
    g_handlers[0x30] = handle_0x30_OR;
    g_handlers[0x31] = handle_0x31_SPRITE_OFF;
    g_handlers[0x32] = handle_0x32_FIND_ITEM;
    g_handlers[0x33] = handle_0x33_PRINT_RETURN;
    g_handlers[0x34] = handle_0x34_ECL_CLOCK;
    g_handlers[0x35] = handle_0x35_SAVE_TABLE;
    g_handlers[0x36] = handle_0x36_ADD_NPC;
    g_handlers[0x37] = handle_0x37_LOAD_PIECES;
    g_handlers[0x38] = handle_0x38_PROGRAM;
    g_handlers[0x39] = handle_0x39_WHO;
    g_handlers[0x3A] = handle_0x3A_DELAY;
    g_handlers[0x3B] = handle_0x3B_SPELL;
    g_handlers[0x3C] = handle_0x3C_PROTECTION;
    g_handlers[0x3D] = handle_0x3D_CLEAR_BOX;
}

OpcodeHandler getOpcodeHandler(uint8 opcode) {
    if (g_handlers.contains(opcode)) {
        return g_handlers[opcode];
    }
    return nullptr;
}

} // namespace ECL
} // namespace Goldbox
