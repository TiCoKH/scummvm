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
#include "goldbox/poolrad/data/poolrad_character.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {

    void PoolradCharacter::load(Common::SeekableReadStream &stream) {
        stream.skip(1); // 0x000: name length

        // 0x001–0x00F: name
        char nameBuf[16] = {};
        stream.read(nameBuf, 15);
        name = nameBuf;

        stream.seek(0x10, SEEK_SET);
        abilities.strength.current     = stream.readByte(); // 0x010
        abilities.intelligence.current = stream.readByte(); // 0x011
        abilities.wisdom.current       = stream.readByte(); // 0x012
        abilities.dexterity.current    = stream.readByte(); // 0x013
        abilities.constitution.current = stream.readByte(); // 0x014
        abilities.charisma.current     = stream.readByte(); // 0x015
        abilities.strException.current = stream.readByte(); // 0x016

        // 0x017–0x02B: memorized spells
        stream.read(spells.memorizedSpells, 21);

        stream.readByte(); // 0x02C: unknown

        combat.thac0Base = stream.readByte(); // 0x02D

        race       = stream.readByte(); // 0x02E
        classType  = stream.readByte(); // 0x02F
        age        = stream.readUint16LE(); // 0x030–0x031

        hitPoints.max = stream.readByte(); // 0x032

        // 0x033–0x06A: cleric/mage spell knowledge — 62 bytes
        stream.skip(0x06A - 0x033 + 1);

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

        stream.skip(4); // 0x07F–0x082: effects address (pointer)

        stream.readByte(); // 0x083: unknown
        npc = stream.readByte();      // 0x084
        modified = stream.readByte(); // 0x085
        stream.skip(2);               // 0x086–0x087: unknown

        // 0x088–0x095: coin and item valuables
        valuableItems.coinsCopper   = stream.readUint16LE(); // 0x088
        valuableItems.coinsSilver   = stream.readUint16LE(); // 0x08A
        valuableItems.coinsElectrum = stream.readUint16LE(); // 0x08C
        valuableItems.coinsGold     = stream.readUint16LE(); // 0x08E
        valuableItems.coinsPlatinum = stream.readUint16LE(); // 0x090
        valuableItems.gems          = stream.readUint16LE(); // 0x092
        valuableItems.jewelry       = stream.readUint16LE(); // 0x094

        // 0x096–0x09D: class levels
        levels.cleric  = stream.readByte(); // 0x096
        levels.druid   = stream.readByte(); // 0x097
        levels.fighter = stream.readByte(); // 0x098
        levels.paladin = stream.readByte(); // 0x099
        levels.ranger  = stream.readByte(); // 0x09A
        levels.mage    = stream.readByte(); // 0x09B
        levels.thief   = stream.readByte(); // 0x09C
        levels.monk    = stream.readByte(); // 0x09D

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
        equipment.equippedWeapon    = stream.readUint32LE();
        equipment.equippedShield    = stream.readUint32LE();
        equipment.equippedArmor     = stream.readUint32LE();
        equipment.equippedGauntlets = stream.readUint32LE();
        equipment.equippedHelm      = stream.readUint32LE();
        equipment.equippedBelt      = stream.readUint32LE();
        equipment.equippedRobe      = stream.readUint32LE();
        equipment.equippedCloak     = stream.readUint32LE();
        equipment.equippedBoots     = stream.readUint32LE();
        equipment.equippedRing1     = stream.readUint32LE();
        equipment.equippedRing2     = stream.readUint32LE();
        equipment.equippedArrow     = stream.readUint32LE();
        equipment.equippedBolt      = stream.readUint32LE();

        handsUsed   = stream.readByte();       // 0x100
        saveBonus   = stream.readByte();       // 0x101
        encumbrance = stream.readUint16LE();   // 0x102–0x103

        stream.readUint32LE(); // 0x104–0x107: next character address (pointer)
        stream.readUint32LE(); // 0x108–0x10B: combat address (pointer)

        healthStatus = stream.readByte();     // 0x10C
        inCombat     = (stream.readByte() != 0); // 0x10D
        stream.readByte();                    // 0x10E: hostile flag
        quickFightFlag = stream.readByte();   // 0x10F

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
		sideInCombat = 0;
		quickFightFlag = 0;
		priAttacksLeft = 0;
		secAttacksLeft = 0;
		curPriDiceNum = 0;
		curSecDiceNum = 0;
		curPriDiceSides = 0;
		curSecDiceSides = 0;
		curPriBonus = 0;
		curSecBonus = 0;
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
        if (levels.fighter > 0) {
            hp += levels.fighter * 10;
            totalLevels += levels.fighter;
        }
        if (levels.cleric > 0) {
            hp += levels.cleric * 8;
            totalLevels += levels.cleric;
        }
        if (levels.mage > 0) {
            hp += levels.mage * 4;
            totalLevels += levels.mage;
        }
        if (levels.thief > 0) {
            hp += levels.thief * 6;
            totalLevels += levels.thief;
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
        // --- Write the character name
        stream.writeByte(name.size());
        stream.write(name.c_str(), 15); // Pad/truncate to 15 bytes if needed

        // --- Seek to ability scores section

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
         //TODO write spell knowledge

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
        stream.writeUint16LE(valuableItems.coinsCopper);
        stream.writeUint16LE(valuableItems.coinsSilver);
        stream.writeUint16LE(valuableItems.coinsElectrum);
        stream.writeUint16LE(valuableItems.coinsGold);
        stream.writeUint16LE(valuableItems.coinsPlatinum);
        stream.writeUint16LE(valuableItems.gems);
        stream.writeUint16LE(valuableItems.jewelry);

        // Class levels
        stream.writeByte(levels.cleric);
        stream.writeByte(levels.druid);
        stream.writeByte(levels.fighter);
        stream.writeByte(levels.paladin);
        stream.writeByte(levels.ranger);
        stream.writeByte(levels.mage);
        stream.writeByte(levels.thief);
        stream.writeByte(levels.monk);

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
        stream.writeByte(inCombat ? 1 : 0);
        stream.writeByte(0); // hostile flag
        stream.writeByte(quickFightFlag);

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

} // namespace Data
} // namespace Poolrad
} // namespace Goldbox
