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

#ifndef GOLDBOX_GAME_FACTORY_H
#define GOLDBOX_GAME_FACTORY_H

class OSystem;

namespace Goldbox {

class Engine;
struct GoldboxGameDescription;

typedef Engine *(*EngineCreateFunc)(OSystem *syst, const GoldboxGameDescription *gameDesc);

/**
 * Registry mapping a GameType (see detection.h) to the function that
 * constructs the matching Goldbox::Engine subclass.
 *
 * Shared dispatch code (GoldboxMetaEngine::createInstance) only calls
 * GameFactory::create() and never names a concrete engine class, so adding
 * a new game means adding a new self-registering source file rather than
 * editing the shared switch statement.
 */
class GameFactory {
public:
	static void registerGame(int gameType, EngineCreateFunc create);
	static Engine *create(int gameType, OSystem *syst, const GoldboxGameDescription *gameDesc);

	/**
	 * Static-initialization helper: instantiate one of these at namespace
	 * scope in a game's .cpp file to register its engine class without
	 * touching any shared file.
	 */
	template<typename EngineClass>
	struct Registrar {
		explicit Registrar(int gameType) {
			GameFactory::registerGame(gameType, &createInstance);
		}

		static Engine *createInstance(OSystem *syst, const GoldboxGameDescription *gameDesc) {
			return new EngineClass(syst, gameDesc);
		}
	};
};

} // End of namespace Goldbox

#endif // GOLDBOX_GAME_FACTORY_H
