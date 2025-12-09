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

#include "common/scummsys.h"
#include "common/system.h"
#include "common/error.h"
#include "common/fs.h"
#include "common/hash-str.h"
#include "common/random.h"
#include "common/serializer.h"
#include "common/util.h"
#include "common/array.h"
#include "graphics/font.h"
#include "engines/engine.h"
#include "engines/savestate.h"
#include "graphics/screen.h"

#include "goldbox/detection.h"
#include "goldbox/events.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/strings_data.h"
#include "goldbox/data/items/base_items.h"
#include "goldbox/data/daxfilemanager.h"

namespace Goldbox {

struct GoldboxGameDescription;

class Engine : public ::Engine, public Events {
private:
	const GoldboxGameDescription *_gameDescription;
	Common::RandomSource _randomSource;
protected:
	Data::DaxFileManager _daxManager;
	// Engine APIs
	Common::Error run() override;
	virtual GUI::Debugger *getConsole() = 0;

	/**
	 * Returns true if the game should quit
	 */
	bool shouldQuit() const override {
		return false; // TODO
	}

public:
	Graphics::Font * _font = nullptr;
	Graphics::Font * _symbols = nullptr;
	Data::StringsData _strings;
	Common::Array<Data::PlayerCharacter *> _party;
	Data::PlayerCharacter * _selectedCharacter = nullptr;
	Data::PlayerCharacter * _nextCharacter = nullptr;
	static Goldbox::Data::Items::Storage gItemProps;

	/**
	 * Returns the currently selected character.
	 */
	Data::PlayerCharacter *getSelectedCharacter() const { return _selectedCharacter; }

	/**
	 * Sets the currently selected character.
	 */
	void setSelectedCharacter(Data::PlayerCharacter *character) { _selectedCharacter = character; }


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

	Common::Array<Data::PlayerCharacter *> &getParty();

	/**
	 * DAX File Management - Accessors for all DAX containers
	 * These containers persist across the entire engine lifetime
	 * and are cleaned up only when the engine is destroyed.
	 */
	Data::DaxBlockContainer &getDax8x8d() { return _daxManager.get8x8d(); }
	Data::DaxBlockContainer &getDaxBody() { return _daxManager.getBody(); }
	Data::DaxBlockContainer &getDaxCBody() { return _daxManager.getCBody(); }
	Data::DaxBlockContainer &getDaxCHead() { return _daxManager.getCHead(); }
	Data::DaxBlockContainer &getDaxComSpr() { return _daxManager.getComSpr(); }
	Data::DaxBlockContainer &getDaxEcl() { return _daxManager.getEcl(); }
	Data::DaxBlockContainer &getDaxGeo() { return _daxManager.getGeo(); }
	Data::DaxBlockContainer &getDaxHead() { return _daxManager.getHead(); }
	Data::DaxBlockContainer &getDaxMonCha() { return _daxManager.getMonCha(); }
	Data::DaxBlockContainer &getDaxMonItm() { return _daxManager.getMonItm(); }
	Data::DaxBlockContainer &getDaxMonSpc() { return _daxManager.getMonSpc(); }
	Data::DaxBlockContainer &getDaxPic() { return _daxManager.getPic(); }
	Data::DaxBlockContainer &getDaxCPic() { return _daxManager.getCPic(); }
	Data::DaxBlockContainer &getDaxSprit() { return _daxManager.getSprit(); }
	Data::DaxBlockContainer &getDaxTitle() { return _daxManager.getTitle(); }
	Data::DaxBlockContainer &getDaxWalldef() { return _daxManager.getWalldef(); }

	// Const accessors
	const Data::DaxBlockContainer &getDax8x8d() const { return _daxManager.get8x8d(); }
	const Data::DaxBlockContainer &getDaxBody() const { return _daxManager.getBody(); }
	const Data::DaxBlockContainer &getDaxCBody() const { return _daxManager.getCBody(); }
	const Data::DaxBlockContainer &getDaxCHead() const { return _daxManager.getCHead(); }
	const Data::DaxBlockContainer &getDaxComSpr() const { return _daxManager.getComSpr(); }
	const Data::DaxBlockContainer &getDaxEcl() const { return _daxManager.getEcl(); }
	const Data::DaxBlockContainer &getDaxGeo() const { return _daxManager.getGeo(); }
	const Data::DaxBlockContainer &getDaxHead() const { return _daxManager.getHead(); }
	const Data::DaxBlockContainer &getDaxMonCha() const { return _daxManager.getMonCha(); }
	const Data::DaxBlockContainer &getDaxMonItm() const { return _daxManager.getMonItm(); }
	const Data::DaxBlockContainer &getDaxMonSpc() const { return _daxManager.getMonSpc(); }
	const Data::DaxBlockContainer &getDaxPic() const { return _daxManager.getPic(); }
	const Data::DaxBlockContainer &getDaxCPic() const { return _daxManager.getCPic(); }
	const Data::DaxBlockContainer &getDaxSprit() const { return _daxManager.getSprit(); }
	const Data::DaxBlockContainer &getDaxTitle() const { return _daxManager.getTitle(); }
	const Data::DaxBlockContainer &getDaxWalldef() const { return _daxManager.getWalldef(); }

	/**
	 * Direct access to the DaxFileManager for advanced operations
	 */
	Data::DaxFileManager &getDaxManager() { return _daxManager; }
	const Data::DaxFileManager &getDaxManager() const { return _daxManager; }

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
};

extern Engine *g_engine;
#define SHOULD_QUIT ::Goldbox::g_engine->shouldQuit()

} // End of namespace Goldbox

#endif // GOLDBOX_ENGINE_H
