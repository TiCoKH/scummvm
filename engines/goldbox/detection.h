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

#ifndef GOLDBOX_DETECTION_H
#define GOLDBOX_DETECTION_H

#include "engines/advancedDetector.h"

namespace Goldbox {

enum GameType {
	PASCAL_ENGINE, C_ENGINE
};

enum GoldboxDebugChannels {
	kDebugGraphics = 1,
	kDebugPath,
	kDebugScan,
	kDebugFilePath,
	kDebugScript,
};

struct GoldboxGameDescription {
	AD_GAME_DESCRIPTION_HELPERS(desc);
	ADGameDescription desc;

	int gameType;
};

extern const PlainGameDescriptor goldboxGames[];

extern const GoldboxGameDescription gameDescriptions[];

#define GAMEOPTION_ORIGINAL_SAVELOAD GUIO_GAMEOPTIONS1

} // End of namespace Goldbox

class GoldboxMetaEngineDetection : public AdvancedMetaEngineDetection<Goldbox::GoldboxGameDescription> {
	static const DebugChannelDef debugFlagList[];

public:
	GoldboxMetaEngineDetection();
	~GoldboxMetaEngineDetection() override {}

	const char *getName() const override {
		return "goldbox";
	}

	const char *getEngineName() const override {
		return "Goldbox";
	}

	const char *getOriginalCopyright() const override {
		return "Strategic Simulations, Inc. (C)";
	}

	const DebugChannelDef *getDebugChannels() const override {
		return debugFlagList;
	}
};

#endif // GOLDBOX_DETECTION_H
