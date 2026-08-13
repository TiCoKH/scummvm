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

GameText::GameText(Engine *engine) : _engine(engine), _characterName(),
		_characterNameColor(kPartyNameColor),
		_pageStart(0), _renderPos(0), _startX(1), _startY(17),
		_endX(38), _endY(22), _nameX(1), _nameY(17), _cursorX(1), _cursorY(17),
		_textColor(kTextColor),
		_combat(false), _waitingForKey(false), _active(false) {
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

void GameText::drawCharacterName(Surface &surface) const {
	if (_characterName.empty())
		return;

	surface.writeStringC(_nameX, _nameY, _characterNameColor,
			_characterName);
}

void GameText::drawRange(Surface &surface) const {
	uint cursorX = _startX;
	uint cursorY = _startY;
	uint pos = _pageStart;
	while (pos < _renderPos && cursorY <= _endY) {
		const uint wordStart = pos;
		pos = wordEnd(_text, wordStart, _renderPos);
		const uint wordLength = pos - wordStart;
		if (cursorX != _startX && cursorX + wordLength > _endX + 1) {
			++cursorY;
			cursorX = _startX;
		}
		if (cursorY > _endY)
			break;
		for (uint i = wordStart; i < pos; ++i) {
			if (_text[i] == ' ' && cursorX > _endX)
				continue;
			surface.writeCharC(cursorX++, cursorY, _textColor, _text[i]);
		}
	}
}

bool GameText::advanceWord() {
	if (!_active || _waitingForKey || _renderPos >= _text.size())
		return false;

	const uint wordStart = _renderPos;
	const uint wordEndPos = wordEnd(_text, wordStart, _text.size());
	const uint wordLength = wordEndPos - wordStart;
	if (_cursorX != _startX && _cursorX + wordLength > _endX + 1) {
		++_cursorY;
		_cursorX = _startX;
	}
	if (_cursorY > _endY) {
		_waitingForKey = true;
		return false;
	}

	_renderPos = wordEndPos;
	_cursorX += wordLength;
	return true;
}

void GameText::showMessage(Data::PlayerCharacter *character,
		const Common::String &message, uint8 line, bool withDelay) {
	_characterName.clear();
	_characterNameColor = kPartyNameColor;
	if (character) {
		_characterName = character->name;
		if (!character->enabled)
			_characterNameColor = kDisabledNameColor;
		else if (character->hostile)
			_characterNameColor = kEnemyNameColor;
	}
	_text = message;
	_pageStart = 0;
	_renderPos = 0;
	_combat = _engine && _engine->getGameState() == GS_COMBAT;
	_startX = _combat ? 23 : 1;
	_startY = _combat ? line + 1 : line + 2;
	_nameX = _combat ? 23 : 1;
	_nameY = line;
	_endX = 38;
	_endY = _combat ? 21 : 22;
	_cursorX = _startX;
	_cursorY = _startY;
	_textColor = _combat ? 28 : kTextColor;
	_waitingForKey = false;
	_active = true;
	(void)withDelay;
}

void GameText::printText(const Common::String &text, bool clearBox) {
	setText(text, clearBox);
}

void GameText::setText(const Common::String &text, bool clearBox) {
	if (clearBox || !_active) {
		_characterName.clear();
		_characterNameColor = kPartyNameColor;
		_text = text;
		_pageStart = 0;
		_renderPos = 0;
		_startX = 1;
		_startY = 17;
		_endX = 38;
		_endY = 22;
		_nameX = 1;
		_nameY = 17;
		_cursorX = _startX;
		_cursorY = _startY;
		_textColor = kTextColor;
		_combat = false;
		_waitingForKey = false;
		_active = true;
		return;
	}

	if (_waitingForKey) {
		_pageStart = _renderPos;
		_cursorX = _startX;
		_cursorY = _startY;
		_waitingForKey = false;
	}
	_text += text;
}

bool GameText::advance() {
	return advanceWord();
}

bool GameText::nextPage() {
	if (!_waitingForKey)
		return false;
	_pageStart = _renderPos;
	_cursorX = _startX;
	_cursorY = _startY;
	_waitingForKey = false;
	return true;
}

void GameText::draw(Surface &surface) const {
	if (!_active)
		return;
	if (_combat)
		surface.clearBox(_nameX, _nameY, _endX, _endY, 0);
	else
		surface.clearBox(_startX, _startY, _endX, _endY, 0);
	drawCharacterName(surface);
	drawRange(surface);
}

void GameText::clearMessageArea() {
	_active = false;
	_waitingForKey = false;
	_characterName.clear();
	_text.clear();
	_pageStart = 0;
	_renderPos = 0;

	if (_engine && _engine->getScreen()) {
		Surface surface(*_engine->getScreen(), Common::Rect(0, 0,
				_engine->getScreen()->w, _engine->getScreen()->h));
		surface.clearBox(1, 17, 38, 22, 0);
		surface.clearBox(0, 24, 39, 24, 0);
	}
}

} // namespace Goldbox
