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

#include "goldbox/goldbox.h"
#include "goldbox/detection.h"
#include "goldbox/console.h"
#include "common/scummsys.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/events.h"
#include "common/system.h"
#include "engines/util.h"
#include "graphics/paletteman.h"

namespace Goldbox {

GoldboxEngine *g_engine;

GoldboxEngine::GoldboxEngine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst),
	_gameDescription(gameDesc), _randomSource("Goldbox") {
	g_engine = this;
}

GoldboxEngine::~GoldboxEngine() {
}

uint32 GoldboxEngine::getFeatures() const {
	return _gameDescription->flags;
}

Common::String GoldboxEngine::getGameId() const {
	return _gameDescription->gameId;
}

Common::Error GoldboxEngine::run() {

    switch (getPlatform()) {
        case Common::kPlatformDOS:
        case Common::kPlatformAmiga:
            initGraphics(320, 200);
            break;
        case Common::kPlatformPC98:
            initGraphics(640, 480);
            break;
        default:
            initGraphics(320, 200);
            break;
    }

    GoldBox::g_daxCache.loadDax("exampleFile", 0);

    // Set the engine's debugger console
    setDebugger(new Console());

    runGame();

    return Common::kNoError;
}

Common::Error GoldboxEngine::syncGame(Common::Serializer &s) {
	// The Serializer has methods isLoading() and isSaving()
	// if you need to specific steps; for example setting
	// an array size after reading it's length, whereas
	// for saving it would write the existing array's length
	int dummy = 0;
	s.syncAsUint32LE(dummy);

	return Common::kNoError;
}

} // End of namespace Goldbox
