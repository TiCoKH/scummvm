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

#include "alis/alisvm.h"
#include "alis/platform.h"
#include "alis/metaengine.h"

const char *AlisMetaEngine::getName() const {
    return "alis";
}

Common::Error AlisMetaEngine::createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const {
    const Alis::AlisGameDescription *gd = (const Alis::AlisGameDescription *)desc;
    switch (gd->gameId) {
    case Alis::GameId::GAME_ISHAR1:
        *engine = new Alis::AlisVM(syst, gd);
        break;
    default:
        return Common::kUnsupportedGameidError;
    }
    return Common::kNoError;
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

#if PLUGIN_ENABLED_DYNAMIC(ALIS)
    REGISTER_PLUGIN_DYNAMIC(ALIS, PLUGIN_TYPE_ENGINE, AlisMetaEngine);
#else
    REGISTER_PLUGIN_STATIC(ALIS, PLUGIN_TYPE_ENGINE, AlisMetaEngine);
#endif
