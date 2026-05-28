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

#include "goldbox/engine.h"
#include "goldbox/console.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/events.h"
#include "common/path.h"
#include "engines/util.h"
#include "graphics/palette.h"
#include "goldbox/gfx/dax_tile.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {

Engine *g_engine;
Data::Items::Storage Engine::gItemProps;

Engine::Engine(OSystem *syst, const GoldboxGameDescription *gameDesc) : ::Engine(syst),
	_gameDescription(gameDesc), _randomSource("Goldbox"),
	_daxManager(gameDesc->desc.platform) {
	g_engine = this;
}

Engine::~Engine() {
	delete _fixedTileCacheSlot0;
	delete _font;
	_daxManager.clear();
	for (uint i = 0; i < _party.size(); ++i) {
        delete _party[i];
    }
    _party.clear();
}

void Engine::setup() {
	if (!initializeGameData())
		return;

	if (!loadGameAssets())
		return;

	if (!initializeRuntimeSystems())
		return;
}

void Engine::initFixedTileCacheSlots(uint8 symbolsBlockId,
		uint8 slot0BlockId) {
	Data::DaxBlock *symbolsBlock = getDax8x8d().getBlockById(symbolsBlockId);
	if (!symbolsBlock)
		error("Failed to load symbols block %u from 8x8d container",
			(unsigned)symbolsBlockId);

	Data::DaxBlock8x8D *symbols8x8 =
		dynamic_cast<Data::DaxBlock8x8D *>(symbolsBlock);
	if (!symbols8x8)
		error("8x8d block %u has unexpected type",
			(unsigned)symbolsBlockId);

	_tileCache.setSlot(4, new Gfx::DaxTile(symbols8x8));

	Data::DaxBlock *slot0Block = getDax8x8d().getBlockById(slot0BlockId);
	if (!slot0Block)
		error("Failed to load fixed tile cache block %u from 8x8d container",
			(unsigned)slot0BlockId);

	Data::DaxBlock8x8D *slot08x8 =
		dynamic_cast<Data::DaxBlock8x8D *>(slot0Block);
	if (!slot08x8)
		error("8x8d block %u has unexpected type",
			(unsigned)slot0BlockId);

	delete _fixedTileCacheSlot0;
	_fixedTileCacheSlot0 = new Gfx::DaxTile(slot08x8);
	_tileCache.setSlot(0, _fixedTileCacheSlot0);
}

uint32 Engine::getFeatures() const {
	return _gameDescription->desc.flags;
}

Common::String Engine::getGameId() const {
	return _gameDescription->desc.gameId;
}

Common::Platform Engine::getPlatform() const {
	return _gameDescription->desc.platform;
}

Common::String Engine::getString(const Common::String &key) const {
    return _strings.getVal(key);
}

Common::Path Engine::resolveGameDataPath() const {
	Common::Path gamePath = ConfMan.getPath("path");
	if (gamePath.empty())
		gamePath = ConfMan.getPath("currentpath");

	return gamePath;
}

Common::Path Engine::resolveSavePath() const {
	Common::Path gamePath = resolveGameDataPath();
	if (!gamePath.empty())
		gamePath.joinInPlace("save");
	if (!gamePath.empty())
		return gamePath;

	// Fallback only when currentpath is unavailable.
	return ConfMan.getPath("savepath");
}

void Engine::setGameState(GameState state) {
	GameState prev = _gameState;
	debug(2, "Engine::setGameState prev=%d next=%d", (int)prev, (int)state);
	_gameState = state;
	onGameStateEnter(prev, state);
}

Common::Error Engine::run() {

	switch (getPlatform()) {
		case Common::kPlatformDOS:
		case Common::kPlatformAmiga:
			initGraphics(320, 200);
			break;
		case Common::kPlatformPC98:
			initGraphics(640, 400);
			break;
		default:
			initGraphics(320, 200);
			break;
	}

	// Set the engine's debugger console
	setDebugger(getConsole());

	runGame();

	return Common::kNoError;
}

Common::Array<Data::PlayerCharacter *> &Engine::getParty() {
    return _party;
}

Common::Error Engine::syncGame(Common::Serializer &s) {
	// The Serializer has methods isLoading() and isSaving()
	// if you need to specific steps; for example setting
	// an array size after reading it's length, whereas
	// for saving it would write the existing array's length
	int dummy = 0;
	s.syncAsUint32LE(dummy);

	return Common::kNoError;
}

} // End of namespace Goldbox
