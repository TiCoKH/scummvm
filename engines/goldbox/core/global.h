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

#ifndef GOLDBOX_CORE_GLOBAL_H
#define GOLDBOX_CORE_GLOBAL_H



namespace Goldbox {

enum GameState  {
    GS_START_MENU     = 0,
    GS_SHOP           = 1,
    GS_CAMPING        = 2,
    GS_WILDERNESS_MAP = 3,
    GS_DUNGEON_MAP    = 4,
    GS_COMBAT         = 5,
    GS_AFTER_COMBAT   = 6,
    GS_END_GAME       = 7
};

} // namespace Goldbox

#endif // GOLDBOX_CORE_GLOBAL_H
