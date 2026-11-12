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
		g_system->getPaletteManager()->setPalette(_renderer.palette(), 0, _renderer.paletteSize());

		// Not every graphics set has pictures
		_pictures.open(profile, gfx);

		// TODO: let the player choose kana or kanji text where there is any
		if (!_messages.open(profile.asciiText, profile.huffman))
			error("Could not load the messages from %s", profile.asciiText);

		// Wizardry V keeps its data in a file this class cannot read yet
		if (_scenario.open(profile.scenario) && g_engine->hasTown()) {
			if (!_party.loadRoster(_scenario))
				error("%s does not have the expected character records", profile.scenario);
		}
	}

#if 0
	_SCNTOCBL = FINDFILE(DRIVE1, "scenario.data");
	assert(_SCNTOCBL >= 0);
	UNITREAD(DRIVE1, _IOCACHE, sizeof(_IOCACHE), _SCNTOCBL);
	_SCNTOC.load(_IOCACHE);
#endif
}

void Globals::present() {
	Graphics::Screen *screen = g_events->getScreen();
	Common::Rect area;

	if (screen && _renderer.render(_cells, (byte *)screen->getPixels(), screen->pitch, area))
		screen->addDirtyRect(area);
}

} // namespace Wizardry
