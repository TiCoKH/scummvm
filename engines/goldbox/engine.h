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

#ifndef GOLDBOX_ENGINE_H
#define GOLDBOX_ENGINE_H

#include "common/system.h"
#include "common/error.h"
#include "common/list.h"
#include "common/random.h"
#include "common/serializer.h"
#include "graphics/font.h"
#include "engines/engine.h"
#include "engines/savestate.h"

#include "goldbox/core/global.h"
#include "goldbox/detection.h"
#include "goldbox/events.h"
#include "goldbox/sound/sound_driver.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/strings_data.h"
#include "goldbox/data/items/base_items.h"
#include "goldbox/data/daxfilemanager.h"
#include "goldbox/gfx/walldef_surface_builder.h"
#include "goldbox/gfx/viewport_background.h"
#include "goldbox/gfx/area_map_cache.h"
#include "goldbox/gfx/encounter_sprite_cache.h"
#include "goldbox/gfx/picture_display_cache.h"
#include "goldbox/runtime/runtime_geo.h"

namespace Goldbox {

struct GoldboxGameDescription;
class RuntimeExchange;
class TreasurePool;

class Engine : public ::Engine, public Events {
private:
	const GoldboxGameDescription *_gameDescription;
	Common::RandomSource _randomSource;
	GameState _gameState = GS_START_MENU;

	GoldboxSoundDriver *_soundDriver;  ///< PC Speaker / Tandy sound driver
	SoundMode _soundMode;              ///< Current sound mode from config
protected:
	Data::DaxFileManager _daxManager;
	RuntimeGeoBlock _runtimeGeo;
	TreasurePool *_treasurePool;
	Gfx::ViewportBackground _viewportBg;
	Gfx::AreaMapCache _areaMapCache;
	Gfx::EncounterSpriteCache _encounterSpriteCache;
	Gfx::PictureDisplayCache _pictureDisplayCache;
	// Engine APIs
	Common::Error run() override;
	void setup() override;
	virtual GUI::Debugger *getConsole() = 0;

	/**
	 * Initialization template hooks called by Engine::setup().
	 * Derived engines override these to organize startup logic by purpose.
	 */
	virtual bool initializeGameData() {
		return true;
	}
	virtual bool loadGameAssets() {
		return true;
	}
	virtual bool initializeRuntimeSystems() {
		return true;
	}

	/**
	 * Returns true if the game should quit
	 */
	bool shouldQuit() const override {
		return ::Engine::shouldQuit();
	}

public:
	Graphics::Font * _font = nullptr;
	Gfx::WalldefSlotCache _walldefSlotCache;
	Gfx::Tile8x8Cache _tileCache;
	Gfx::DaxTile *_fixedTileCacheSlot0 = nullptr;
	Data::StringsData _strings;
	Common::List<Data::PlayerCharacter *> _party;
	Data::PlayerCharacter * _selectedCharacter = nullptr;
	Data::PlayerCharacter * _targetCharacter = nullptr;
	Data::PlayerCharacter * _nextCharacter = nullptr;
	uint _textDelay = 3; // 1-5 range (1=fastest, 5=slowest), maps to original BYTE_DELAY
	static Goldbox::Data::Items::Storage gItemProps;

	uint8 _skyColor = 0;
	uint8 _skylineColor = 0;
	uint8 _horizonColor = 0;
	uint8 _floorColor = 0;

	/**
	 * Play a sound effect or song by ID.
	 * Matches the original SOUND_Play(s_id) convention:
	 *   0    = stop all, disable speaker
	 *   1    = enable speaker (unmute)
	 *   0xFF = stop songs, keep speaker enabled
	 *   2+   = play song (1-based index)
	 */
	void soundPlay(uint8 songId);

	/**
	 * Returns the current sound mode.
	 */
	SoundMode getSoundMode() const { return _soundMode; }

	/**
	 * Gets the current game state.
	 */
	GameState getGameState() const { return _gameState; }

