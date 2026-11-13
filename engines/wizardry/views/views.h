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

#ifndef WIZARDRY_VIEWS_H
#define WIZARDRY_VIEWS_H

#include "wizardry/views/castle.h"
#include "wizardry/views/gallery.h"
#include "wizardry/views/maze/maze_view.h"
#include "wizardry/views/title.h"
#include "wizardry/views/town/castle.h"
#include "wizardry/views/town/edge.h"
#include "wizardry/views/town/equip.h"
#include "wizardry/views/town/inn.h"
#include "wizardry/views/town/sheet.h"
#include "wizardry/views/town/shop.h"
#include "wizardry/views/town/spell_books.h"
#include "wizardry/views/town/temple.h"
#include "wizardry/views/town/training.h"
#include "wizardry/views/town/tavern.h"

namespace Wizardry {

struct Views {
	Castle _castle;
	Title _title;
	Gallery _gallery;
	Town::Castle _townCastle;
	Town::Tavern _townTavern;
	Town::InnView _townInn;
	Town::TempleView _townTemple;
	Town::ShopView _townShop;
	Town::Edge _townEdge;
	Town::TrainingView _townTraining;
	Town::SheetView _townSheet;
	Town::SpellBooks _townSpellBooks;
	Town::EquipView _townEquip;
	InMaze::MazeView _maze;
};

} // namespace Wizardry

#endif
