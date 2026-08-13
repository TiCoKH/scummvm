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
 * Interactive paging remains a UI concern; this class renders one message
 * immediately into the current Goldbox screen.
 */
class GameText {
private:
	Engine *_engine;
	bool _messageBoxDrawn;

	void drawCharacterName(Data::PlayerCharacter *character, int x, int y);
	void printBlock(int startX, int startY, int endX, int endY,
			int color, const Common::String &text);

public:
	explicit GameText(Engine *engine);

	/** Return true for the legacy TEXT_BlockPrint word delimiters. */
	static bool isWordBreak(char c);

	/** Return the end of the next word, including its delimiter. */
	static uint wordEnd(const Common::String &text, uint start, uint limit);

	/** Render a legacy TEXT_drawIntoMsgBox message. */
	void showMessage(Data::PlayerCharacter *character,
			const Common::String &message, uint8 line, bool withDelay = false);

	/** Render ordinary ECL text in the shared message area. */
	void printText(const Common::String &text, bool clearBox = false);

	/** Clear the message area and its prompt line. */
	void clearMessageArea();
};

} // namespace Goldbox

#endif
