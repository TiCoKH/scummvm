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
 * Assisted-by: Claude:claude-opus-4.6
 *
 */

#ifndef GOLDBOX_ENGINE_H
#define GOLDBOX_ENGINE_H

#include "common/system.h"
#include "common/error.h"
#include "common/list.h"
#include "common/random.h"
#include "common/serializer.h"
#include "common/translation.h"
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
#include "goldbox/gfx/game_text.h"
#include "goldbox/runtime/runtime_geo.h"

namespace Goldbox {

struct GoldboxGameDescription;
class RuntimeExchange;
namespace Runtime { class TreasurePool; }
namespace Data { namespace Effects { class EffectHandlerBase; } }
namespace Spells { class SpellCastingService; }
namespace Combat { struct CombatContext; }

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
	Runtime::TreasurePool            *_treasurePool;
	Data::Effects::EffectHandlerBase *_effectHandler = nullptr;
	Spells::SpellCastingService      *_spellCasting  = nullptr;
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
	GameText _gameText;
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

	// Mirrors IN_SPELL_PROCESS: set true while a spell or effect-triggered
	// action is being resolved, to suppress re-entrant EFFECT_add calls.
	bool _inSpellProcess = false;

	uint8 _skyColor = 0;
	uint8 _skylineColor = 0;
	uint8 _horizonColor = 0;
	uint8 _floorColor = 0;

	bool isInSpellProcess() const { return _inSpellProcess; }
	void setInSpellProcess(bool active) { _inSpellProcess = active; }

	/**
	 * Play a sound by ARRAY_SOUND_MAP index, like the original
	 * SOUND_Play(ARRAY_SOUND_MAP[index]):
	 *   0  = silence
	 *   1  = sound off (mute)
	 *   2  = sound on (unmute)
	 *   3+ = play sound table entry (index - 2)
	 */
	void soundPlay(uint8 soundIndex);

	/**
	 * SOUND_Setup: 0 starts continuous playback (sounds keep playing
	 * until the next non-zero setup); non-zero selects short blip mode.
	 */
	void soundSetup(uint16 flag);

	/** WORD_SOUND_FLAG, also written by ECL scripts. */
	void setSoundFlag(uint16 flag);
	uint16 getSoundFlag() const;

	/**
	 * Selects the sound data file for a given "sound_version" config value:
	 * "1.0" -> "sound-v10.dat"; "1.3", empty (default) and any other string
	 * -> "sound.dat". When @p versionRecognized is non-null it is set false
	 * for an unrecognized (non-empty, non-"1.0"/"1.3") value so the caller can
	 * warn. Pure helper used by setup(); no engine state is touched.
	 */
	static const char *soundVersionDataFile(const Common::String &version,
			bool *versionRecognized = nullptr);

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

	GameText &getGameText() { return _gameText; }
	const GameText &getGameText() const { return _gameText; }

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
		assert(number >= 0 && sides > 0);
		int total = 0;
		for (int i = 0; i < number; ++i) {
			total += getRandomNumber(sides - 1) + 1;
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

	// GB13-033 native-save capability sentinel.
	//
	// 24.1 contained native save (false) while syncGame() was a placeholder;
	// 24.2 added the versioned schema + serializer; 24.3 made native LOAD
	// transactional (validate-into-temporary -> check -> swap, so a failed load
	// leaves the previous game intact) and bounded SAVE to stable map states.
	// With both sides safe, native save/load is now advertised again. Saving is
	// still DISABLED in combat/rest/modal/suspended contexts by the live
	// boundary predicate (see canSaveGameStateCurrently), NOT by this constant.
	static const bool kNativeSaveImplemented = true;

	// Pure, headless-testable capability decision. Native save/load is only
	// allowed when the real serializer exists AND (in 24.3) the current
	// context permits it. The contextAllows seam is reserved for the
	// modal/combat/rest gating required by Requirement 30.3; for now the
	// sentinel short-circuits it to false.
	static bool nativeSaveAllowed(bool contextAllows = true) {
		return kNativeSaveImplemented && contextAllows;
	}

	bool hasFeature(EngineFeature f) const override {
		// Runtime save/load track kNativeSaveImplemented (GB13-033 24.3: true).
		// kSupportsReturnToLauncher is always honest.
		if (f == kSupportsReturnToLauncher)
			return true;
		if (f == kSupportsSavingDuringRuntime || f == kSupportsLoadingDuringRuntime)
			return kNativeSaveImplemented;
		return false;
	};

	bool canLoadGameStateCurrently(Common::U32String *msg = nullptr) override {
		// GB13-033 24.3: loading is honest whenever native save is implemented.
		// Unlike saving, loading has no stable-boundary requirement: the load is
		// transactional (parse -> validate -> swap) so it is safe at startup and
		// at any point; a bad/truncated/wrong-profile/missing-resource save
		// surfaces a truthful error and leaves the current game intact. Startup
		// load routes through the same transactional loadGameStream path.
		if (!nativeSaveAllowed()) {
			if (msg)
				*msg = _("Loading is not supported in this engine.");
			return false;
		}
		return true;
	}
	bool canSaveGameStateCurrently(Common::U32String *msg = nullptr) override {
		// Base default: no live context predicate. Game engines that support
		// native save (Poolrad) OVERRIDE this to AND in a stable-boundary
		// predicate so saving is disabled in modal/combat/rest contexts
		// (Requirement 30.3) instead of being advertised.
		if (!nativeSaveAllowed()) {
			if (msg)
				*msg = _("Saving is not yet supported in this engine.");
			return false;
		}
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

	Runtime::TreasurePool &getTreasurePool();
	const Runtime::TreasurePool &getTreasurePool() const;

	Data::Effects::EffectHandlerBase *getEffectHandler() { return _effectHandler; }
	const Data::Effects::EffectHandlerBase *getEffectHandler() const { return _effectHandler; }

	Spells::SpellCastingService *getSpellCasting() { return _spellCasting; }
	const Spells::SpellCastingService *getSpellCasting() const { return _spellCasting; }

	/** Nullable - only valid during an active combat session. */
	Combat::CombatContext *getCombatContext();
	const Combat::CombatContext *getCombatContext() const;

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

	/**
	 * GB13-033 24.2 game-specific native serializer seam.
	 *
	 * syncGame() routes here so a game engine can drive its own versioned
	 * native schema (load+save in one pass via Serializer::isLoading()/
	 * isSaving()). The base default returns a clear error so an engine without
	 * a native serializer (e.g. Curse) fails honestly instead of accepting a
	 * placeholder payload. The base engine stays free of game-specific
	 * includes; the Poolrad override lives in the poolrad/ layer.
	 */
	virtual Common::Error syncGameState(Common::Serializer &s);

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
