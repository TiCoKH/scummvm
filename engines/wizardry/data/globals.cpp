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

#include "wizardry/data/globals.h"
#include "wizardry/libs/file.h"
#include "wizardry/libs/memory.h"
#include "common/config-manager.h"
#include "common/system.h"
#include "graphics/paletteman.h"
#include "wizardry/gfx/palette.h"
#include "wizardry/wizardry.h"
#include "wizardry/libs/wizardry_archive.h"

namespace Wizardry {

Globals *g_globals;

Globals::Globals() {
	g_globals = this;
}

Globals::~Globals() {
	g_globals = nullptr;
	delete _normalFont;
	delete _gfxFont;
	delete _wideFont;
}

void Globals::setup() {
	const bool isApple = g_engine->getFamily() == kFamilyApple;

	// Setup file access. The Apple II release needs an archive that gives
	// names to parts of its scenario file. All other releases are plain
	// files in the game folder, named in the engine's profile.
	if (isApple) {
		WizardryArchive *arc;
		if (g_engine->isDiskImage())
			arc = new Compressed::WizardryV1Archive();
		else
			arc = new Uncompressed::Wizardry1V1Archive();
		SearchMan.add("dsk", arc, 1);

		Gfx::setupPalette();

		File f1("font");
		_normalFont = new WizardryFontV1(f1);
		File f2("gfx_font");
		_gfxFont = new WizardryFontV1(f2);
	} else {
		const Profile &profile = *g_engine->getProfile();
		const GfxSet &gfx = g_engine->getGfxSet();

		if (!_renderer.open(gfx))
			error("Could not load the character set %s", gfx.charset);
		applyPalette();

		// Not every graphics set has pictures
		_pictures.open(profile, gfx);

		loadMessages();

		// The releases that can be played cannot do without their scenario.
		// (Wizardry V keeps its data in a file this class cannot read yet.)
		const bool hasScenario = _scenario.open(profile.scenario);
		if (g_engine->hasTown() && !hasScenario)
			error("Could not read %s: it is missing, or not a scenario file of the kind expected", profile.scenario);
		if (hasScenario && g_engine->hasTown()) {
			if (!_party.loadRoster(_scenario) || !_items.load(_scenario) || !_monsters.load(_scenario) ||
					!_rewards.load(_scenario))
				error("%s does not have the expected character, item and monster records", profile.scenario);
		}
	}

#if 0
	_SCNTOCBL = FINDFILE(DRIVE1, "scenario.data");
	assert(_SCNTOCBL >= 0);
	UNITREAD(DRIVE1, _IOCACHE, sizeof(_IOCACHE), _SCNTOCBL);
	_SCNTOC.load(_IOCACHE);
#endif
}

void Globals::loadMessages() {
	const Profile &profile = *g_engine->getProfile();

	// The kana text where the release has it and the game option asks for
	// it. Its two-byte characters are drawn from the character set, as the
	// PC-98 driver does; the English text of the PC-98 has some too.
	const int language = ConfMan.hasKey("text_language") ? ConfMan.getInt("text_language") : kTextEnglish;
	bool kanji = profile.kanjiText && language == kTextKanji;
	const bool kana = profile.kanaText && (language == kTextKana || language == kTextKanji);

	// Kanji needs a font of them; without one the kana text is shown
	_renderer.setWideFont(nullptr);
	if (kanji && !_wideFont) {
#ifdef USE_SJIS
		_wideFont = createPC98WideFont();
#endif
		if (!_wideFont) {
			warning("No kanji font (FONT.ROM, FONT.BMP or SJIS.FNT): showing the kana text");
			kanji = false;
		}
	}
	if (kanji)
		_renderer.setWideFont(_wideFont);

	const char *name = kanji ? profile.kanjiText : (kana ? profile.kanaText : profile.asciiText);
	if (!_messages.open(name, profile.huffman))
		error("Could not load the messages from %s", name);
	_kanaText = kanji || kana;

	// The title is shown before the original asks for a language, so its
	// lines always come from the ASCII text
	MessageFile ascii;
	const MessageFile *source = &_messages;
	if (kana && ascii.open(profile.asciiText, profile.huffman))
		source = &ascii;
	for (int i = 0; i < 5; ++i)
		_titleLines[i] = source->get(2018 + i);   // TownScreens::kTitleFirst

	_tiles.setTwoByteText(hasJapaneseRules(&profile));
}

void Globals::applyPalette() {
	if (g_engine->getPlatform() == Common::kPlatformDOS)
		_renderer.setCgaPalette(ConfMan.getInt("cga_palette"));

	g_system->getPaletteManager()->setPalette(_renderer.palette(), 0, _renderer.paletteSize());
}

void Globals::present() {
	Graphics::Screen *screen = g_events->getScreen();
	Common::Rect area;

	if (screen && _renderer.render(_tiles, (byte *)screen->getPixels(), screen->pitch, area))
		screen->addDirtyRect(area);
}

} // namespace Wizardry
