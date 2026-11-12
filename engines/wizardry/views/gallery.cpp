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
#include "wizardry/wizardry.h"

namespace Wizardry {

void Gallery::drawCells(TextWindow &window) {
	_G(cells).clear();
	window.clear();

	switch (_page) {
	case kPageCharset:
		drawCharset(window);
		break;
	case kPageMessages:
		drawMessages(window);
		break;
	default:
		drawPictures(window);
		break;
	}

	window.printCentered(window.height() - 1, "ARROWS:CHANGE  TAB:PAGE  ESC:QUIT");
}

void Gallery::drawPictures(TextWindow &window) {
	const PictureFile &pictures = _G(pictures);
	if (!pictures.size()) {
		window.printCentered(1, "THIS GRAPHICS SET HAS NO PICTURES");
		return;
	}

	window.printCentered(0, Common::String::format("PICTURE %u OF %u", _picture, pictures.size()));

	CellImage image;
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
	const uint lines = getCellBounds().height() - 6;

	switch (msg.keycode) {
	case Common::KEYCODE_ESCAPE:
		close();
		return true;

	case Common::KEYCODE_TAB:
		_page = (_page + 1) % kPageCount;
		break;

	case Common::KEYCODE_LEFT:
	case Common::KEYCODE_UP:
		if (_page == kPagePictures && pictures)
			_picture = (_picture > 1) ? _picture - 1 : pictures;
		else if (_page == kPageMessages)
			_firstMessage = (_firstMessage >= lines) ? _firstMessage - lines : 0;
		break;

	case Common::KEYCODE_RIGHT:
	case Common::KEYCODE_DOWN:
		if (_page == kPagePictures && pictures)
			_picture = (_picture < pictures) ? _picture + 1 : 1;
		else if (_page == kPageMessages && _firstMessage + lines < messages)
			_firstMessage += lines;
		break;

	default:
		return true;
	}

	redraw();
	return true;
}

} // End of namespace Wizardry