	/**
	 * Sets the current game state and triggers state-enter hook.
	 */
	void setGameState(GameState state);

	/**
	 * Hook called on every game state transition.
	 * Default implementation does nothing; engines override to route screens.
	 */
	virtual void onGameStateEnter(GameState prev, GameState next) {}


	void setColors(uint8 sky, uint8 skyline, uint8 horizon, uint8 floor) {
		_skyColor = sky;
		_skylineColor = skyline;
		_horizonColor = horizon;
		_floorColor = floor;
		_viewportBg.set3DViewportColors(sky, skyline, horizon, floor);
	}

	const Gfx::ViewportBackground &getViewportBackground() const {
		return _viewportBg;
	}

	Gfx::AreaMapCache &getAreaMapCache() { return _areaMapCache; }
	const Gfx::AreaMapCache &getAreaMapCache() const { return _areaMapCache; }

	Gfx::EncounterSpriteCache &getEncounterSpriteCache() { return _encounterSpriteCache; }
	const Gfx::EncounterSpriteCache &getEncounterSpriteCache() const { return _encounterSpriteCache; }

	Gfx::PictureDisplayCache &getPictureDisplayCache() { return _pictureDisplayCache; }
	const Gfx::PictureDisplayCache &getPictureDisplayCache() const { return _pictureDisplayCache; }

	/**
	 * Returns the currently selected character.
	 */
	Data::PlayerCharacter *getSelectedCharacter() const { return _selectedCharacter; }

	/**
	 * Sets the currently selected character.
	 */
	void setSelectedCharacter(Data::PlayerCharacter *character) { _selectedCharacter = character; }

	/**
	 * Returns the currently selected trade/combat target character.
	 */
	Data::PlayerCharacter *getTargetCharacter() const { return _targetCharacter; }

	/**
	 * Sets the currently selected trade/combat target character.
	 */
	void setTargetCharacter(Data::PlayerCharacter *character) { _targetCharacter = character; }

	/**
	 * Gets the text display delay (1-5 range).
	 * 1 = fastest (shortest display time)
	 * 5 = slowest (longest display time)
	 * Maps to original BYTE_DELAY: low value = fast, high value = slow
	 */
	uint getTextDelay() const { return _textDelay; }

	/**
	 * Sets the text display delay (1-5 range).
	 * Affects message display duration in PromptMessage and other text dialogs.
	 * Low value = fast display, High value = slow display.
	 */
	void setTextDelay(uint delay) {
		if (delay < 1) delay = 1;
		if (delay > 5) delay = 5;
		_textDelay = delay;
	}


	Engine(OSystem *syst, const GoldboxGameDescription *gameDesc);
	~Engine() override;

	uint32 getFeatures() const;

	/**
	 * Returns the game Id
	 */
	Common::String getGameId() const;

	/**
	 * Returns the platform
	 */
	Common::Platform getPlatform() const;

	/**
	 * Gets a random number
	 */
	uint32 getRandomNumber(uint maxNum) {
		return _randomSource.getRandomNumber(maxNum);
	}
	uint32 getRandomNumber(uint minVal, uint maxVal) {
		return _randomSource.getRandomNumberRng(minVal, maxVal);
	}
	Common::RandomSource &getRandomSource() {
		return _randomSource;
	}

	int rollDice(int number, int sides) {
		int total = 0;
		for (int i = 0; i < number; ++i) {
			total += getRandomNumber(sides) + 1;
		}
		return total;
	}

	/**
	 * Retrieves a string from StringsData with a default value if the key is not found.
	 * @param key The key of the string to retrieve.
	 * @return The string associated with the key, or the default value if not found.
	 */
	Common::String getString(const Common::String &key) const;

	/**
	 * Resolves the active game data root folder for Goldbox content.
	 *
	 * Priority:
	 *   1) target path
	 *   2) currentpath
	 */
	Common::Path resolveGameDataPath() const;

