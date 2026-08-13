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

#ifndef GOLDBOX_GFX_GAME_TEXT_H
#define GOLDBOX_GFX_GAME_TEXT_H

#include "common/str.h"
#include "common/scummsys.h"
#include "goldbox/core/global.h"
#include "goldbox/gfx/surface.h"

namespace Goldbox {

namespace Data {
class PlayerCharacter;
}

class Engine;

/**
 * Shared legacy message-box text renderer.
 *
 * This is deliberately independent of a View or Dialog so ECL hosts,
 * combat, map events, and game-specific code can use the same text rules.
 * Interactive paging remains a UI concern; this class stores and renders the
 * current page when asked by a UI adapter.
 */
class GameText {
private:
	Engine *_engine;
	Common::String _characterName;
	int _characterNameColor;
	Common::String _text;
	uint _pageStart;
	uint _renderPos;
	uint8 _startX;
	uint8 _startY;
	uint8 _endX;
	uint8 _endY;
	uint8 _nameX;
	uint8 _nameY;
	uint8 _cursorX;
	uint8 _cursorY;
	int _textColor;
	bool _combat;
	bool _waitingForKey;
	bool _active;

	void drawCharacterName(Surface &surface) const;
	void drawRange(Surface &surface) const;
	bool advanceWord();

public:
	explicit GameText(Engine *engine);

	/** Return true for the legacy TEXT_BlockPrint word delimiters. */
	static bool isWordBreak(char c);

	/** Return the end of the next word, including its delimiter. */
	static uint wordEnd(const Common::String &text, uint start, uint limit);

	/** Render a legacy TEXT_drawIntoMsgBox message. */
	void showMessage(Data::PlayerCharacter *character,
			const Common::String &message, uint8 line, bool withDelay = false);

	/** Start ordinary ECL text in the normal message area. */
	void printText(const Common::String &text, bool clearBox = false);

	/** Start or append text in the normal dialog message area. */
	void setText(const Common::String &text, bool clearBox);

	/** Advance the message by one word. Returns true while work was done. */
	bool advance();

	/** Continue after a full page was acknowledged by the UI. */
	bool nextPage();

	/** Draw the current page and currently rendered words. */
	void draw(Surface &surface) const;

	bool isActive() const { return _active; }
	bool isWaitingForKey() const { return _waitingForKey; }
	bool isComplete() const { return _active && !_waitingForKey && _renderPos >= _text.size(); }
	bool isBusy() const { return _active && !isComplete(); }

	/** Clear the message area and its prompt line. */
	void clearMessageArea();
};

} // namespace Goldbox

#endif
