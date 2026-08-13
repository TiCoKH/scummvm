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
 */

#include "goldbox/gfx/game_text.h"

#include "goldbox/data/player_character.h"
#include "goldbox/engine.h"
#include "goldbox/gfx/surface.h"

namespace Goldbox {

namespace {

static const int kTextColor = 10;
static const int kEnemyNameColor = 0x12;
static const int kDisabledNameColor = 0x18;
static const int kPartyNameColor = 0x1C;

} // namespace

GameText::GameText(Engine *engine) : _engine(engine), _messageBoxDrawn(false) {
}

bool GameText::isWordBreak(char c) {
	return c == ' ' || c == '-' || c == '.' || c == ',' || c == '?'
		|| c == '!' || c == ':' || c == ';';
}

uint GameText::wordEnd(const Common::String &text, uint start, uint limit) {
	uint end = start;
	while (end < limit && !isWordBreak(text[end]))
		++end;
	if (end < limit)
		++end;
	return end;
}

void GameText::drawCharacterName(Data::PlayerCharacter *character,
		int x, int y) {
	if (!character)
		return;

	int color = kPartyNameColor;
	if (!character->enabled)
		color = kDisabledNameColor;
	else if (character->hostile)
		color = kEnemyNameColor;

	Surface surface(*_engine->getScreen(), Common::Rect(0, 0,
			_engine->getScreen()->w, _engine->getScreen()->h));
	surface.writeStringC(x, y, color, character->name);
}

void GameText::printBlock(int startX, int startY, int endX, int endY,
		int color, const Common::String &text) {
	Surface surface(*_engine->getScreen(), Common::Rect(0, 0,
			_engine->getScreen()->w, _engine->getScreen()->h));
	int cursorX = startX;
	int cursorY = startY;
	uint pos = 0;

	while (pos < text.size() && cursorY <= endY) {
		const uint wordStart = pos;
		pos = wordEnd(text, wordStart, text.size());

		const uint wordLength = pos - wordStart;
		if (cursorX != startX && cursorX + wordLength > endX + 1) {
			++cursorY;
			cursorX = startX;
			if (cursorY > endY)
				break;
		}

		for (uint i = wordStart; i < pos; ++i) {
			if (text[i] == ' ' && cursorX > endX)
				continue;
			surface.writeCharC(cursorX++, cursorY, color, text[i]);
		}
	}
}

void GameText::showMessage(Data::PlayerCharacter *character,
		const Common::String &message, uint8 line, bool withDelay) {
	if (!_engine || !_engine->getScreen())
		return;

	Surface surface(*_engine->getScreen(), Common::Rect(0, 0,
			_engine->getScreen()->w, _engine->getScreen()->h));
	if (_engine->getGameState() == GS_COMBAT) {
		surface.clearBox(23, line, 38, 21, 0);
		drawCharacterName(character, 23, line);
		printBlock(23, line + 1, 38, 21, 28, message);
	} else {
		const int startY = _messageBoxDrawn ? 18 : 17;
		if (!_messageBoxDrawn) {
			surface.drawWindow(1, 17, 38, 22, 0);
			_messageBoxDrawn = true;
		}

		surface.clearBox(1, startY, 38, 22, 0);
		drawCharacterName(character, 1, startY + 1);
		printBlock(1, startY + 2, 38, 22, 28, message);
	}

	// Delay/prompt handling is asynchronous in the View layer. The flag is
	// accepted here to preserve the legacy call contract without blocking the
	// engine thread.
	(void)withDelay;
}

void GameText::printText(const Common::String &text, bool clearBox) {
	if (clearBox)
		clearMessageArea();

	showMessage(nullptr, text, 17, false);
}

void GameText::clearMessageArea() {
	if (!_engine || !_engine->getScreen())
		return;

	Surface surface(*_engine->getScreen(), Common::Rect(0, 0,
			_engine->getScreen()->w, _engine->getScreen()->h));
	surface.clearBox(1, 17, 38, 22, 0);
	surface.clearBox(0, 24, 39, 24, 0);
	_messageBoxDrawn = false;
}

} // namespace Goldbox