	/**
	 * Resolves the legacy Goldbox save directory.
	 *
	 * Legacy game artifacts (.SAV/.CHA/.ITM/.SPC/.DAT/CHARLIST.TXT) are
	 * intentionally stored in the game data folder under "save".
	 */
	Common::Path resolveSavePath() const;

	bool hasFeature(EngineFeature f) const override {
		return
			(f == kSupportsLoadingDuringRuntime) ||
			(f == kSupportsSavingDuringRuntime) ||
			(f == kSupportsReturnToLauncher);
	};

	bool canLoadGameStateCurrently(Common::U32String *msg = nullptr) override {
		return true;
	}
	bool canSaveGameStateCurrently(Common::U32String *msg = nullptr) override {
		return true;
	}

	Common::List<Data::PlayerCharacter *> &getParty();

	/**
	 * DAX File Management - Accessors for all DAX containers
	 * These containers persist across the entire engine lifetime
	 * and are cleaned up only when the engine is destroyed.
	 */
	Data::DaxBlockContainer &getDax8x8d() { return _daxManager.get8x8d(); }
	Data::DaxBlockContainer &getDaxBacpac() { return _daxManager.getBacpac(); }
	Data::DaxBlockContainer &getDaxDungcom() { return _daxManager.getDungcom(); }
	Data::DaxBlockContainer &getDaxRandcom() { return _daxManager.getRandcom(); }
	Data::DaxBlockContainer &getDaxSqrpaci() { return _daxManager.getSqrpaci(); }
	Data::DaxBlockContainer &getDaxBody() { return _daxManager.getBody(); }
	Data::DaxBlockContainer &getDaxCBody() { return _daxManager.getCBody(); }
	Data::DaxBlockContainer &getDaxCHead() { return _daxManager.getCHead(); }
	Data::DaxBlockContainer &getDaxComSpr() { return _daxManager.getComSpr(); }
	Data::DaxBlockContainer &getDaxEcl() { return _daxManager.getEcl(); }
	Data::DaxBlockContainer &getDaxGeo() { return _daxManager.getGeo(); }
	Data::DaxBlockContainer &getDaxHead() { return _daxManager.getHead(); }
	Data::DaxBlockContainer &getDaxMonCha() { return _daxManager.getMonCha(); }
	Data::DaxBlockContainer &getDaxMonItm() { return _daxManager.getMonItm(); }
	Data::DaxBlockContainer &getDaxItem() { return _daxManager.getItem(); }
	Data::DaxBlockContainer &getDaxMonSpc() { return _daxManager.getMonSpc(); }
	Data::DaxBlockContainer &getDaxPic() { return _daxManager.getPic(); }
	Data::DaxBlockContainer &getDaxCPic() { return _daxManager.getCPic(); }
	Data::DaxBlockContainer &getDaxSprit() { return _daxManager.getSprit(); }
	Data::DaxBlockContainer &getDaxTitle() { return _daxManager.getTitle(); }
	Data::DaxBlockContainer &getDaxWalldef() { return _daxManager.getWalldef(); }
	Data::DaxBlockContainer &getDaxWildcom() { return _daxManager.getWildcom(); }

