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

#include "wizardry/views/gallery.h"
#include "wizardry/game/spells.h"
#include "wizardry/ui/data_pages.h"
#include "wizardry/wizardry.h"

namespace Wizardry {

void Gallery::drawTiles(TextWindow &window) {
	_G(tiles).clear();
	window.clear();

	switch (_page) {
	case kPageCharset:
		drawCharset(window);
		break;
	case kPageMessages:
		drawMessages(window);
		break;
	case kPageItems:
		DataPages::drawItem(window, _G(messages), _G(items), _item);
		break;
	case kPageMonsters:
		DataPages::drawMonster(window, _G(messages), _G(monsters), _monster);
		break;
	case kPageSpells:
		DataPages::drawSpells(window, _G(messages), _firstSpell);
		break;
	case kPageMaze:
		drawMaze(window);
		break;
	default:
		drawPictures(window);
		break;
	}

	window.printCentered(window.height() - 1, _page == kPageMaze ?
		"ARROWS MOVE  PGUP/PGDN LEVEL  TAB  ESC" : "ARROWS:CHANGE  TAB:PAGE  ESC:QUIT");
}

const Common::Array<MazeLevel> &Gallery::levels() {
	// The levels of a game being played, else as the scenario has them
	if (!_G(mazeLevels).empty())
		return _G(mazeLevels);

	if (_levels.empty() && g_engine->hasTown()) {
		for (int n = 1; n <= 64; ++n) {
			MazeLevel one;
			if (!one.load(_G(scenario), n))
				break;
			_levels.push_back(one);
		}
	}
	return _levels;
}

void Gallery::drawMaze(TextWindow &window) {
	const Common::Array<MazeLevel> &all = levels();
	if (all.empty()) {
		window.printCentered(1, "NO MAZE IN THIS SCENARIO");
		return;
	}
	_mazeLevel = CLIP<int>(_mazeLevel, 1, all.size());
	DataPages::drawMaze(window, all[_mazeLevel - 1], _mazeLevel, all.size(), _mazeX, _mazeY);
}

uint Gallery::step(uint value, int by, uint count) {
	if (!count)
		return 0;
	return (uint)(((int)value + by % (int)count + (int)count) % (int)count);
}

void Gallery::drawPictures(TextWindow &window) {
	const PictureFile &pictures = _G(pictures);
	if (!pictures.size()) {
		window.printCentered(1, "THIS GRAPHICS SET HAS NO PICTURES");
		return;
	}

	window.printCentered(0, Common::String::format("PICTURE %u OF %u", _picture, pictures.size()));

	TileImage image;
	if (pictures.load(_picture, image))
		window.showImage(image, (window.width() - image.cols) / 2, 2);
}

void Gallery::drawCharset(TextWindow &window) {
	const uint count = _G(renderer).charset().size();
	window.printCentered(0, Common::String::format("CHARACTER SET: %u GLYPHS", count));

	// 32 to a row. Glyphs 0 and 32 always show blank.
	for (uint glyph = 0; glyph < count; ++glyph) {
		window.MVCURSOR(3 + glyph % 32, 2 + glyph / 32);
		window.PRINTCHR(glyph);
	}
}

void Gallery::drawMessages(TextWindow &window) {
	const MessageFile &messages = _G(messages);
	window.printCentered(0, Common::String::format("MESSAGES: %u", messages.size()));

	for (int line = 0; line < window.height() - 4; ++line) {
		const uint index = _firstMessage + line;
		if (index >= messages.size())
			break;

		const MessageFile::Entry &e = messages.entry(index);
		window.PRINTSTR(0, 2 + line, Common::String::format("%5u ", e.id));
		window.PRINTSTR(e.text);
	}
}

bool Gallery::msgKeypress(const KeypressMessage &msg) {
	const uint pictures = _G(pictures).size();
	const uint messages = _G(messages).size();
	const uint lines = getTileBounds().height() - 6;

	switch (msg.keycode) {
	case Common::KEYCODE_ESCAPE:
		close();
		return true;

	case Common::KEYCODE_TAB:
		_page = (_page + 1) % kPageCount;
		break;

	case Common::KEYCODE_PAGEUP:
	case Common::KEYCODE_PAGEDOWN:
		if (_page != kPageMaze || levels().empty())
			return true;
		_mazeLevel = 1 + step(_mazeLevel - 1, msg.keycode == Common::KEYCODE_PAGEUP ? -1 : 1, levels().size());
		break;

	case Common::KEYCODE_LEFT:
	case Common::KEYCODE_UP:
	case Common::KEYCODE_RIGHT:
	case Common::KEYCODE_DOWN:
		if (_page >= kPageItems) {
			const bool back = msg.keycode == Common::KEYCODE_LEFT || msg.keycode == Common::KEYCODE_UP;
			const bool vertical = msg.keycode == Common::KEYCODE_UP || msg.keycode == Common::KEYCODE_DOWN;
			const int by = (back ? -1 : 1) * (vertical ? 10 : 1);

			switch (_page) {
			case kPageItems:
				_item = step(_item, by, _G(items).size());
				break;
			case kPageMonsters:
				_monster = step(_monster, by, _G(monsters).size());
				break;
			case kPageSpells: {
				const int perPage = (getTileBounds().height() - 2 - 3) * 2;
				_firstSpell = back ? MAX(1, _firstSpell - perPage) :
					(_firstSpell + perPage <= kSpellCount ? _firstSpell + perPage : _firstSpell);
				break;
			}
			case kPageMaze:
				// North is up on the map
				if (vertical)
					_mazeY = step(_mazeY, back ? 1 : -1, MazeLevel::kSize);
				else
					_mazeX = step(_mazeX, back ? -1 : 1, MazeLevel::kSize);
				break;
			default:
				break;
			}
			break;
		}

		if (msg.keycode == Common::KEYCODE_RIGHT || msg.keycode == Common::KEYCODE_DOWN) {
			if (_page == kPagePictures && pictures)
				_picture = (_picture < pictures) ? _picture + 1 : 1;
			else if (_page == kPageMessages && _firstMessage + lines < messages)
				_firstMessage += lines;
			break;
		}
		if (_page == kPagePictures && pictures)
			_picture = (_picture > 1) ? _picture - 1 : pictures;
		else if (_page == kPageMessages)
			_firstMessage = (_firstMessage >= lines) ? _firstMessage - lines : 0;
		break;

	default:
		return true;
	}

	redraw();
	return true;
}

} // End of namespace Wizardry
