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

#include "base/plugins.h"

#include "common/config-manager.h"

#include "alis/detection.h"
#include "alis/detection_tables.h"
#include "alis/alis.h"

namespace Alis {

static const PlainGameDescriptor ALIS_GAMES[] = {
#ifdef ENABLE_ALIS
	{ "manhattan", "Manhattan Dealers / Operation: Cleanstreets" }, 
	{ "madshow", "Mad Show" },
	{ "windsurf", "Windsurf Willy" },
	{ "maya", "Maya / Le Fetiche Maya" },
	{ "targhan", "Targhan" },
	{ "colorado", "Colorado" },
	{ "arborea", "Crystals of Arborea" },
	{ "starblade", "StarBlade" },
	{ "boston", "Boston Bomb Club" },
	{ "metal", "Metal Mutant" },
	{ "bunny", "Bunny Bricks" },
	{ "storm", "Storm Master" },
	{ "artic", "Transarctica / Artic Baron" },
	{ "ishar1", "Ishar: Legend of the Fortress" },
	{ "ishar2", "Ishar 2: Messengers of Doom" },
	{ "ishar3", "Ishar 3: The Seven Gates of Infinity" },
	{ "robinson", "Robinson's Requiem" },
	{ "deus", "Deus" },
#endif
	{ 0, 0 }
};

} // End of namespace Alis


AlisMetaEngineDetection::AlisMetaEngineDetection() : AdvancedMetaEngineDetection(Alis::GAME_DESCRIPTIONS,
	        sizeof(Alis::AlisGameDescription), Alis::ALIS_GAMES) {
	static const char *const DIRECTORY_GLOBS[2] = { "usecode", 0 };
	_maxScanDepth = 2;
	_directoryGlobs = DIRECTORY_GLOBS;
}

REGISTER_PLUGIN_STATIC(ALIS_DETECTION, PLUGIN_TYPE_ENGINE_DETECTION, AlisMetaEngineDetection);