	// Const accessors
	const Data::DaxBlockContainer &getDax8x8d() const { return _daxManager.get8x8d(); }
	const Data::DaxBlockContainer &getDaxBacpac() const { return _daxManager.getBacpac(); }
	const Data::DaxBlockContainer &getDaxDungcom() const { return _daxManager.getDungcom(); }
	const Data::DaxBlockContainer &getDaxRandcom() const { return _daxManager.getRandcom(); }
	const Data::DaxBlockContainer &getDaxSqrpaci() const { return _daxManager.getSqrpaci(); }
	const Data::DaxBlockContainer &getDaxBody() const { return _daxManager.getBody(); }
	const Data::DaxBlockContainer &getDaxCBody() const { return _daxManager.getCBody(); }
	const Data::DaxBlockContainer &getDaxCHead() const { return _daxManager.getCHead(); }
	const Data::DaxBlockContainer &getDaxComSpr() const { return _daxManager.getComSpr(); }
	const Data::DaxBlockContainer &getDaxEcl() const { return _daxManager.getEcl(); }
	const Data::DaxBlockContainer &getDaxGeo() const { return _daxManager.getGeo(); }
	const Data::DaxBlockContainer &getDaxHead() const { return _daxManager.getHead(); }
	const Data::DaxBlockContainer &getDaxMonCha() const { return _daxManager.getMonCha(); }
	const Data::DaxBlockContainer &getDaxMonItm() const { return _daxManager.getMonItm(); }
	const Data::DaxBlockContainer &getDaxItem() const { return _daxManager.getItem(); }
	const Data::DaxBlockContainer &getDaxMonSpc() const { return _daxManager.getMonSpc(); }
	const Data::DaxBlockContainer &getDaxPic() const { return _daxManager.getPic(); }
	const Data::DaxBlockContainer &getDaxCPic() const { return _daxManager.getCPic(); }
	const Data::DaxBlockContainer &getDaxSprit() const { return _daxManager.getSprit(); }
	const Data::DaxBlockContainer &getDaxTitle() const { return _daxManager.getTitle(); }
	const Data::DaxBlockContainer &getDaxWalldef() const { return _daxManager.getWalldef(); }
	const Data::DaxBlockContainer &getDaxWildcom() const { return _daxManager.getWildcom(); }

	/**
	 * Direct access to the DaxFileManager for advanced operations
	 */
	Data::DaxFileManager &getDaxManager() { return _daxManager; }
	const Data::DaxFileManager &getDaxManager() const { return _daxManager; }

	Gfx::WalldefSlotCache &getWalldefSlotCache() { return _walldefSlotCache; }
	const Gfx::WalldefSlotCache &getWalldefSlotCache() const {
		return _walldefSlotCache;
	}

	RuntimeGeoBlock &getRuntimeGeo() { return _runtimeGeo; }
	const RuntimeGeoBlock &getRuntimeGeo() const { return _runtimeGeo; }

	TreasurePool &getTreasurePool();
	const TreasurePool &getTreasurePool() const;

	Gfx::Tile8x8Cache &getTileCache() { return _tileCache; }
	const Gfx::Tile8x8Cache &getTileCache() const { return _tileCache; }

	/**
	 * Load and bind the fixed 8x8 tile cache slots used by original Goldbox
	 * runtimes:
	 *   - symbolsBlockId -> slot 4
	 *   - slot0BlockId   -> slot 0
	 */
	void initFixedTileCacheSlots(uint8 symbolsBlockId = 202,
			uint8 slot0BlockId = 203);

	/**
	 * Uses a serializer to allow implementing savegame
	 * loading and saving using a single method
	 */
	Common::Error syncGame(Common::Serializer &s);

	Common::Error saveGameStream(Common::WriteStream *stream, bool isAutosave = false) override {
		Common::Serializer s(nullptr, stream);
		return syncGame(s);
	}
	Common::Error loadGameStream(Common::SeekableReadStream *stream) override {
		Common::Serializer s(stream, nullptr);
		return syncGame(s);
	}

	/**
	 * Runtime data exchange bridge used by game-specific engines.
	 * Base engine exposes a nullable hook so callers can use a generic
	 * contract without depending on a specific game implementation.
	 */
	virtual RuntimeExchange *getRuntimeExchange() {
		return nullptr;
	}

	virtual const RuntimeExchange *getRuntimeExchange() const {
		return nullptr;
	}
};

extern Engine *g_engine;
#define SHOULD_QUIT ::Goldbox::g_engine->shouldQuit();

} // End of namespace Goldbox

#endif // GOLDBOX_ENGINE_H
