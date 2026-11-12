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

#include "wizardry/resources/charset.h"
#include "wizardry/resources/dump.h"
#include "wizardry/resources/messages.h"
#include "wizardry/resources/picture_file.h"
#include "wizardry/resources/scenario_file.h"
#include "wizardry/resources/title_image.h"

namespace Wizardry {

static void dumpGfxSet(const Profile &profile, const GfxSet &gfx, DumpTarget &target,
		Common::WriteStream &summary) {
	if (!gfx.isPresent())
		return;

	// The first three characters of the file name are "200" or "400"
	const Common::String set(gfx.charset, 3);
	summary.writeString(Common::String::format("set %s: screen %dx%d, cell %dx%d\n",
		set.c_str(), gfx.screenW, gfx.screenH, gfx.cellW, gfx.cellH));

	CharSet charset;
	if (charset.open(gfx)) {
		Common::WriteStream *out = target.create(("charset_" + set + ".bin").c_str());
		for (uint i = 0; i < charset.size(); ++i)
			out->write(charset.glyph(i), gfx.cellW * gfx.cellH);
		delete out;
		summary.writeString(Common::String::format("  %s: %u glyphs\n", gfx.charset, charset.size()));
	} else {
		summary.writeString(Common::String::format("  %s: NOT LOADED\n", gfx.charset));
	}

	PictureFile pictures;
	if (!gfx.pictures) {
		summary.writeString("  no picture file\n");
	} else if (pictures.open(profile, gfx)) {
		Common::WriteStream *out = target.create(("pictures_" + set + ".bin").c_str());
		CellImage image;
		for (uint id = 1; id <= pictures.size(); ++id) {
			pictures.load(id, image);
			out->write(&image.pixels[0], image.pixels.size());
		}
		delete out;
		summary.writeString(Common::String::format("  %s: %u pictures of %ux%u cells, %ux%u pixels\n",
			gfx.pictures, pictures.size(), image.cols, image.rows, image.width(), image.height()));
	} else {
		summary.writeString(Common::String::format("  %s: NOT LOADED\n", gfx.pictures));
	}

	CellImage title;
	if (!gfx.title) {
		summary.writeString("  no title file\n");
	} else if (loadTitleImage(profile, gfx, title)) {
		Common::WriteStream *out = target.create(("title_" + set + ".bin").c_str());
		out->write(&title.pixels[0], title.pixels.size());
		delete out;
		summary.writeString(Common::String::format("  %s: %ux%u cells, %ux%u pixels\n",
			gfx.title, title.cols, title.rows, title.width(), title.height()));
	} else {
		summary.writeString(Common::String::format("  %s: NOT LOADED\n", gfx.title));
	}
}

static void dumpMessages(const char *label, const char *name, const char *huffman,
		DumpTarget &target, Common::WriteStream &summary) {
	if (!name)
		return;

	MessageFile messages;
	if (!messages.open(name, huffman)) {
		summary.writeString(Common::String::format("%s: NOT LOADED\n", name));
		return;
	}

	Common::WriteStream *out = target.create(Common::String::format("messages_%s.txt", label).c_str());
	for (uint i = 0; i < messages.size(); ++i) {
		const MessageFile::Entry &e = messages.entry(i);
		Common::String line = Common::String::format("%u\t", e.id);
		for (uint j = 0; j < e.text.size(); ++j)
			line += Common::String::format("%02x", (byte)e.text[j]);
		line += '\n';
		out->writeString(line);
	}
	delete out;

	summary.writeString(Common::String::format("%s: %u messages\n", name, messages.size()));
}

static void dumpScenario(const char *name, DumpTarget &target, Common::WriteStream &summary) {
	ScenarioFile scenario;
	if (!scenario.open(name)) {
		summary.writeString(Common::String::format("%s: no table of contents of the known kind\n", name));
		return;
	}

	Common::WriteStream *text = target.create("scenario.txt");
	Common::WriteStream *records = target.create("scenario.bin");

	for (uint j = 0; j < scenario.title().size(); ++j)
		text->writeString(Common::String::format("%02x", (byte)scenario.title()[j]));
	text->writeString("\n");

	uint total = 0;
	for (uint t = 0; t < kRecordTypeCount; ++t) {
		const ScenarioFile::Type &type = scenario.type((RecordType)t);
		uint written = 0;

		for (uint i = 0; t != kRecordHeader && i < type.count; ++i) {
			const byte *record = scenario.record((RecordType)t, i);
			if (record) {
				records->write(record, type.recordSize);
				++written;
			}
		}

		text->writeString(Common::String::format("%u %u %u %u %u %u %u\n", t, type.perPair,
			type.count, type.third, type.firstBlock, type.recordSize, written));
		total += written;
	}

	delete text;
	delete records;
	summary.writeString(Common::String::format("%s: %u records\n", name, total));
}

void dumpResources(const Profile &profile, DumpTarget &target) {
	Common::WriteStream *summary = target.create("summary.txt");

	dumpGfxSet(profile, profile.low, target, *summary);
	dumpGfxSet(profile, profile.high, target, *summary);
	dumpMessages("ascii", profile.asciiText, profile.huffman, target, *summary);
	dumpMessages("kana", profile.kanaText, profile.huffman, target, *summary);
	dumpMessages("kanji", profile.kanjiText, profile.huffman, target, *summary);
	dumpScenario(profile.scenario, target, *summary);

	delete summary;
}

} // End of namespace Wizardry
