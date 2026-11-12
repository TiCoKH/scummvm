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

#include "common/file.h"
#include "wizardry/console.h"
#include "wizardry/data/globals.h"
#include "wizardry/resources/dump.h"
#include "wizardry/wizardry.h"

namespace Wizardry {

Console::Console() : GUI::Debugger() {
	registerCmd("test",   WRAP_METHOD(Console, Cmd_test));
	registerCmd("profile", WRAP_METHOD(Console, Cmd_profile));
	registerCmd("dump",   WRAP_METHOD(Console, Cmd_dump));
	registerCmd("roster", WRAP_METHOD(Console, Cmd_roster));
}

Console::~Console() {
}

bool Console::Cmd_test(int argc, const char **argv) {
	debugPrintf("Test\n");
	return true;
}

bool Console::Cmd_profile(int argc, const char **argv) {
	const Profile *p = g_engine->getProfile();
	if (!p) {
		debugPrintf("This release has no profile\n");
		return true;
	}

	const GfxSet &gfx = g_engine->getGfxSet();
	const char *const files[] = {
		p->codeFile, p->interpreter, p->scenario, p->asciiText, p->kanaText,
		p->kanjiText, p->huffman, gfx.charset, gfx.pictures, gfx.title
	};
	const char *const labels[] = {
		"code", "interpreter", "scenario", "ascii text", "kana text",
		"kanji text", "huffman table", "character set", "pictures", "title"
	};

	debugPrintf("%s, %s\n", g_engine->getGameId().c_str(),
		Common::getPlatformDescription(p->platform));
	debugPrintf("screen %dx%d, cells %dx%d, pictures %dx%d cells\n",
		gfx.screenW, gfx.screenH, gfx.cellW, gfx.cellH, p->pictureCols, p->pictureRows);

	for (uint i = 0; i < ARRAYSIZE(files); ++i) {
		if (!files[i])
			debugPrintf("%-14s -\n", labels[i]);
		else
			debugPrintf("%-14s %-14s %s\n", labels[i], files[i],
				Common::File::exists(files[i]) ? "found" : "MISSING");
	}

	return true;
}

namespace {

class DumpFolder : public DumpTarget {
public:
	Common::WriteStream *create(const char *name) override {
		Common::DumpFile *f = new Common::DumpFile();
		if (!f->open(Common::Path("dumps").appendComponent(name), true))
			error("Could not create dumps/%s", name);
		return f;
	}
};

} // End of anonymous namespace

bool Console::Cmd_roster(int argc, const char **argv) {
	Party &party = _G(party);

	if (argc == 2 && !strcmp(argv[1], "sample")) {
		// A new scenario file has no characters and there is no training
		// ground yet, so make some up
		static const struct {
			const char *name;
			int16 classType, alignment, hp, armorClass;
		} SAMPLES[] = {
			{ "HAWKWIND", kClassFighter, kAlignGood, 12, 4 },
			{ "SEZMAR", kClassSamurai, kAlignGood, 14, 5 },
			{ "MORADIN", kClassThief, kAlignNeutral, 6, 8 },
			{ "PRESTO", kClassMage, kAlignNeutral, 4, 10 },
			{ "TUCK", kClassPriest, kAlignGood, 8, 6 },
			{ "SARAH", kClassBishop, kAlignEvil, 5, 9 }
		};

		uint next = 0;
		for (uint i = 0; i < party.rosterSize() && next < ARRAYSIZE(SAMPLES); ++i) {
			Character &c = party.rosterEntry(i);
			if (!c.isFree())
				continue;

			c.name = SAMPLES[next].name;
			c.classType = SAMPLES[next].classType;
			c.alignment = SAMPLES[next].alignment;
			c.hp = c.hpMax = SAMPLES[next].hp;
			c.armorClass = SAMPLES[next].armorClass;
			c.status = kStatusOk;
			c.level = 1;
			c.gold = 100 + 37 * next;
			++next;
		}

		debugPrintf("%u sample characters added to the roster\n", next);
		return true;
	}

	for (uint i = 0; i < party.rosterSize(); ++i) {
		const Character &c = party.rosterEntry(i);
		if (!c.isFree())
			debugPrintf("%2u %-15s class %d alignment %d level %d hp %d/%d gold %d%s\n", i,
				c.name.c_str(), c.classType, c.alignment, c.level, c.hp, c.hpMax, (int)c.gold,
				c.inMaze ? " (in the party)" : "");
	}
	debugPrintf("Use \"roster sample\" to add sample characters\n");
	return true;
}

bool Console::Cmd_dump(int argc, const char **argv) {
	const Profile *p = g_engine->getProfile();
	if (!p) {
		debugPrintf("This release has no profile\n");
		return true;
	}

	DumpFolder folder;
	dumpResources(*p, folder);
	debugPrintf("Resources written to the folder dumps; see summary.txt there\n");
	return true;
}

} // End of namespace Wizardry
