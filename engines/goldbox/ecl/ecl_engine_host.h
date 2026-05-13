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

#ifndef GOLDBOX_ECL_ECL_ENGINE_HOST_H
#define GOLDBOX_ECL_ECL_ENGINE_HOST_H

#include "common/scummsys.h"
#include "common/array.h"
#include "common/str.h"
#include "goldbox/ecl/ecl_types.h"
#include "goldbox/ecl/syscall_handler.h"

namespace Goldbox {

namespace ECL {

/**
 * Host interface provided by the Goldbox engine to the ECL VM.
 * The VM calls out through this interface for I/O, menus, combat,
 * and other engine-side async operations.
 *
 * Pure virtual methods MUST be implemented by every game host.
 * Non-pure virtual methods have a default no-op implementation and
 * may be overridden by game-specific hosts as needed.
 */
class EclEngineHost : public SyscallHandler {
public:
    virtual ~EclEngineHost() = default;

    // -----------------------------------------------------------------------
    // Text / I/O
    // -----------------------------------------------------------------------

    /**
     * Print text to the message/text box.
     * @param text     Text to print
     * @param clearBox If true, clear text box before printing
     */
    virtual void printText(const Common::String &text, bool clearBox = false) = 0;

    /**
     * Clear the text/message box.
     */
    virtual void clearTextBox() = 0;

    /**
     * Input a number from the player. Blocks until the player confirms.
     * @param maxDigits Maximum digits to accept
     * @return Value entered by the player (non-negative), or -1 on cancel
     */
    virtual int16 inputNumber(uint8 maxDigits) = 0;

    /**
     * Input a string from the player. Blocks until the player confirms.
     * @param maxLength Maximum characters to accept
     * @return String entered by the player (may be empty on cancel)
     */
    virtual Common::String inputString(uint8 maxLength) = 0;

    // -----------------------------------------------------------------------
    // Menus
    // -----------------------------------------------------------------------

    /**
     * Display a vertical (list) menu and wait for selection. Blocks until the
     * player selects an entry.
     * @param message Prompt message shown above the list
     * @param options Option strings (one per entry)
     * @return 0-based selection index, or -1 on cancel
     */
    virtual int16 verticalMenu(const Common::String &message,
            const Common::Array<Common::String> &options) = 0;

    /**
     * Display a horizontal (hotkey) menu and wait for selection. Blocks until
     * the player selects an entry.
     * @param options Option strings (one per key)
     * @return 0-based selection index, or -1 on cancel
     */
    virtual int16 horizontalMenu(const Common::Array<Common::String> &options) = 0;

    /**
     * Display the parlay attitude menu (0x2C PARLAY).
     * Presents five attitudes: haughty, sly, nice, meek, abusive.
     * Blocks until the player selects an attitude.
     * @param attitudes Five attitude text strings in original order
     * @return Selected attitude index (0-4), or -1 on cancel
     */
    virtual int16 parlayMenu(const Common::Array<Common::String> &attitudes) { return 0; }

    /**
     * Display the encounter menu (0x29 ENCOUNTER MENU).
     * Full encounter loop: fight / parlay / advance / flee options.
     * Blocks until encounter is resolved. Host writes all result values to
     * memory directly from the operand addresses.
     * @param operands Raw operand data from the decoded instruction
     * @return VmResult
     */
    virtual VmResult encounterMenu(const Common::Array<uint8> &operands) { return VM_OK; }

    /**
     * Ask the player to select a party member (0x39 WHO). Blocks until
     * the player makes a selection.
     * @param message Prompt message to display
     * @return 0-based party member index, or -1 on cancel
     */
    virtual int16 selectPartyMember(const Common::String &message) { return 0; }

    // -----------------------------------------------------------------------
    // Graphics / Pictures
    // -----------------------------------------------------------------------

    /**
     * Display a picture or sprite (0x0E PICTURE).
     * @param picID Picture/portrait ID; 0xFF = clear display and redraw 3D view
     * @return VmResult
     */
    virtual VmResult displayPicture(uint8 picID) = 0;

    /**
     * Display a picture with an extended variant parameter (0x4C PICTURE2).
     * @param picID   Picture ID
     * @param variant Variant selector
     * @return VmResult
     */
    virtual VmResult displayPicture2(uint8 picID, uint8 variant) { return VM_OK; }

    // -----------------------------------------------------------------------
    // Map / Asset loading
    // -----------------------------------------------------------------------

    /**
     * Load a new ECL script block (0x20 NEWECL).
     * @param scriptID Script ID to load
     * @return VmResult
     */
    virtual VmResult loadScript(uint8 scriptID) = 0;

