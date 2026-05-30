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

#include "goldbox/ecl/opcode_table.h"
#include "goldbox/ecl/game_config.h"

namespace Goldbox {
namespace ECL {

static const OpcodeInfo *g_opcodeRegistry[256] = { nullptr };

// Forward declarations for operand type arrays
static OperandType noOps[] = { OperandType::NONE };
static OperandType addrOps[] = { OperandType::ADDR16, OperandType::NONE };
static OperandType val8Ops[] = { OperandType::VAL8, OperandType::NONE };
static OperandType val16Ops[] = { OperandType::VAL16, OperandType::NONE };
static OperandType addr2Ops[] = { OperandType::ADDR16, OperandType::ADDR16, OperandType::NONE };
static OperandType val8AddrOps[] = { OperandType::VAL8, OperandType::ADDR16, OperandType::NONE };
static OperandType val16AddrOps[] = { OperandType::VAL16, OperandType::ADDR16, OperandType::NONE };
static OperandType val16Val8Ops[] = { OperandType::VAL16, OperandType::VAL8, OperandType::NONE };
static OperandType addr16Val8Ops[] = { OperandType::ADDR16, OperandType::VAL8, OperandType::NONE };
static OperandType val8Val8Ops[] = { OperandType::VAL8, OperandType::VAL8, OperandType::NONE };
static OperandType sixVal8Ops[] = {
    OperandType::VAL8, OperandType::VAL8, OperandType::VAL8,
    OperandType::VAL8, OperandType::VAL8, OperandType::VAL8,
    OperandType::NONE
};
static OperandType val8Val8AddrOps[] = { OperandType::VAL8, OperandType::VAL8, OperandType::ADDR16, OperandType::NONE };
static OperandType varArgsOps[] = { OperandType::VARARGS, OperandType::NONE };

// Shared baseline ECL opcode table.
// Pool of Radiance uses 0x00-0x3D; later games may extend beyond that.
static OpcodeInfo opcodeTable[] = {
    { 0x00, "EXIT", noOps, "Stops execution and returns control to the player" },
    { 0x01, "GOTO", addrOps, "Continue execution at address" },
    { 0x02, "GOSUB", addrOps, "Call subroutine at address (pushes return address to stack)" },
    { 0x03, "COMPARE", val8Val8Ops, "Compare var1 and var2 for subsequent IF" },
    { 0x04, "ADD", val8Val8AddrOps, "Result var1 + var2 placed in address" },
    { 0x05, "SUBTRACT", val8Val8AddrOps, "Result var2 - var1 placed in address" },
    { 0x06, "DIVIDE", val8Val8AddrOps, "Result var1 / var2 placed in address" },
    { 0x07, "MULTIPLY", val8Val8AddrOps, "Result var1 * var2 placed in address" },
    { 0x08, "RANDOM", val8AddrOps, "Random 0 to var placed in address" },
    { 0x09, "SAVE", val8AddrOps, "Store var in address" },
    { 0x0A, "LOAD CHARACTER", val8Ops, "Load character statistics" },
    { 0x0B, "LOAD MONSTER", val8Val8AddrOps, "Add monsters to encounter list" },
    { 0x0C, "SPRITE_START3", val8Val8AddrOps, "Load sprite and draw encounter stage" },
    { 0x0D, "SPRITE_ADVANCE", noOps, "Decrement distance and redraw sprite" },
    { 0x0E, "PICTURE", val8Ops, "Display picture or end graphical display" },
    { 0x0F, "INPUT NUMBER", val8AddrOps, "Ask player to input number" },
    { 0x10, "INPUT STRING", val8AddrOps, "Ask player to input string" },
    { 0x11, "PRINT", val8Ops, "Print string to text box" },
    { 0x12, "PRINTCLEAR", val8Ops, "Clear text box and print string" },
    { 0x13, "RETURN", noOps, "Return from GOSUB" },
    { 0x14, "COMPARE AND", varArgsOps, "Compare var1 to var2 and var3 to var4" },
    { 0x15, "VERTICAL MENU", varArgsOps, "Display vertical menu with options" },
    { 0x16, "IF =", noOps, "Skip next if not equal" },
    { 0x17, "IF <>", noOps, "Skip next if equal" },
    { 0x18, "IF <", noOps, "Skip next if not less than" },
    { 0x19, "IF >", noOps, "Skip next if not greater than" },
    { 0x1A, "IF <=", noOps, "Skip next if not less or equal" },
    { 0x1B, "IF >=", noOps, "Skip next if not greater or equal" },
    { 0x1C, "CLEARMONSTERS", noOps, "Remove all monsters from encounter list" },
    { 0x1D, "PARTYSTRENGTH", addrOps, "Calculate party strength" },
    { 0x1E, "CHECKPARTY", varArgsOps, "Check party for attribute or effect" },
    { 0x1F, "UNDEFINED", noOps, "Undefined opcode" },
    { 0x20, "NEWECL", val8Ops, "Load new ECL script" },
    { 0x21, "LOAD_AREA_GEO", varArgsOps, "Load map and resources" },
    { 0x22, "PARTY SURPRISE", addr2Ops, "Initialize surprise rolls" },
    { 0x23, "SURPRISE", varArgsOps, "Roll for combat surprise" },
    { 0x24, "COMBAT", noOps, "Start combat" },
    { 0x25, "ON GOTO", varArgsOps, "Conditional GOTO based on value" },
    { 0x26, "ON GOSUB", varArgsOps, "Conditional GOSUB based on value" },
    { 0x27, "TREASURE", varArgsOps, "Add treasure for next combat" },
    { 0x28, "ROB", val8Val8AddrOps, "Rob party of money and items" },
    { 0x29, "ENCOUNTER MENU", varArgsOps, "Encounter with menu options" },
    { 0x2A, "COPY MEM", varArgsOps, "Copy values inernal memory" },
    { 0x2B, "HORIZONTAL MENU", varArgsOps, "Display horizontal menu" },
    { 0x2C, "PARLAY", varArgsOps, "Parley attitude selection" },
    { 0x2D, "CALL", addrOps, "Call machine code at address" },
    { 0x2E, "DAMAGE", varArgsOps, "Apply damage to characters" },
    { 0x2F, "AND", val8Val8AddrOps, "Bitwise AND, result in address" },
    { 0x30, "OR", val8Val8AddrOps, "Bitwise OR, result in address" },
    { 0x31, "SPRITE OFF", noOps, "Turn off sprite display" },
    { 0x32, "FIND ITEM", val8Ops, "Find item in party inventory" },
    { 0x33, "PRINT RETURN", noOps, "Print blank line" },
    { 0x34, "ECL CLOCK", val8Val8AddrOps, "Clock command (unused in Poolrad)" },
    { 0x35, "SAVE TABLE", varArgsOps, "Save value to array" },
    { 0x36, "ADD NPC", val8Val8AddrOps, "Add NPC to party" },
    { 0x37, "LOAD_AREA_WALLDEF", varArgsOps, "Load wall definitions" },
    { 0x38, "PROGRAM", val8Ops, "Run special routine" },
    { 0x39, "WHO", val8Ops, "Select party member" },
    { 0x3A, "DELAY", noOps, "Delay execution" },
    { 0x3B, "SPELL", varArgsOps, "Search for spell in party" },
    { 0x3C, "PROTECTION", val8Ops, "Print runic phrase" },
    { 0x3D, "CLEAR BOX", noOps, "Clear text box" },
    { 0x3E, "NPC REMOVE", noOps, "Remove NPC from party" },
    { 0x3F, "HAS EFFECT", val8Ops, "Check if character has effect" },
    { 0x40, "DESTROY ITEM", val8Ops, "Destroy item from inventory" },
    { 0x41, "GIVE EXP", val8Val8Ops, "Give experience points to party" },
    { 0x42, "STOP MOVE", noOps, "Halt movement" },
    { 0x43, "SOUND", val8Ops, "Play sound event" },
    { 0x44, "UNDEFINED", noOps, "Undefined opcode" },
    { 0x45, "RANDOM0", addr16Val8Ops, "Random 0 to var placed in address" },
    { 0x46, "FOR START", val8Val8Ops, "Start for loop" },
    { 0x47, "FOR REPEAT", noOps, "Repeat for loop body" },
    { 0x48, "UNDEFINED", val8Ops, "Undefined opcode (1 arg)" },
    { 0x49, "UNDEFINED", sixVal8Ops, "Undefined opcode (6 args)" },
    { 0x4A, "UNDEFINED", noOps, "Undefined opcode" },
    { 0x4B, "UNDEFINED", val8Ops, "Undefined opcode (1 arg)" },
    { 0x4C, "PICTURE 2", val8Val8Ops, "Display secondary picture" }
};

void clearOpcodeTable() {
    for (uint i = 0; i < ARRAYSIZE(g_opcodeRegistry); ++i) {
        g_opcodeRegistry[i] = nullptr;
    }
}

void registerOpcodeInfo(uint8 opcode, const OpcodeInfo *info) {
    g_opcodeRegistry[opcode] = info;
}

void registerBaselineOpcodeTable(const GameConfig &config) {
    clearOpcodeTable();
    for (uint8 opcode = 0; opcode <= config.getMaxOpcodeTableByte(); ++opcode) {
        registerOpcodeInfo(opcode, &opcodeTable[opcode]);
    }
}

const OpcodeInfo *getOpcodeInfo(uint8 opcode) {
    return g_opcodeRegistry[opcode];
}

const char *getOpcodeName(uint8 opcode) {
    const OpcodeInfo *info = getOpcodeInfo(opcode);
    return info ? info->name : "UNKNOWN";
}

int getOperandCount(uint8 opcode) {
    const OpcodeInfo *info = getOpcodeInfo(opcode);
    if (!info || !info->operands) {
        return 0;
    }
    int count = 0;
    for (int i = 0; info->operands[i] != OperandType::NONE; ++i) {
        count++;
    }
    // VARARGS returns 0 (variable operands determined at decode time)
    if (info->operands[0] == OperandType::VARARGS) {
        return 0;
    }
    return count;
}

} // namespace ECL
} // namespace Goldbox
