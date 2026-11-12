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

#include "wizardry/profile.h"

namespace Wizardry {

#define NO_SET { nullptr, nullptr, nullptr, 0, 0, kPixelsNone, 0, 0 }

// IBM-PC: CGA 320x200 in four colours, or CGA 640x200 in two
#define IBM_LOW(PICTURES, TITLE) \
	{ "200.CHARSET", PICTURES, TITLE, 8, 8, kPixelsCGA2bpp, 320, 200 }
#define IBM_HIGH(PICTURES, TITLE) \
	{ "400.CHARSET", PICTURES, TITLE, 16, 8, kPixelsMono, 640, 200 }

// PC-98: 640x200 or 640x400, eight colours
#define PC98_LOW \
	{ "200.CHARSET", "200.MONSTERS", "200.TITLE", 16, 8, kPixelsPlanarBRG, 640, 200 }
#define PC98_HIGH(TITLE) \
	{ "400.CHARSET", "400.MONSTERS", TITLE, 16, 16, kPixelsPlanarBRG, 640, 400 }

static const Profile PROFILES[] = {
	// IBM-PC. Wizardry II and III have no picture file for the two-colour
	// mode, and Wizardry IV has neither pictures nor a title for it.
	{ kWizardry1, Common::kPlatformDOS, "SYSTEM.PASCAL", "SYSTEM.INTERP", "SCENARIO.DATA",
	  "ASCII.KRN", nullptr, nullptr, nullptr,
	  IBM_LOW("200.MONSTERS", "200.TITLE"), IBM_HIGH("400.MONSTERS", "400.TITLE"),
	  6, 5, kPicturesPlain },
	{ kWizardry2, Common::kPlatformDOS, "SYSTEM.PASCAL", "SYSTEM.INTERP", "SCENARIO.DATA",
	  "ASCII.KRN", nullptr, nullptr, nullptr,
	  IBM_LOW("200.MONSTERS", "200.TITLE"), IBM_HIGH(nullptr, "400.TITLE"),
	  6, 5, kPicturesPlain },
	{ kWizardry3, Common::kPlatformDOS, "SYSTEM.PASCAL", "SYSTEM.INTERP", "SCENARIO.DATA",
	  "ASCII.KRN", nullptr, nullptr, nullptr,
	  IBM_LOW("200.MONSTERS", "200.TITLE"), IBM_HIGH(nullptr, "400.TITLE"),
	  6, 5, kPicturesPlain },
	{ kWizardry4, Common::kPlatformDOS, "SYSTEM.PASCAL", "SYSTEM.INTERP", "SCENARIO.DATA",
	  "ASCII.KRN", nullptr, nullptr, "ASCII.HUFF",
	  IBM_LOW("200.MONSTERS", nullptr), IBM_HIGH(nullptr, nullptr),
	  6, 5, kPicturesPlain },
	{ kWizardry5, Common::kPlatformDOS, "SYSTEM.PASCAL", "SYSTEM.INTERP", "DRAGON.DATA",
	  "ASCII.KRN", nullptr, nullptr, "ASCII.HUFF",
	  IBM_LOW("200.MONSTERS", nullptr), NO_SET,
	  12, 14, kPicturesPlain },

	// NEC PC-98. Wizardry IV and V have only the "400" set and no kana text.
	// Wizardry V keeps a title picture under the name 200.TITLE; how it is
	// shown has not been looked at yet.
	{ kWizardry1, Common::kPlatformPC98, "PC98.PASCAL", "SYSTEM.INTERP", "SCENARIO.DATA",
	  "ASCII.KRN", "KANA.KRN", "KANJI.KRN", nullptr,
	  PC98_LOW, PC98_HIGH("400.TITLE"),
	  6, 5, kPicturesPlain },
	{ kWizardry2, Common::kPlatformPC98, "SYSTEM.PASCAL", "SYSTEM.INTERP", "SCENARIO.DATA",
	  "ASCII.KRN", "KANA.KRN", "KANJI.KRN", nullptr,
	  PC98_LOW, PC98_HIGH("400.TITLE"),
	  6, 5, kPicturesPlain },
	{ kWizardry3, Common::kPlatformPC98, "SYSTEM.PASCAL", "SYSTEM.INTERP", "SCENARIO.DATA",
	  "ASCII.KRN", "KANA.KRN", "KANJI.KRN", nullptr,
	  PC98_LOW, PC98_HIGH("400.TITLE"),
	  6, 5, kPicturesPlain },
	{ kWizardry4, Common::kPlatformPC98, "SYSTEM.PASCAL", "SYSTEM.INTERP", "SCENARIO.DATA",
	  "ASCII.KRN", nullptr, "KANJI.KRN", "WERDNA.TABS",
	  NO_SET, PC98_HIGH(nullptr),
	  6, 5, kPicturesPlain },
	{ kWizardry5, Common::kPlatformPC98, "SYSTEM.PASCAL", "SYSTEM.INTERP", "DRAGON.DATA",
	  "ASCII.KRN", nullptr, "KANJI.KRN", "HEART.TABS",
	  NO_SET, PC98_HIGH("200.TITLE"),
	  12, 14, kPicturesPackedIndexed }
};

const Profile *findProfile(GameType game, Common::Platform platform) {
	for (const Profile &p : PROFILES) {
		if (p.game == game && p.platform == platform)
			return &p;
	}

	return nullptr;
}

} // End of namespace Wizardry
