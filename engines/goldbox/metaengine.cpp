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

#include "common/translation.h"

#include "goldbox/metaengine.h"
#include "goldbox/detection.h"
//#include "goldbox/keymapping.h"
#include "goldbox/game_factory.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/curse/curse.h"
#include "goldbox/engine.h"

namespace Goldbox {

namespace {
// Anchors that force each game's translation unit (and its
// GameFactory::Registrar static initializer) to be linked into static
// builds. Adding a new game means adding one term here plus its own
// self-registering .cpp file - the createInstance() dispatch below never
// needs to change.
volatile int gForceLinkGames =
	Goldbox::Poolrad::kForceLinkPoolrad +
	Goldbox::Curse::kForceLinkCurse;
} // End of anonymous namespace

static const ADExtraGuiOptionsMap optionsList[] = {
	{
		GAMEOPTION_ORIGINAL_SAVELOAD,
		{
			_s("Use original save/load screens"),
			_s("Use the original save/load screens instead of the ScummVM ones"),
			"original_menus",
			false,
			0,
			0
		}
	},
	{
		GAMEOPTION_SOUND_MODE,
		{
			_s("Sound device"),
			_s("Select sound output: Tandy 3-voice, PC Speaker, or Off"),
			"sound_mode",
			true,
			0,
			0
		}
	},
	AD_EXTRA_GUI_OPTIONS_TERMINATOR
};

} // End of namespace Goldbox

const char *GoldboxMetaEngine::getName() const {
	return "goldbox";
}

const ADExtraGuiOptionsMap *GoldboxMetaEngine::getAdvancedExtraGuiOptions() const {
	return Goldbox::optionsList;
}

Common::Error GoldboxMetaEngine::createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const {
	const auto *gd = (Goldbox::GoldboxGameDescription *)desc;

	Goldbox::Engine *goldboxEngine = Goldbox::GameFactory::create(gd->gameType, syst, gd);
	if (!goldboxEngine)
		error("Unknown or unsupported game type %d", gd->gameType);

	*engine = goldboxEngine;
	return Common::kNoError;
}

bool GoldboxMetaEngine::hasFeature(MetaEngineFeature f) const {
	return checkExtendedSaves(f) ||
		(f == kSupportsLoadingDuringStartup);
}
/*
Common::KeymapArray GoldboxMetaEngine::initKeymaps(const char *target) const {
	return Goldbox::Keymapping::initKeymaps();
}
*/
#if PLUGIN_ENABLED_DYNAMIC(GOLDBOX)
REGISTER_PLUGIN_DYNAMIC(GOLDBOX, PLUGIN_TYPE_ENGINE, GoldboxMetaEngine);
#else
REGISTER_PLUGIN_STATIC(GOLDBOX, PLUGIN_TYPE_ENGINE, GoldboxMetaEngine);
#endif