    /**
        * Load a GEO dungeon map block by block ID (0x21 LOAD_AREA_GEO, indoor branch).
     * @param blockId DAX GEO block ID
     * @return VmResult
     */
    virtual VmResult loadGeoBlock(uint8 blockId) { return VM_OK; }

    /**
     * Load a wallset definition and its 8x8 tile graphics into a cache slot
        * (0x37 LOAD_AREA_WALLDEF).
     * Slots 1-3 are dynamic (set per area); slots 0 and 4 are fixed at init.
     * blockId 0xFF = clear/invalidate the slot.
     * @param blockId DAX WALLDEF block ID (0xFF = clear)
     * @param setSlot Cache slot to populate (1, 2, or 3)
     * @return VmResult
     */
    virtual VmResult loadWallSet(uint8 blockId, uint8 setSlot) { return VM_OK; }

    /**
        * Load the outdoor/city icon strip (0x21 LOAD_AREA_GEO, outdoor branch).
     * @return VmResult
     */
    virtual VmResult loadIconBlock() { return VM_OK; }

    // -----------------------------------------------------------------------
    // Monsters / Encounter
    // -----------------------------------------------------------------------

    /**
     * Add monsters to the encounter list (0x0B LOAD MONSTER).
     * @param monsterId Monster template ID
     * @param count     Number of monsters to add
     * @param graphicId Sprite/icon ID for these monsters
     * @return VmResult
     */
    virtual VmResult loadMonster(uint8 monsterId, uint8 count,
            uint8 graphicId) { return VM_OK; }

    /**
     * Set up a monster encounter at a given distance (0x0C SETUP MONSTER).
     * Loads sprite sheet and draws the encounter stage.
     * @param monsterId Monster template ID
     * @param distance  Starting distance (0=adjacent, 1=near, 2=far)
     * @param graphicId Sprite/icon ID
     * @return VM_YIELD to show encounter stage, VM_OK when done
     */
    virtual VmResult setupMonsterEncounter(uint8 monsterId, uint8 distance,
            uint8 graphicId) { return VM_OK; }

    /**
     * Clear all monsters from the encounter list (0x1C CLEARMONSTERS).
     * @return VmResult
     */
    virtual VmResult clearMonsters() { return VM_OK; }

    /**
     * Add an NPC to the party (0x36 ADD NPC).
     * @param monsterId Monster template ID for the NPC
     * @param morale    Initial morale value
     * @return VmResult
     */
    virtual VmResult addNpc(uint8 monsterId, uint8 morale) { return VM_OK; }

    /**
     * Remove the currently selected NPC from the party (0x3E NPC REMOVE).
     * @return VmResult
     */
    virtual VmResult removeNpc() { return VM_OK; }

    // -----------------------------------------------------------------------
    // Combat
    // -----------------------------------------------------------------------

    /**
     * Start the main combat loop with currently loaded monsters (0x24 COMBAT).
     * @return VM_YIELD while combat is in progress
     */
    virtual VmResult startCombat() = 0;

    /**
     * Enter a temple (0x24 COMBAT with temple flag set).
     * @return VmResult
     */
    virtual VmResult enterTemple() { return VM_OK; }

    /**
     * Enter a shop (0x24 COMBAT with shop flag set).
     * @return VmResult
     */
    virtual VmResult enterShop() { return VM_OK; }

    /**
     * Initialize surprise rolls for an encounter (0x22 PARTY SURPRISE).
     * Checks party for thief/druid presence and writes roll results.
     * @param monsterRollAddr Memory address to store monster surprise roll
     * @param partyRollAddr   Memory address to store party surprise roll
     * @return VmResult
     */
    virtual VmResult rollPartySurprise(uint16 monsterRollAddr,
            uint16 partyRollAddr) { return VM_OK; }

    /**
     * Roll a full surprise check (0x23 SURPRISE).
     * Writes result to kVmGlobalFieldCombatIsAmbush.
     * @param addr1 Result address 1
     * @param addr2 Result address 2
     * @param mod1  Modifier for party roll
     * @param mod2  Modifier for monster roll
     * @return VmResult
     */
    virtual VmResult rollSurprise(uint16 addr1, uint16 addr2,
            uint16 mod1, uint16 mod2) { return VM_OK; }

    /**
     * Apply damage to a target (0x2E DAMAGE).
     * @param targetSel  Target selector (party member index or monster)
     * @param dice       Number of dice to roll
     * @param sides      Sides per die
     * @param bonus      Flat damage bonus
     * @param resultAddr Memory address to store total damage dealt
     * @return VmResult
     */
    virtual VmResult applyDamage(uint8 targetSel, uint8 dice, uint8 sides,
            uint8 bonus, uint16 resultAddr) { return VM_OK; }

    // -----------------------------------------------------------------------
    // Party / Character queries
    // -----------------------------------------------------------------------

