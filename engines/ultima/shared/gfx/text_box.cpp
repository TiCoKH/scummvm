/* ScummVM - Graphic Adventure Engine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "ultima/shared/gfx/text_box.h"

#include "common/textconsole.h"
#include "ultima/shared/gfx/towns_screen.h"

namespace Ultima {
namespace Shared {
namespace Gfx {

TextBox::TextBox(TownsScreen &screen, SjisTextRasterizer &rasterizer) :
		_screen(screen), _rasterizer(rasterizer), _state(), _initialized(false) {
}

bool TextBox::initialize(const TextBoxConfig &config) {
	const int64 right = int64(config.x) + config.width;
	const int64 bottom = int64(config.y) + config.height;
	const int page = _screen.getWritePage();
	const int pageWidth = _screen.getLayerWidth(page);
	const int pageHeight = _screen.getLayerHeight(page);

	if (config.width < 14 || config.height < 19 ||
			config.x < 0 || config.y < 0 || config.requestedColumns < 0 ||
			config.requestedRows < 0 || right > pageWidth || bottom > pageHeight) {
		warning("TextBox::initialize(): Invalid box geometry or text limits");
		_initialized = false;
		return false;
	}

	_state = TextBoxState();
	_state.x = config.x;
	_state.y = config.y;
	_state.width = config.width;
	_state.height = config.height;
	_state.maxColumns = MIN(config.requestedColumns, (config.width - 6) / 8);
	_state.maxRows = MIN(config.requestedRows, (config.height - 4) / 16);
	_state.unknownState = -1;
	_state.fontColor = config.fontColor;
	_state.fontStyle = 0;
	_state.borderColor = config.borderColor;
	_state.backgroundColor = config.backgroundColor;
	_state.textBackgroundMode = config.textBackgroundMode;
	_state.needsRedraw = true;
	_initialized = true;
	return true;
}

void TextBox::drawFrame() {
	if (!_initialized)
		return;

	const int right = _state.x + _state.width - 1;
	const int bottom = _state.y + _state.height - 1;
	_screen.drawColoredRectangle(_state.x + 1, _state.y + 1,
			right - 1, bottom - 1, _state.backgroundColor);
	_screen.setForegroundColor(_state.borderColor);
	_screen.drawLine(_state.x + 2, _state.y, right - 2, _state.y);
	_screen.drawLine(right, _state.y + 2, right, bottom - 2);
	_screen.drawLine(_state.x + 2, bottom, right - 2, bottom);
	_screen.drawLine(_state.x, _state.y + 2, _state.x, bottom - 2);
	// Inset rectangle: border foreground with background fill
	_screen.setForegroundColor(_state.borderColor);
	_screen.setSurfaceFillColor(_state.backgroundColor);
	_screen.drawFilledRectangle(_state.x + 1, _state.y + 1, right - 1, bottom - 1);
	_state.needsRedraw = false;
}

void TextBox::prepare() {
	if (!_initialized)
		return;

	_screen.drawColoredRectangle(_state.x + 2, _state.y + 2,
			_state.x + _state.width - 3, _state.y + _state.height - 3,
			_state.backgroundColor);
	_screen.setForegroundColor(_state.fontColor);
	_screen.setSurfaceFillColor(_state.backgroundColor);
	_screen.setBackgroundColor(_state.textBackgroundMode == 0 ?
			11 : _state.backgroundColor);
}

void TextBox::applyFont() {
	if (!_initialized)
		return;

	_screen.setForegroundColor(_state.fontColor);
	if (_state.textBackgroundMode == -1)
		_screen.setBackgroundColor(_state.backgroundColor);
	_screen.setFontStyle(_state.fontStyle);
}

bool TextBox::isLeadByte(byte ch) {
	return (ch >= 0x81 && ch <= 0x9f) || (ch >= 0xe0 && ch <= 0xfc);
}

bool TextBox::isTrailByte(byte ch) {
	return (ch >= 0x40 && ch <= 0x7e) || (ch >= 0x80 && ch <= 0xfc);
}

void TextBox::renderSegment(const byte *text, uint16 length) {
	if (!length)
		return;

	const int x = _state.x + 4 + _state.cursorColumn * 8;
	const int baselineY = _state.y + 17 + _state.cursorRow * 16;
	_rasterizer.drawSjisBytes(_screen, static_cast<int16>(x),
			static_cast<int16>(baselineY), text, length);
	_screen.invalidateRect(x, baselineY - 15, int(length) * 8, 16);
	_state.cursorColumn += length;
}

void TextBox::printSjis(const char *text) {
	if (!_initialized || !text)
		return;

	const byte *cursor = reinterpret_cast<const byte *>(text);
	while (*cursor) {
		const int remaining = _state.maxColumns - _state.cursorColumn + 1;
		if (remaining <= 0) {
			advanceLine();
			continue;
		}

		const byte *segment = cursor;
		int used = 0;
		while (used < remaining && cursor[used] && cursor[used] != '\n') {
			const bool isPair = isLeadByte(cursor[used]) && cursor[used + 1] &&
					isTrailByte(cursor[used + 1]);
			if (isPair) {
				if (remaining - used < 2)
					break;
				used += 2;
			} else {
				++used;
			}
		}

		if (used) {
			renderSegment(segment, static_cast<uint16>(used));
			cursor += used;
		}

		if (used == remaining) {
			advanceLine();
		} else if (*cursor == '\n') {
			++cursor;
			advanceLine();
		} else if (!*cursor) {
			return;
		} else if (!used) {
			const bool isPair = isLeadByte(cursor[0]) && cursor[1] &&
					isTrailByte(cursor[1]);
			if (isPair && _state.maxColumns == 0) {
				warning("TextBox::printSjis(): Kanji pair does not fit in a one-byte text row");
				cursor += 2;
			}
			advanceLine();
		} else {
			advanceLine();
		}
	}
}

void TextBox::scrollUpOneLine() {
	if (!_initialized)
		return;

	_screen.setBackgroundColor(_state.backgroundColor);
	_screen.scrollRegion(_state.x + 2, _state.y + 2,
			_state.x + _state.width - 3, _state.y + _state.height - 3,
			0, -16, true);
	if (_state.textBackgroundMode == 0)
		_screen.setBackgroundColor(11);
}

void TextBox::advanceLine() {
	if (!_initialized)
		return;

	_state.cursorColumn = 0;
	if (_state.cursorRow == _state.maxRows)
		scrollUpOneLine();
	else
		++_state.cursorRow;
}

void TextBox::setCursor(int32 column, int32 row) {
	if (!_initialized)
		return;
	if (column < 0 || column > _state.maxColumns ||
			row < 0 || row > _state.maxRows) {
		warning("TextBox::setCursor(): Cursor is outside the configured text area");
		return;
	}

	_state.cursorColumn = column;
	_state.cursorRow = row;
}

void TextBox::hide(uint16 eraseColor) {
	if (!_initialized || _state.needsRedraw)
		return;

	_screen.drawColoredRectangle(_state.x, _state.y,
			_state.x + _state.width - 1, _state.y + _state.height - 1, eraseColor);
	_screen.setForegroundColor(15);
	_screen.setSurfaceFillColor(15);
	_state.textBackgroundMode = -1;
	_state.needsRedraw = true;
}

void TextBox::fillArea(int32 xCell, int32 yCell, int32 xEnd, int32 yEnd) {
	if (!_initialized)
		return;

	const int x1 = _state.x + 4 + xCell * 8;
	const int y1 = _state.y + 2 + yCell * 16;
	const int x2 = _state.x + 11 + xEnd * 8;
	const int y2 = _state.y + 17 + yEnd * 16;
	_screen.setForegroundColor(_state.backgroundColor);
	_screen.setSurfaceFillColor(_state.backgroundColor);
	_screen.drawFilledRectangle(x1, y1, x2, y2);
	_screen.setForegroundColor(_state.fontColor);
}

void TextBox::highlightArea(int32 xCell, int32 yCell, int32 xEnd, int32 yEnd) {
	if (!_initialized)
		return;

	const int x1 = _state.x + 4 + xCell * 8;
	const int y1 = _state.y + 2 + yCell * 16;
	const int x2 = _state.x + 11 + xEnd * 8;
	const int y2 = _state.y + 17 + yEnd * 16;
	const uint16 savedWriteMode = _screen.getWriteMode();
	const uint16 savedPaintMode = _screen.getPaintMode();
	// SCR_clearBoxArea: writeMode=4 (XOR), paintMode=0x422 (fill+border+area).
	// The 0x400 EGB area-mode bit has no software equivalent; the XOR effect
	// is carried entirely by writeMode=4 acting on the fill operation.
	_screen.setWriteMode(4);
	_screen.setPaintMode(TownsScreen::kPaintFillSolid | TownsScreen::kPaintBorderSolid);
	_screen.setForegroundColor(15);
	_screen.setSurfaceFillColor(15);
	_screen.drawFilledRectangle(x1, y1, x2, y2);
	_screen.setForegroundColor(_state.fontColor);
	_screen.setSurfaceFillColor(_state.backgroundColor);
	_screen.setWriteMode(savedWriteMode);
	_screen.setPaintMode(savedPaintMode);
}

// ASCII-to-SJIS mapping table from TEXT_drawMappedExtendedGlyph / DARKNESS reference.
static uint16 mappedAsciiToSjis(char ch) {
	switch (ch) {
	case ' ':  return 0x8140;
	case '!':  return 0xeb74;
	case '/':  return 0x815e;
	case '0':  return 0x824f; // -0x7f15 as unsigned 16-bit
	case '1':  return 0xeb76;
	case '2':  return 0xeb77;
	case '3':  return 0xeb78;
	case '4':  return 0xeb79;
	case '5':  return 0xeb7a;
	case '6':  return 0xeb7b;
	case '7':  return 0xeb7c;
	case '8':  return 0xeb7d;
	case '9':  return 0xeb7e;
	case ':':  return 0x8146;
	case '?':  return 0xeb75;
	case 'A':  return 0xeb40;
	case 'B':  return 0xeb41;
	case 'C':  return 0xeb42;
	case 'D':  return 0xeb43;
	case 'E':  return 0xeb44;
	case 'F':  return 0xeb45;
	case 'G':  return 0xeb46;
	case 'H':  return 0xeb47;
	case 'I':  return 0xeb48;
	case 'J':  return 0xeb49;
	case 'K':  return 0xeb4a;
	case 'L':  return 0xeb4b;
	case 'M':  return 0xeb4c;
	case 'N':  return 0xeb4d;
	case 'O':  return 0xeb4e;
	case 'P':  return 0xeb4f;
	case 'Q':  return 0xeb50;
	case 'R':  return 0xeb51;
	case 'S':  return 0xeb52;
	case 'T':  return 0xeb53;
	case 'U':  return 0xeb54;
	case 'V':  return 0xeb55;
	case 'W':  return 0xeb56;
	case 'X':  return 0xeb57;
	case 'Y':  return 0xeb58;
	case 'Z':  return 0xeb59;
	default:   return 0;
	}
}

void TextBox::printMappedAscii(const char *text) {
	if (!_initialized || !text)
		return;

	for (; *text; ++text) {
		if (_state.cursorColumn + 1 > _state.maxColumns) {
			_state.cursorColumn = 0;
			if (_state.cursorRow == _state.maxRows)
				scrollUpOneLine();
			else
				++_state.cursorRow;
		}
		const uint16 sjis = mappedAsciiToSjis(*text);
		if (sjis) {
			const int x = _state.x + 4 + _state.cursorColumn * 8;
			const int baselineY = _state.y + 17 + _state.cursorRow * 16;
			const byte pair[2] = { byte(sjis >> 8), byte(sjis & 0xff) };
			_rasterizer.drawSjisBytes(_screen, static_cast<int16>(x),
					static_cast<int16>(baselineY), pair, 2);
			_screen.invalidateRect(x, baselineY - 15, 16, 16);
		}
		_state.cursorColumn += 2;
	}
}

void TextBox::putCharData(const byte *glyphData) {
	if (!_initialized || !glyphData)
		return;

	// GFX_putCharDataIntoBox positions via EGB_asciiString then draws a 16x16
	// raw glyph via EGB_anyChar. We pass the data through the rasterizer as a
	// 2-byte SJIS sequence; callers supplying actual raw pixel data should use
	// getWriteSurface() directly and call invalidateRect themselves.
	const int x = _state.x + 4 + _state.cursorColumn * 8;
	const int baselineY = _state.y + 17 + _state.cursorRow * 16;
	_rasterizer.drawSjisBytes(_screen, static_cast<int16>(x),
			static_cast<int16>(baselineY), glyphData, 2);
	_screen.invalidateRect(x, baselineY - 15, 16, 16);
}

} // namespace Gfx
} // namespace Shared
} // namespace Ultima
