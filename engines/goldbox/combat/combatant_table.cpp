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

#include "goldbox/combat/combatant_table.h"
#include "goldbox/data/player_character.h"
#include <string.h>

namespace Goldbox {
namespace Combat {

CombatantTable::CombatantTable()
    : _count(0), _friendsCount(0), _foesCount(0) {
    clear();
}

void CombatantTable::clear() {
    for (int i = 0; i < MAX_COMBATANTS; i++)
        _entries[i] = Entry();
    _count = 0;
    _friendsCount = 0;
    _foesCount = 0;
    memset(_occupancy, 0, sizeof(_occupancy));
    _triggers.clear();
}

int CombatantTable::addCombatant(Data::PlayerCharacter *ch, uint8 size) {
    if (_count >= MAX_COMBATANTS)
        return -1;
    int idx = _count;
    _entries[idx].character = ch;
    _entries[idx].size = size;
    _entries[idx].tileCol = 0;
    _entries[idx].tileRow = 0;
    _count++;
    return idx;
}

void CombatantTable::removeCombatant(int idx) {
    if (idx < 0 || idx >= MAX_COMBATANTS)
        return;
    _entries[idx].size = 0;
    _entries[idx].character = nullptr;
}

Data::PlayerCharacter *CombatantTable::getCharacter(int idx) const {
    if (idx < 0 || idx >= MAX_COMBATANTS)
        return nullptr;
    return _entries[idx].character;
}

uint8 CombatantTable::getTileCol(int idx) const {
    if (idx < 0 || idx >= MAX_COMBATANTS)
        return 0;
    return _entries[idx].tileCol;
}

uint8 CombatantTable::getTileRow(int idx) const {
    if (idx < 0 || idx >= MAX_COMBATANTS)
        return 0;
    return _entries[idx].tileRow;
}

uint8 CombatantTable::getSize(int idx) const {
    if (idx < 0 || idx >= MAX_COMBATANTS)
        return 0;
    return _entries[idx].size;
}

void CombatantTable::setPosition(int idx, uint8 col, uint8 row) {
    if (idx < 0 || idx >= MAX_COMBATANTS)
        return;
    _entries[idx].tileCol = col;
    _entries[idx].tileRow = row;
}

void CombatantTable::setSize(int idx, uint8 size) {
    if (idx < 0 || idx >= MAX_COMBATANTS)
        return;
    _entries[idx].size = size;
}

void CombatantTable::addTrigger(Data::PlayerCharacter *ch, uint8 col, uint8 row, uint8 savedTile) {
    TriggerRecord rec;
    rec.character = ch;
    rec.tileCol = col;
    rec.tileRow = row;
    rec.savedTile = savedTile;
    _triggers.push_back(rec);
}

void CombatantTable::rebuildOccupancy() {
    memset(_occupancy, 0, sizeof(_occupancy));

    for (int i = 0; i < _count; i++) {
        if (_entries[i].size == 0)
            continue;

        uint8 col = _entries[i].tileCol;
        uint8 row = _entries[i].tileRow;
        uint8 size = _entries[i].size & 7;

        // Size footprint: 0/1 = 1x1, 2 = 2x1, 3 = 1x2, 4+ = 2x2
        int w = (size >= 2 && size != 3) ? 2 : 1;
        int h = (size >= 3) ? 2 : 1;

        for (int dy = 0; dy < h; dy++) {
            for (int dx = 0; dx < w; dx++) {
                int c = col + dx;
                int r = row + dy;
                if (c >= 0 && c < 50 && r >= 0 && r < 25)
                    _occupancy[r][c] = (uint8)(i + 1);
            }
        }
    }
}

uint8 CombatantTable::getOccupant(int col, int row) const {
    if (col < 0 || col >= 50 || row < 0 || row >= 25)
        return 0;
    return _occupancy[row][col];
}

void CombatantTable::countSides(int partyCount) {
    _friendsCount = 0;
    _foesCount = 0;
    for (int i = 0; i < _count; i++) {
        if (_entries[i].character == nullptr)
            continue;
        if (i < partyCount)
            _friendsCount++;
        else
            _foesCount++;
    }
}

} // namespace Combat
} // namespace Goldbox
