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
#include "goldbox/combat/combat_ground_info.h"
#include "goldbox/data/player_character.h"
#include <string.h>

namespace Goldbox {
namespace Combat {

CombatantTable::CombatantTable()
    : _count(0), _friendsCount(0), _foesCount(0),
      _occupancyDirty(true), _vpPosDirty(true),
      _vpOriginCol(0), _vpOriginRow(0) {
    clear();
}

void CombatantTable::clear() {
    for (int i = 0; i < MAX_COMBATANTS; i++)
        _entries[i] = Entry();
    _count = 0;
    _friendsCount = 0;
    _foesCount = 0;
    _occupancyDirty = true;
    _vpPosDirty = true;
    _vpOriginCol = 0;
    _vpOriginRow = 0;
    memset(_occupancy, 0, sizeof(_occupancy));
    memset(_colDist, 0, sizeof(_colDist));
    memset(_rowDist, 0, sizeof(_rowDist));
    _downedMembers.clear();
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
    _occupancyDirty = true;
    _vpPosDirty = true;
    return idx;
}

bool CombatantTable::rollbackLastAdd(int idx) {
    if (_count <= 0)
        return false;
    if (idx != _count - 1)
        return false;

    _entries[idx] = Entry();
    _count--;
    _occupancyDirty = true;
    _vpPosDirty = true;
    return true;
}

void CombatantTable::removeCombatant(int idx) {
    if (idx < 0 || idx >= MAX_COMBATANTS)
        return;
    _entries[idx].size = 0;
    _entries[idx].character = nullptr;
    _occupancyDirty = true;
    _vpPosDirty = true;
}

int CombatantTable::findIndex(const Data::PlayerCharacter *ch) const {
    for (int i = 0; i < _count; i++) {
        if (_entries[i].character == ch)
            return i;
    }
    return -1;
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
    _occupancyDirty = true;
    _vpPosDirty = true;
}

void CombatantTable::setSize(int idx, uint8 size) {
    if (idx < 0 || idx >= MAX_COMBATANTS)
        return;
    _entries[idx].size = size;
    _occupancyDirty = true;
}

uint8 CombatantTable::getCharacterCol(const Data::PlayerCharacter *ch) const {
    int idx = findIndex(ch);
    if (idx < 0)
        return 0;
    return _entries[idx].tileCol;
}

uint8 CombatantTable::getCharacterRow(const Data::PlayerCharacter *ch) const {
    int idx = findIndex(ch);
    if (idx < 0)
        return 0;
    return _entries[idx].tileRow;
}

uint8 CombatantTable::getCharacterSize(const Data::PlayerCharacter *ch) const {
    int idx = findIndex(ch);
    if (idx < 0)
        return 0;
    return _entries[idx].size;
}

void CombatantTable::addDownedMember(Data::PlayerCharacter *ch, uint8 col, uint8 row, uint8 savedTile) {
    DownedMemberRecord rec;
    rec.character = ch;
    rec.tileCol = col;
    rec.tileRow = row;
    rec.savedTile = savedTile;
    _downedMembers.push_back(rec);
}

// --- Occupancy grid (lazy) ---

void CombatantTable::ensureOccupancy() const {
    if (_occupancyDirty)
        doRebuildOccupancy();
}

uint8 CombatantTable::getOccupant(int col, int row) const {
    if (col < 0 || col >= 50 || row < 0 || row >= 25)
        return 0;
    ensureOccupancy();
    return _occupancy[row][col];
}

void CombatantTable::doRebuildOccupancy() const {
    memset(_occupancy, 0, sizeof(_occupancy));

    for (int i = 0; i < _count; i++) {
        if (_entries[i].size == 0)
            continue;

        uint8 baseCol = _entries[i].tileCol;
        uint8 baseRow = _entries[i].tileRow;
        uint8 iconSize = _entries[i].size & 7;

        // Use the canonical getIconOffsetBySize for footprint — same
        // lookup table used by getGroundInfo, ensuring consistency.
        for (uint8 slot = 0; slot < 4; slot++) {
            int8 colDelta, rowDelta;
            if (!getIconOffsetBySize(iconSize, slot, colDelta, rowDelta))
                continue;

            int c = (int)baseCol + colDelta;
            int r = (int)baseRow + rowDelta;
            if (c >= 0 && c < 50 && r >= 0 && r < 25)
                _occupancy[r][c] = (uint8)(i + 1);
        }
    }

    _occupancyDirty = false;
}

// --- Viewport-relative position cache (lazy) ---

void CombatantTable::setViewportOrigin(int vpCol, int vpRow) {
    if (vpCol != _vpOriginCol || vpRow != _vpOriginRow) {
        _vpOriginCol = vpCol;
        _vpOriginRow = vpRow;
        _vpPosDirty = true;
    }
}

int8 CombatantTable::getColDist(int idx) const {
    if (idx < 0 || idx >= MAX_COMBATANTS)
        return 0;
    if (_vpPosDirty)
        doRebuildViewportPositions();
    return _colDist[idx];
}

int8 CombatantTable::getRowDist(int idx) const {
    if (idx < 0 || idx >= MAX_COMBATANTS)
        return 0;
    if (_vpPosDirty)
        doRebuildViewportPositions();
    return _rowDist[idx];
}

int CombatantTable::getManhattanDist(int idx) const {
    if (idx < 0 || idx >= MAX_COMBATANTS)
        return 127;
    if (_vpPosDirty)
        doRebuildViewportPositions();
    int cd = _colDist[idx] < 0 ? -_colDist[idx] : _colDist[idx];
    int rd = _rowDist[idx] < 0 ? -_rowDist[idx] : _rowDist[idx];
    return cd + rd;
}

void CombatantTable::doRebuildViewportPositions() const {
    for (int i = 0; i < _count; i++) {
        if (_entries[i].size == 0) {
            _colDist[i] = 0;
            _rowDist[i] = 0;
            continue;
        }
        _colDist[i] = (int8)((int)_entries[i].tileCol - _vpOriginCol);
        _rowDist[i] = (int8)((int)_entries[i].tileRow - _vpOriginRow);
    }
    _vpPosDirty = false;
}

// --- Side counts ---

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