    /**
     * Calculate total party strength (0x1D PARTYSTRENGTH).
     * Sum based on experience, hit points, movement, and skills.
     * @param resultAddr Memory address to store result
     * @return VmResult
     */
    virtual VmResult calculatePartyStrength(uint16 resultAddr) { return VM_OK; }

    /**
     * Check party for an attribute or effect (0x1E CHECKPARTY).
     * @param attributeAddr Address of attribute to check (0 = check effect instead)
     * @param effectId      Effect ID to check (0 = check attribute instead)
     * @param highAddr      Address to store highest value found
     * @param lowAddr       Address to store count or lowest value found
     * @return VmResult
     */
    virtual VmResult checkParty(uint16 attributeAddr, uint16 effectId,
            uint16 highAddr, uint16 lowAddr) { return VM_OK; }

    /**
     * Check whether a party-wide effect is active (0x3F HAS EFFECT).
     * Should set compare EQ if effect is present, NE if not.
     * @param effectId   Effect ID to test
     * @param resultAddr Address to store 0 (present) or 1 (absent)
     * @return VmResult
     */
    virtual VmResult hasEffect(uint8 effectId, uint16 resultAddr) { return VM_OK; }

    /**
     * Find a specific item in the party inventory (0x32 FIND ITEM).
     * Should set compare EQ if found, NE if not.
     * @param itemId     Item ID to search for
     * @param resultAddr Address to store finder's member index (or 255 if not found)
     * @return VmResult
     */
    virtual VmResult findItem(uint8 itemId, uint16 resultAddr) { return VM_OK; }

    /**
     * Remove a specific item from the party inventory (0x40 DESTROY ITEM).
     * @param itemId Item ID to destroy
     * @return VmResult
     */
    virtual VmResult destroyItem(uint8 itemId) { return VM_OK; }

    /**
     * Award experience points to party members (0x41 GIVE EXP).
     * @param amount      Base XP amount
     * @param divideFlag  0 = give to each member, 1 = divide among party
     * @return VmResult
     */
    virtual VmResult giveExperience(uint16 amount, uint8 divideFlag) { return VM_OK; }

    /**
     * Set up treasure for the next combat (0x27 TREASURE).
     * @param copper     Copper coins
     * @param silver     Silver coins
     * @param electrum   Electrum coins
     * @param gold       Gold coins
     * @param platinum   Platinum coins
     * @param gems       Gem count
     * @param jewelry    Jewelry count
     * @param treasureId Magic item treasure table ID
     * @return VmResult
     */
    virtual VmResult setupTreasure(uint8 copper, uint8 silver, uint8 electrum,
            uint8 gold, uint8 platinum, uint8 gems, uint8 jewelry,
            uint8 treasureId) { return VM_OK; }

    /**
     * Rob the party of money and items (0x28 ROB).
     * @param wholeParty  Non-zero = rob entire party, 0 = current character only
     * @param percentMoney Percentage of money to steal
     * @param itemChance  Chance (0-100) to steal an item
     * @return VmResult
     */
    virtual VmResult robParty(uint8 wholeParty, uint8 percentMoney,
            uint8 itemChance) { return VM_OK; }

    /**
     * Search party for a character who has a given spell (0x3B SPELL).
     * @param spellId    Spell ID to search for
     * @param memberAddr Address to store party member index (or 255 if not found)
     * @param slotAddr   Address to store spell slot index
     * @return VmResult
     */
    virtual VmResult findSpell(uint8 spellId, uint16 memberAddr,
            uint16 slotAddr) { return VM_OK; }

    // -----------------------------------------------------------------------
    // Time / Misc
    // -----------------------------------------------------------------------

    /**
     * Advance the game clock by a given amount (0x34 ECL CLOCK).
     * @param amount Number of time units to advance
     * @return VmResult
     */
    virtual VmResult advanceClock(uint8 amount) { return VM_OK; }

    /**
     * Play a sound effect (0x43 SOUND EVENT).
     * @param soundId Sound effect ID
     * @return VmResult
     */
    virtual VmResult playSoundEvent(uint8 soundId) { return VM_OK; }

    /**
     * Stop movement and redraw the screen (0x42 STOP MOVE).
     * Halts the VM after updating the player position and clearing the display.
     * @return VM_HALTED
     */
    virtual VmResult stopMove() { return VM_HALTED; }

    /**
     * Execute a special program routine (0x38 PROGRAM). Blocks until the
     * program completes.
     * @param programID 0=main menu / training, 8=win screen, 9=encamp
     * @return VM_OK on completion, VM_HALTED if the program ends the game
     */
    virtual VmResult executeProgram(uint8 programID) = 0;
};

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_ECL_ENGINE_HOST_H
