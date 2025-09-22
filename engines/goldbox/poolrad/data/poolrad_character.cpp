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

#include "common/stream.h"
#include "goldbox/vm_interface.h"
#include "goldbox/data/pascal_string_buffer.h"
#include "goldbox/data/spells/spellbook.h"
#include "goldbox/poolrad/data/poolrad_character.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {

    // PoolradSpellBook implementation
    void PoolradSpellBook::fromPoolradMem(const uint8 *memorized, const uint8 *known) {
        clear();
        for (uint8 i = 0; i < 21; ++i)
            setSpell(i, known[i] != 0, memorized[i]);
        for (uint8 i = 21; i < 62; ++i)
            setSpell(i, known[i] != 0, 0);
    }

    void PoolradSpellBook::toPoolradMem(uint8 *memorized, uint8 *known) const {
        for (uint8 i = 0; i < 21; ++i) {
            const Goldbox::Data::Spells::SpellEntry *e = getSpell(i);
            memorized[i] = e ? e->memorized : 0;
            known[i] = e ? (e->known ? 1 : 0) : 0;
        }
        for (uint8 i = 21; i < 62; ++i) {
            const Goldbox::Data::Spells::SpellEntry *e = getSpell(i);
            known[i] = e ? (e->known ? 1 : 0) : 0;
        }
    }

    void PoolradCharacter::load(Common::SeekableReadStream &stream) {

        name = Goldbox::Data::PascalStringBuffer<15>::read(stream); // 0x000–0x00F
        abilities.strength.current     = stream.readByte(); // 0x010
        abilities.intelligence.current = stream.readByte(); // 0x011
        abilities.wisdom.current       = stream.readByte(); // 0x012
        abilities.dexterity.current    = stream.readByte(); // 0x013
        abilities.constitution.current = stream.readByte(); // 0x014
        abilities.charisma.current     = stream.readByte(); // 0x015
        abilities.strException.current = stream.readByte(); // 0x016

        // 0x017–0x02B: memorized spells
        stream.read(spells.memorizedSpells, 21);
		// 0x02C: unknown
		stream.seek(0x2D, SEEK_SET);
        combat.thac0Base = stream.readByte(); // 0x02D

        race       = stream.readByte(); // 0x02E
        classType  = stream.readByte(); // 0x02F
        age        = stream.readUint16LE(); // 0x030–0x031

        hitPoints.max = stream.readByte(); // 0x032

        // 0x033–0x06A: cleric/mage spell knowledge — 55 bytes
        stream.read(spells.knownSpells, 55);

        attackLevel = stream.readByte();   // 0x06B
        iconDimension = stream.readByte(); // 0x06C

        savingThrows.vsParalysis     = stream.readByte(); // 0x06D
        savingThrows.vsPetrification = stream.readByte(); // 0x06E
        savingThrows.vsRodStaffWand  = stream.readByte(); // 0x06F
        savingThrows.vsBreathWeapon  = stream.readByte(); // 0x070
        savingThrows.vsSpell         = stream.readByte(); // 0x071

        baseMovement = stream.readByte();      // 0x072
        highestLevel = stream.readByte();      // 0x073
        drainedLevels = stream.readByte();     // 0x074
        drainedHp = stream.readByte();         // 0x075
        undeadResistance = stream.readByte();  // 0x076

        thiefSkills.pickPockets      = stream.readByte(); // 0x077
        thiefSkills.openLocks        = stream.readByte(); // 0x078
        thiefSkills.findRemoveTraps  = stream.readByte(); // 0x079
        thiefSkills.moveSilently     = stream.readByte(); // 0x07A
        thiefSkills.hideInShadows    = stream.readByte(); // 0x07B
        thiefSkills.hearNoise        = stream.readByte(); // 0x07C
        thiefSkills.climbWalls       = stream.readByte(); // 0x07D
        thiefSkills.readLanguages    = stream.readByte(); // 0x07E

		// 0x07F–0x082: effects address (pointer)
		stream.seek(0x84, SEEK_SET);
        npc = stream.readSByte();      // 0x084
        modified = stream.readByte(); // 0x085
		// 0x086–0x087: unknown
		stream.seek(0x88, SEEK_SET);
        // 0x088–0x095: coin and item valuables
        for (int i = 0; i < Goldbox::Data::VALUABLE_COUNT; ++i)
            valuableItems.values[i] = stream.readUint16LE();

        // 0x096–0x09D: class levels
        for (int i = 0; i < 8; ++i)
            levels.levels[i] = stream.readByte();

        // 0x09E–0x0A0: gender, type, alignment
        gender    = (Goldbox::Data::Gender)stream.readByte(); // 0x09E
        monsterType = stream.readByte();                     // 0x09F
        alignment = stream.readByte();                       // 0x0A0

        // 0x0A1–0x0A2: primary & secondary attacks ×2
        primaryAttacks   = stream.readByte();
        secondaryAttacks = stream.readByte();

        // 0x0A3–0x0A8: unarmed combat data
        priDmgDiceNum     = stream.readByte();
        secDmgDiceNum     = stream.readByte();
        priDmgDiceSides   = stream.readByte();
        secDmgDiceSides   = stream.readByte();
        priDmgModifier    = stream.readByte();
        secDmgModifier    = stream.readByte();

        armorClass.base = stream.readByte();          // 0x0A9
        strengthBonusAllowed = stream.readByte();     // 0x0AA
        combatIcon = stream.readByte();               // 0x0AB

        experiencePoints = stream.readUint32LE();     // 0x0AC–0x0AF

        itemsLimit = stream.readByte(); // 0x0B0:
        hitPointsRolled = stream.readByte(); // 0x0B1

        // 0x0B2–0x0B7: spell slots
        spellSlots[0] = stream.readByte(); // cleric 1
        spellSlots[1] = stream.readByte(); // cleric 2
        spellSlots[2] = stream.readByte(); // cleric 3
        spellSlots[3] = stream.readByte(); // mage 1
        spellSlots[4] = stream.readByte(); // mage 2
        spellSlots[5] = stream.readByte(); // mage 3

        xpForDefeating = stream.readUint16LE(); // 0x0B8–0x0B9
        bonusXpPerHp = stream.readByte();       // 0x0BA

        portrait.head = stream.readByte(); // 0x0BB
        portrait.body = stream.readByte(); // 0x0BC

        iconData.iconHead = stream.readByte(); // 0x0BD
        iconData.iconBody = stream.readByte(); // 0x0BE
        orderNumber = stream.readByte();       // 0x0BF
        iconData.iconSize = stream.readByte(); // 0x0C0

        iconData.setBody(stream.readByte());     // 0x0C1
        iconData.setArm(stream.readByte());      // 0x0C2
        iconData.setLeg(stream.readByte());      // 0x0C3
        iconData.setHairFace(stream.readByte()); // 0x0C4
        iconData.setShield(stream.readByte());   // 0x0C5
        iconData.setWeapon(stream.readByte());   // 0x0C6

        numOfItems = stream.readByte();          // 0x0C7
        itemsAddress = stream.readUint32LE();    // 0x0C8–0x0CB — pointer

        // 0x0CC–0x0FF: equipped items (all pointers)
        for (int i = 0; i < EQUIPMENT_SLOT_COUNT; ++i)
            equippedOffsets[i] = stream.readUint32LE();

        handsUsed   = stream.readByte();       // 0x100
        saveBonus   = stream.readByte();       // 0x101
        encumbrance = stream.readUint16LE();   // 0x102–0x103

        stream.readUint32LE(); // 0x104–0x107: next character address (pointer)
        stream.readUint32LE(); // 0x108–0x10B: combat address (pointer)

        healthStatus = stream.readByte();        // 0x10C
        enabled      = (stream.readByte() != 0); // 0x10D
        hostile      = (stream.readByte() != 0); // 0x10E
        quickfight   = (stream.readByte() != 0); // 0x10F

        thac0.current        = stream.readByte(); // 0x110
        armorClass.current   = stream.readByte(); // 0x111
        acRear.current       = stream.readByte(); // 0x112

        priAttacksLeft = stream.readByte(); // 0x113
        secAttacksLeft = stream.readByte(); // 0x114

        curPriDiceNum   = stream.readByte(); // 0x115
        curSecDiceNum   = stream.readByte(); // 0x116
        curPriDiceSides = stream.readByte(); // 0x117
        curSecDiceSides = stream.readByte(); // 0x118
        curPriBonus     = stream.readByte(); // 0x119
        curSecBonus     = stream.readByte(); // 0x11A

        hitPoints.current = stream.readByte(); // 0x11B
        movement.current  = stream.readByte(); // 0x11C
        spellBook.fromPoolradMem(spells.memorizedSpells, spells.knownSpells);
    }

    PoolradCharacter::PoolradCharacter() {
        // Initialize default values
        initialize();
    }

    void PoolradCharacter::initialize() {


		highestLevel = 0;
		creatureSize = 1;
		baseMovement = 0;
		hitDice = 0;
		drainedLevels = 0;
		drainedHp = 0;
		undeadResistance = 0;
		monsterType = 0;
		primaryAttacks = 0;
		secondaryAttacks = 0;
		priDmgDiceNum = 0;
		secDmgDiceNum = 0;
		priDmgDiceSides = 0;
		secDmgDiceSides = 0;
		priDmgModifier = 0;
		secDmgModifier = 0;
		strengthBonusAllowed = 0;
		combatIcon = 0;
		hitPointsRolled = 0;

		xpForDefeating = 0;
		bonusXpPerHp = 0;
		itemsLimit = 0;
		handsUsed = 0;
		encumbrance = 0;
		actions = 0;
		enabled = true;
        hostile = true;
        quickfight = true;
		priAttacksLeft = 0;
		secAttacksLeft = 0;
		curPriDiceNum = 0;
		curSecDiceNum = 0;
		curPriDiceSides = 0;
		curSecDiceSides = 0;
		curPriBonus = 0;
		curSecBonus = 0;
    }

    // Returns true if the character has any memorized spell
    bool PoolradCharacter::haveMemorizedSpell() const {
        for (const auto &entry : spellBook.spells) {
            if (entry._value.memorized > 0)
                return true;
        }
        return false;
    }

    void PoolradCharacter::resolveEquippedItems() {
        for (int slot = 0; slot < EQUIPMENT_SLOT_COUNT; ++slot) {
            equipment.slots[slot] = -1;
            for (size_t i = 0; i < inventory.items().size(); ++i) {
                if (inventory.items()[i].nextAddress == equippedOffsets[slot]) {
                    equipment.slots[slot] = static_cast<int>(i);
                    break;
                }
            }
        }
    }

    void PoolradCharacter::rollAbilityScores() {
        abilities.strength.base = abilities.strength.current = VmInterface::rollDice(3, 6);
        abilities.intelligence.base = abilities.intelligence.current = VmInterface::rollDice(3, 6);
        abilities.wisdom.base       = abilities.wisdom.current       = VmInterface::rollDice(3, 6);
        abilities.dexterity.base    = abilities.dexterity.current    = VmInterface::rollDice(3, 6);
        abilities.constitution.base = abilities.constitution.current = VmInterface::rollDice(3, 6);
        abilities.charisma.base     = abilities.charisma.current     = VmInterface::rollDice(3, 6);
    }

    void PoolradCharacter::applyRacialAdjustments() {
        using namespace Goldbox::Data;

        switch (race) {
        case R_HUMAN:
            break; // no change
        case R_DWARF:
            adjustAbilityForRace(abilities.constitution, +1, 3, 18);
            adjustAbilityForRace(abilities.charisma, -1, 3, 18);
            break;
        case R_ELF:
            adjustAbilityForRace(abilities.dexterity, +1, 3, 18);
            adjustAbilityForRace(abilities.constitution, -1, 3, 18);
            break;
        case R_GNOME:
            adjustAbilityForRace(abilities.intelligence, +1, 3, 18);
            adjustAbilityForRace(abilities.wisdom, -1, 3, 18);
            break;
        case R_HALFLING:
            adjustAbilityForRace(abilities.dexterity, +1, 3, 18);
            adjustAbilityForRace(abilities.strength, -1, 3, 18);
            break;
        case R_HALF_ELF:
        case R_HALF_ORC:
        case R_MONSTER:
        default:
            break;
        }
    }

    void PoolradCharacter::adjustAbilityForRace(Goldbox::Data::Stat &ability, int adjustment, int minValue, int maxValue) {
        int newAbility = ability.current + adjustment;
        newAbility = CLIP(newAbility, minValue, maxValue);
        ability.current = static_cast<uint8>(newAbility);
    }

    uint8 PoolradCharacter::getBaseIconColor(int index) {
        if (index < 0 || index >= 7)
            return 0;
        uint8 base = BASE_ICON_VALUES[index];
        return base + static_cast<uint8>((base + 8) * 0x10);
    }

    bool PoolradCharacter::meetsClassRequirements() const {
        using namespace Goldbox::Data;

        switch (classType) {
        case C_FIGHTER:
            return abilities.strength.current >= 9;
        case C_RANGER:
            return abilities.strength.current >= 13 &&
                   abilities.dexterity.current >= 13 &&
                   abilities.wisdom.current >= 14;
        case C_PALADIN:
            return abilities.strength.current >= 12 &&
                   abilities.charisma.current >= 17;
        // Add more cases
        default:
            return true;
        }
    }
    void PoolradCharacter::calculateHitPoints() {
        int totalLevels = 0;
        int hp = 0;

        // Each class contributes hit dice based on its level
        if (levels.levels[2] > 0) {
            hp += levels.levels[2] * 10;
            totalLevels += levels.levels[2];
        }
        if (levels.levels[0] > 0) {
            hp += levels.levels[0] * 8;
            totalLevels += levels.levels[0];
        }
        if (levels.levels[5] > 0) {
            hp += levels.levels[5] * 4;
            totalLevels += levels.levels[5];
        }
        if (levels.levels[6] > 0) {
            hp += levels.levels[6] * 6;
            totalLevels += levels.levels[6];
        }

        int average = (totalLevels > 0) ? (hp / totalLevels) : 4;

        // Constitution modifier
        int conMod = 0;
        if (abilities.constitution.current >= 15) conMod = 1;
        if (abilities.constitution.current >= 17) conMod = 2;

        hitPointsRolled = average + conMod;
        hitPoints.max = hitPoints.current = (hitPointsRolled > 0) ? hitPointsRolled : 1;
    }

    void PoolradCharacter::save(Common::WriteStream &stream) {

        spellBook.toPoolradMem(spells.memorizedSpells, spells.knownSpells);
        Goldbox::Data::PascalStringBuffer<15>::write(stream, name);
        // Ability scores
        stream.writeByte(abilities.strength.current);
        stream.writeByte(abilities.intelligence.current);
        stream.writeByte(abilities.wisdom.current);
        stream.writeByte(abilities.dexterity.current);
        stream.writeByte(abilities.constitution.current);
        stream.writeByte(abilities.charisma.current);
        stream.writeByte(abilities.strException.current);

        // Spells memorized
        stream.write(spells.memorizedSpells, 21);

        stream.writeByte(0); // Unknown at 0x02C

        // Combat
        stream.writeByte(combat.thac0Base);

        stream.writeByte(race);
        stream.writeByte(classType);
        stream.writeUint16LE(age);

        stream.writeByte(hitPoints.max);

        stream.write(spells.knownSpells, 62);

        stream.writeByte(attackLevel);
        stream.writeByte(iconDimension);

        // Saving throws
        stream.writeByte(savingThrows.vsParalysis);
        stream.writeByte(savingThrows.vsPetrification);
        stream.writeByte(savingThrows.vsRodStaffWand);
        stream.writeByte(savingThrows.vsBreathWeapon);
        stream.writeByte(savingThrows.vsSpell);

        stream.writeByte(baseMovement);
        stream.writeByte(highestLevel);
        stream.writeByte(drainedLevels);
        stream.writeByte(drainedHp);
        stream.writeByte(undeadResistance);

        // Thief skills
        stream.writeByte(thiefSkills.pickPockets);
        stream.writeByte(thiefSkills.openLocks);
        stream.writeByte(thiefSkills.findRemoveTraps);
        stream.writeByte(thiefSkills.moveSilently);
        stream.writeByte(thiefSkills.hideInShadows);
        stream.writeByte(thiefSkills.hearNoise);
        stream.writeByte(thiefSkills.climbWalls);
        stream.writeByte(thiefSkills.readLanguages);

        for (int i = 0; i < 4; ++i) stream.writeByte(0); // Skip effects address

        stream.writeByte(0); // Unknown at 0x083
        stream.writeByte(npc);
        stream.writeByte(modified);

        for (int i = 0; i < 2; ++i) stream.writeByte(0);

        // Coins and valuables
        for (int i = 0; i < Goldbox::Data::VALUABLE_COUNT; ++i)
            stream.writeUint16LE(valuableItems.values[i]);

        // Class levels
        for (int i = 0; i < 8; ++i)
            stream.writeByte(levels.levels[i]);

        // Gender, Monster type, Alignment
        stream.writeByte(gender);
        stream.writeByte(monsterType);
        stream.writeByte(alignment);

        // Attacks
        stream.writeByte(primaryAttacks);
        stream.writeByte(secondaryAttacks);

        // Unarmed combat
        stream.writeByte(priDmgDiceNum);
        stream.writeByte(secDmgDiceNum);
        stream.writeByte(priDmgDiceSides);
        stream.writeByte(secDmgDiceSides);
        stream.writeByte(priDmgModifier);
        stream.writeByte(secDmgModifier);

        stream.writeByte(armorClass.base);
        stream.writeByte(strengthBonusAllowed);
        stream.writeByte(combatIcon);

        stream.writeUint32LE(experiencePoints);

        stream.writeByte(itemsLimit);
        stream.writeByte(hitPointsRolled);

        // Spell slots
        for (int i = 0; i < 6; ++i)
            stream.writeByte(spellSlots[i]);

        stream.writeUint16LE(xpForDefeating);
        stream.writeByte(bonusXpPerHp);

        stream.writeByte(portrait.head);
        stream.writeByte(portrait.body);

        stream.writeByte(iconData.iconHead);
        stream.writeByte(iconData.iconBody);
        stream.writeByte(orderNumber);
        stream.writeByte(iconData.iconSize);

        // Icon color data
        stream.writeByte((iconData.iconColorBody1 << 4) | iconData.iconColorBody2);
        stream.writeByte((iconData.iconColorArm1 << 4) | iconData.iconColorArm2);
        stream.writeByte((iconData.iconColorLeg1 << 4) | iconData.iconColorLeg2);
        stream.writeByte((iconData.iconColorHair << 4) | iconData.iconColorFace);
        stream.writeByte((iconData.iconColorShield1 << 4) | iconData.iconColorShield2);
        stream.writeByte((iconData.iconColorWeapon1 << 4) | iconData.iconColorWeapon2);

        stream.writeByte(numOfItems);
        stream.writeUint32LE(itemsAddress);

        // Equipped items pointers (ignored, just write 0 for now)
        for (int i = 0; i < 13; ++i)
            stream.writeUint32LE(0);

        stream.writeByte(handsUsed);
        stream.writeByte(saveBonus);
        stream.writeUint16LE(encumbrance);

        stream.writeUint32LE(0); // next character address
        stream.writeUint32LE(0); // combat address

        stream.writeByte(healthStatus);
        stream.writeByte(enabled ? 1 : 0);
        stream.writeByte(hostile ? 1 : 0);
        stream.writeByte(quickfight ? 1 : 0);

        stream.writeByte(thac0.current);
        stream.writeByte(armorClass.current);
        stream.writeByte(acRear.current);

        stream.writeByte(priAttacksLeft);
        stream.writeByte(secAttacksLeft);

        stream.writeByte(curPriDiceNum);
        stream.writeByte(curSecDiceNum);
        stream.writeByte(curPriDiceSides);
        stream.writeByte(curSecDiceSides);
        stream.writeByte(curPriBonus);
        stream.writeByte(curSecBonus);

        stream.writeByte(hitPoints.current);
        stream.writeByte(movement.current);
    }


    void PoolradCharacter::finalizeName() {
        //std::replace(name.begin(), name.end(), ' ', static_cast<char>(-1));
    }

    byte PoolradCharacter::getNameColor() {
        if (name.size() == 0) return 0;
        int txtColor = 15;
        if (!enabled) {
            txtColor = 12;
        } else if (hostile) {
             txtColor = 14;
        } else {
             txtColor = 11;
        }
        return txtColor;
    }


} // namespace Data
} // namespace Poolrad
} // namespace Goldbox
