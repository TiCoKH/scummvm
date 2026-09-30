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

#include "goldbox/game_factory.h"

#include "common/hashmap.h"

namespace Goldbox {

namespace {

typedef Common::HashMap<int, EngineCreateFunc> Registry;

// Function-local static: guarantees the registry exists before the first
// Registrar's static initializer runs, regardless of translation unit order.
Registry &getRegistry() {
	static Registry *registry = new Registry();
	return *registry;
}

} // End of anonymous namespace

void GameFactory::registerGame(int gameType, EngineCreateFunc create) {
	getRegistry()[gameType] = create;
}

Engine *GameFactory::create(int gameType, OSystem *syst, const GoldboxGameDescription *gameDesc) {
	Registry::iterator it = getRegistry().find(gameType);
	if (it == getRegistry().end())
		return nullptr;

	return it->_value(syst, gameDesc);
}

} // End of namespace Goldbox
