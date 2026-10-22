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

#ifndef TEST_ENGINES_GOLDBOX_POOLRAD_INVENTORY_H
#define TEST_ENGINES_GOLDBOX_POOLRAD_INVENTORY_H

#include <cxxtest/TestSuite.h>

#include "common/memstream.h"
#include "goldbox/poolrad/data/poolrad_character.h"

/**
 * Exercises the legacy receive path without loading game progression tables.
 */
class PoolradInventoryTestSuite : public CxxTest::TestSuite {
    class Character : public Goldbox::Poolrad::Data::PoolradCharacter {
    public:
        Character() {
            race = 0;
            classType = Goldbox::Data::C_MAGICUSER;
            abilities.strength.current = 10;
            abilities.strException.current = 0;
            abilities.dexterity.current = 10;
            movement.base = 12;
            armorClass.base = 50;
        }

        void recalcCombatStats() override {
            Goldbox::Data::ADnDCharacter::recalcCombatStats();
        }
    };

    Goldbox::Data::Items::Storage _savedItemProperties;

    static Goldbox::Data::Items::CharacterItem makeItem(uint16 weight,
            uint8 stackSize = 0) {
        Goldbox::Data::Items::CharacterItem item = {};
        item.typeIndex = 1;
        item.weight = weight;
        item.stackSize = stackSize;
        return item;
    }

public:
    void setUp() {
        _savedItemProperties = Goldbox::Engine::gItemProps;
        byte properties[2 + 128 * 16] = {};
        properties[2 + 13] = 0x08;
        properties[2 + 16 + 13] = 0xFF;
        Common::MemoryReadStream stream(properties, sizeof(properties));
        Goldbox::Engine::gItemProps.load(stream);
    }

    void tearDown() {
        Goldbox::Engine::gItemProps = _savedItemProperties;
    }

    void testCapacityModifiers() {
        struct CapacityCase {
            uint8 strength;
            uint8 exceptional;
            int modifier;
        };
        const CapacityCase cases[] = {
            {0, 0, 0}, {3, 0, -350}, {5, 0, -250}, {7, 0, -150},
            {8, 0, 0}, {11, 0, 0}, {12, 0, 100}, {13, 0, 100},
            {14, 0, 200}, {15, 0, 200}, {16, 0, 350}, {17, 0, 500},
            {18, 0, 750}, {18, 50, 1000}, {18, 75, 1250},
            {18, 90, 1500}, {18, 99, 2000}, {18, 100, 3000},
            {19, 0, 4000}, {20, 0, 5000}, {21, 0, 6000},
            {22, 0, 7500}, {23, 0, 9000}, {24, 0, 12000},
            {25, 0, 15000}
        };
        Character character;
        for (uint i = 0; i < ARRAYSIZE(cases); ++i) {
            character.abilities.strength.current = cases[i].strength;
            character.abilities.strException.current = cases[i].exceptional;
            TS_ASSERT_EQUALS(character.getCapacityModifier(),
                cases[i].modifier);
        }
    }

    void testReceiveDoesNotApplyReadyClassRestrictions() {
        Character character;
        Goldbox::Data::Items::CharacterItem item = makeItem(10);
        item.typeIndex = 0;
        item.nextAddress = 0x12345678;

        TS_ASSERT(character.receiveItem(item));
        TS_ASSERT(character.hasItems());
        TS_ASSERT_EQUALS(character.getInventory().count(), 1);
        TS_ASSERT_EQUALS(character.numOfItems, 1);
        TS_ASSERT_EQUALS(character.encumbrance, 10);
        TS_ASSERT_EQUALS(character.getInventory()[0].nextAddress, 0U);
        TS_ASSERT_EQUALS(item.nextAddress, 0x12345678U);
    }

    void testReceiveAllowsSixteenthItemButNotSeventeenth() {
        Character character;
        const Goldbox::Data::Items::CharacterItem item = makeItem(0);
        for (int i = 0; i < 15; ++i)
            character.inventory.appendCopy(item);

        TS_ASSERT(character.receiveItem(item));
        TS_ASSERT_EQUALS(character.numOfItems, 16);
        TS_ASSERT(!character.receiveItem(item));
        TS_ASSERT_EQUALS(character.getInventory().count(), 16);
    }

    void testReceiveAllowsExactCapacityButNotOneMore() {
        Character character;
        TS_ASSERT(character.receiveItem(makeItem(1500)));
        TS_ASSERT_EQUALS(character.encumbrance, 1500);
        TS_ASSERT(!character.receiveItem(makeItem(1)));
        TS_ASSERT_EQUALS(character.getInventory().count(), 1);
    }

    void testReceiveIncludesCoinsInEncumbrance() {
        Character character;
        character.valuableItems[Goldbox::Data::VAL_COPPER] = 1400;

        TS_ASSERT(character.receiveItem(makeItem(100)));
        TS_ASSERT_EQUALS(character.encumbrance, 1500);
        TS_ASSERT(!character.receiveItem(makeItem(1)));
        TS_ASSERT_EQUALS(character.getInventory().count(), 1);
    }

    void testReceiveMultipliesStackWeight() {
        Character character;
        TS_ASSERT(!character.receiveItem(makeItem(501, 3)));
        TS_ASSERT_EQUALS(character.getInventory().count(), 0);
        TS_ASSERT(character.receiveItem(makeItem(500, 3)));
        TS_ASSERT_EQUALS(character.encumbrance, 1500);
    }

    void testReceiveUsesBagAdjustedEncumbrance() {
        Character character;
        Goldbox::Data::Items::CharacterItem bag = makeItem(100);
        bag.nameCode1 = 186;
        bag.readied = 1;
        character.inventory.appendCopy(bag);
        character.inventory.appendCopy(makeItem(1800));

        TS_ASSERT(character.receiveItem(makeItem(100)));
        TS_ASSERT_EQUALS(character.getInventory().count(), 3);
        TS_ASSERT_EQUALS(character.encumbrance, 100);
    }

    void testReceivedItemsRemainVisibleThroughInventoryAccessor() {
        Character character;
        const Goldbox::Data::Items::CharacterInventory &viewInventory =
            character.getInventory();

        TS_ASSERT_EQUALS(viewInventory.count(), 0);
        TS_ASSERT(character.receiveItem(makeItem(10)));
        TS_ASSERT_EQUALS(viewInventory.count(), 1);
        TS_ASSERT(character.receiveItem(makeItem(20)));
        TS_ASSERT_EQUALS(viewInventory.count(), 2);
        TS_ASSERT_EQUALS(viewInventory[0].weight, 10);
        TS_ASSERT_EQUALS(viewInventory[1].weight, 20);
    }
};

#endif // TEST_ENGINES_GOLDBOX_POOLRAD_INVENTORY_H
