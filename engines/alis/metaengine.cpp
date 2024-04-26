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

#include "alis/detection.h"
#include "base/plugins.h"
#include "common/system.h"
#include "common/config-manager.h"
#include "common/savefile.h"
#include "common/str-array.h"
#include "common/memstream.h"
#include "common/translation.h"

#include "alis/alis.h"
#include "alis/platform.h"
#include "alis/metaengine.h"

const char *AlisMetaEngine::getName() const {
	return "alis";
}

Common::Error AlisMetaEngine::createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const {
	const Alis::AlisGameDescription *gd = (const Alis::AlisGameDescription *)desc;
	const Alis::sPlatform platform(gd->desc.platform, gd->gameId);
	switch (gd->gameId) {
/*	case Ultima::GAME_ULTIMA1:
		*engine = new Ultima::Shared::UltimaEarlyEngine(syst, gd);
		break;
	case Ultima::GAME_ULTIMA4:
		*engine = new Ultima::Ultima4::Ultima4Engine(syst, gd);
		break;
	case Ultima::GAME_ULTIMA6:
	case Ultima::GAME_MARTIAN_DREAMS:
	case Ultima::GAME_SAVAGE_EMPIRE:
		*engine = new Ultima::Nuvie::NuvieEngine(syst, gd);
		break;
	case Ultima::GAME_ULTIMA8:
	case Ultima::GAME_CRUSADER_REG:
	case Ultima::GAME_CRUSADER_REM:
		*engine = new Ultima::Ultima8::Ultima8Engine(syst, gd);
		break;

	default:
		return Common::kUnsupportedGameidError;
	}
	*/
	return Common::kNoError;
	}
}

int AlisMetaEngine::getMaximumSaveSlot() const {
	return MAX_SAVES;
}

SaveStateList AlisMetaEngine::listSaves(const char *target) const {
	SaveStateList saveList = AdvancedMetaEngine::listSaves(target);

#ifdef ENABLE_ULTIMA6
	Common::String gameId = getGameId(target);
	if (gameId == "ultima6" || gameId == "ultima6_enh")
		Ultima::Nuvie::MetaEngine::listSaves(saveList);
#endif

	return saveList;
}

SaveStateDescriptor AlisMetaEngine::querySaveMetaInfos(const char *target, int slot) const {
	SaveStateDescriptor desc = AdvancedMetaEngine::querySaveMetaInfos(target, slot);
	if (!desc.isValid() && slot > 0) {
#ifdef ENABLE_ULTIMA8
		Common::String gameId = getGameId(target);
		if (gameId == "ultima8") {
			Common::String filename = getSavegameFile(slot, target);
			desc = SaveStateDescriptor(this, slot, Common::U32String());
			if (!Ultima::Ultima8::MetaEngine::querySaveMetaInfos(filename, desc))
				return SaveStateDescriptor();
		}
#endif
	}

	return desc;
}

Common::KeymapArray AlisMetaEngine::initKeymaps(const char *target) const {
#if defined(ENABLE_ULTIMA4) || defined(ENABLE_ULTIMA6) || defined(ENABLE_ULTIMA8)
	const Common::String gameId = getGameId(target);
#endif

#ifdef ENABLE_ULTIMA4
	if (gameId == "ultima4" || gameId == "ultima4_enh")
		return Ultima::Ultima4::MetaEngine::initKeymaps();
#endif
#ifdef ENABLE_ULTIMA6
	if (gameId == "ultima6" || gameId == "ultima6_enh"
			|| gameId == "martiandreams" || gameId == "martiandreams_enh"
			|| gameId == "savageempire" || gameId == "savageempire_enh")
		return Ultima::Nuvie::MetaEngine::initKeymaps(gameId);
#endif
#ifdef ENABLE_ULTIMA8
	if (gameId == "ultima8" || gameId == "remorse" || gameId == "regret")
		return Ultima::Ultima8::MetaEngine::initKeymaps(gameId);
#endif
	return Common::KeymapArray();
}

Common::String AlisMetaEngine::getGameId(const Common::String& target) {
	// Store a copy of the active domain
	Common::String currDomain = ConfMan.getActiveDomainName();

	// Switch to the given target domain and get it's game Id
	ConfMan.setActiveDomain(target);
	Common::String gameId = ConfMan.get("gameid");

	// Switch back to the original domain and return the game Id
	ConfMan.setActiveDomain(currDomain);
	return gameId;
}

#if PLUGIN_ENABLED_DYNAMIC(ULTIMA)
	REGISTER_PLUGIN_DYNAMIC(ULTIMA, PLUGIN_TYPE_ENGINE, AlisMetaEngine);
#else
	REGISTER_PLUGIN_STATIC(ULTIMA, PLUGIN_TYPE_ENGINE, AlisMetaEngine);
#endif
