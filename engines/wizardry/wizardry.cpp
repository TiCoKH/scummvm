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

#include "wizardry/wizardry.h"
#include "wizardry/detection.h"
#include "wizardry/console.h"
#include "common/scummsys.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/events.h"
#include "common/rendermode.h"
#include "common/system.h"
#include "engines/util.h"
#include "graphics/paletteman.h"

namespace Wizardry {

WizardryEngine *g_engine;

WizardryEngine::WizardryEngine(OSystem *syst, const WizardryGameDescription *gameDesc) : Engine(syst),
	_gameDescription(gameDesc), _randomSource("Wizardry") {
	g_engine = this;
}

WizardryEngine::~WizardryEngine() {
	delete _speaker;
}

void WizardryEngine::speakerCue(int cue) {
	// The IBM-PC driver's tone: timer divisor 533, about 2239 Hz. Its
	// lengths are busy loops: 0x2000 and 0x400 rounds for the long and
	// the short sound, 0x8000 between two short ones, here taken at the
	// speed of a 4.77 MHz PC.
	enum {
		kDivisor = 533,
		kLongUs = 29000,
		kShortUs = 4000,
		kGapUs = 117000
	};
	const float freq = 1193182.0f / kDivisor;

	if (!_speaker)
		return;

	switch (cue) {
	case kCueBeep:
		_speaker->playQueue(Audio::PCSpeaker::kWaveFormSquare, freq, kLongUs);
		break;
	case kCueStep:
		_speaker->playQueue(Audio::PCSpeaker::kWaveFormSquare, freq, kShortUs);
		break;
	case kCueWall:
		_speaker->playQueue(Audio::PCSpeaker::kWaveFormSquare, freq, kShortUs);
		_speaker->playQueue(Audio::PCSpeaker::kWaveFormSilence, 0, kGapUs);
		_speaker->playQueue(Audio::PCSpeaker::kWaveFormSquare, freq, kShortUs);
		break;
	default:
		break;
	}
}

uint32 WizardryEngine::getFeatures() const {
	return _gameDescription->desc.flags;
}

GameType WizardryEngine::getGameType() const {
	return (GameType)_gameDescription->game;
}

DataFamily WizardryEngine::getFamily() const {
	return (DataFamily)_gameDescription->family;
}

Common::Platform WizardryEngine::getPlatform() const {
	return _gameDescription->desc.platform;
}

bool WizardryEngine::isDiskImage() const {
	return (_gameDescription->features & kFeatureDiskImage) != 0;
}

const GfxSet &WizardryEngine::getGfxSet() const {
	assert(_profile);
	bool high;

	if (getPlatform() == Common::kPlatformDOS) {
		// The original picks the two-colour set when Alt is held at start-up
		high = ConfMan.hasKey("render_mode") &&
			Common::parseRenderMode(ConfMan.get("render_mode")) == Common::kRenderCGA_BW;
	} else {
		high = !ConfMan.hasKey("hires") || ConfMan.getBool("hires");
	}

	if (high && !_profile->high.isPresent())
		high = false;
	if (!high && !_profile->low.isPresent())
		high = true;

	return high ? _profile->high : _profile->low;
}

Common::String WizardryEngine::getGameId() const {
	return _gameDescription->desc.gameId;
}

Common::Error WizardryEngine::run() {
	if (getFamily() != kFamilyApple) {
		_profile = findProfile(getGameType(), getPlatform());
		if (!_profile)
			error("No profile for %s on %s", getGameId().c_str(),
				Common::getPlatformDescription(getPlatform()));
	}

	if (_profile) {
		// 320x200 for the four-colour CGA set. Everything else is 640
		// wide and is shown at 640x400; see TileRenderer.
		const GfxSet &gfx = getGfxSet();
		initGraphics(gfx.screenW, gfx.screenW == 320 ? 200 : 400);
	} else {
		initGraphics(320, 200);
	}

	// Set the engine's debugger console
	setDebugger(new Console());

	// The speaker of the DOS and PC-98 versions
	if (getPlatform() == Common::kPlatformDOS || getPlatform() == Common::kPlatformPC98) {
		_speaker = new Audio::PCSpeaker();
		_speaker->init();
	}

	// Setup globals
	g_globals->setup();

	// Play the game
	runGame();

	return Common::kNoError;
}

void WizardryEngine::applyGameSettings() {
	if (_profile) {
		_globals.applyPalette();
		_globals.loadMessages();
	}
}

Common::Error WizardryEngine::syncGame(Common::Serializer &s) {
	// Only the releases with a game to play have anything to save
	if (!hasTown()) {
		int dummy = 0;
		s.syncAsUint32LE(dummy);
		return Common::kNoError;
	}

	// What the original writes back into its scenario file: the
	// characters, who is in the party, the stock of the trading post, and
	// (from version 2) the maze levels as they are after what happened on
	// them. Version 1 saves have no levels; the levels then start afresh.
	enum {
		kSaveVersion = 2,
		kMaxLevels = 64
	};
	if (!s.syncVersion(kSaveVersion))
		return Common::kReadingFailed;

	if (s.isSaving()) {
		_globals._party.synchronize(s);
		_globals._items.synchronize(s);

		uint16 count = _globals._mazeLevels.size();
		s.syncAsUint16LE(count, 2);
		for (uint i = 0; i < count; ++i)
			_globals._mazeLevels[i].synchronize(s);
		return Common::kNoError;
	}

	// Loading: everything is read into copies first, so a save that is
	// damaged or does not belong to this game changes nothing
	Party party;
	ItemList items = _globals._items;
	if (!party.synchronize(s) || !items.synchronize(s))
		return Common::kReadingFailed;

	uint16 count = 0;
	s.syncAsUint16LE(count, 2);
	if (count > kMaxLevels)
		return Common::kReadingFailed;

	Common::Array<MazeLevel> levels;
	levels.resize(count);
	for (uint i = 0; i < count; ++i)
		levels[i].synchronize(s);
	if (s.err())
		return Common::kReadingFailed;

	_globals._party = party;
	_globals._items = items;
	_globals._mazeLevels = levels;

	// The view on screen may show what is no longer true
	if (focusedView())
		replaceView("Town.Castle", true);

	return Common::kNoError;
}

} // End of namespace Wizardry
