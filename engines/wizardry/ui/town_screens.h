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

#ifndef WIZARDRY_UI_TOWN_SCREENS_H
#define WIZARDRY_UI_TOWN_SCREENS_H

#include "wizardry/game/party.h"
#include "wizardry/game/shop.h"
#include "wizardry/gfx/cell_screen.h"
#include "wizardry/resources/messages.h"
#include "wizardry/ui/menu.h"

namespace Wizardry {

/**
 * Drawing of the castle and its places for Wizardry I, with the window
 * positions and message numbers of the original. These functions only
 * draw; they know nothing of the engine, keys or views.
 */
class TownScreens {
public:
	enum {
		kNobody = 0xffffffff
	};

private:
	CellScreen &_screen;
	const MessageFile &_messages;
	const Party &_party;
	const ItemList &_items;

	TextWindow openWindow(int x, int y, int width, int height);
	void printLong(TextWindow &window, int64 value);
	void drawLettered(const Common::Array<uint> &rosterNumbers);
	void printPartyLine(TextWindow &window, uint index);

public:
	TownScreens(CellScreen &screen, const MessageFile &messages, const Party &party, const ItemList &items) :
		_screen(screen), _messages(messages), _party(party), _items(items) {}

	/**
	 * What every place in town shows: the main window, the "CASTLE"
	 * heading and the party
	 */
	void drawBackground(bool withParty = true);
	void drawParty();

	/**
	 * A framed window with one line centred in it, as the original
	 * opens them for short notices and for headings
	 */
	void drawPopup(const Common::String &text, int y);

	void drawEdgeMenu(Menu &menu);
	void drawEnteringMaze();

	// ---- the training grounds, which show neither heading nor party

	void drawTrainingBackground();
	void drawTrainingMenu(Menu &menu);
	void drawRoster();

	/**
	 * The line at the top with what is known of the character so far
	 */
	void drawCharacterHeader(const Character &c, int y = 0);
	void drawTrainingPrompt(const Common::String &text);
	void drawTrainingInput(const Common::String &text);

	/**
	 * A lettered list of names taken from consecutive messages
	 */
	void drawTrainingChoices(uint firstMessage, uint count);

	/**
	 * Sharing out the bonus points
	 * @param cursor    The attribute the pointer is at
	 * @param eligible  For each of the eight classes, whether it is open
	 * @param help      Whether to show the two lines on the keys to use
	 */
	void drawTrainingPoints(const int16 *attributes, int bonus, int cursor, const bool *eligible, bool help);

	/**
	 * A small window in the middle with one line, for things said in passing
	 */
	void drawNotice(const Common::String &text);

	void drawCastleMenu(Menu &menu);

	void drawTavernMenu(Menu &menu);
	/**
	 * @param available  Roster numbers behind the letters A, B, ...;
	 *                   kNobody leaves a gap
	 */
	void drawTavernAdd(const Common::Array<uint> &available);
	void drawTavernRemove();

	/**
	 * The tavern asking for the password of the character behind a letter
	 * @param typed  Number of marks to show for what was typed
	 */
	void drawTavernPassword(const Common::Array<uint> &available, uint letter, uint typed);

	void drawInnWho();
	void drawInnRooms(uint member);
	void drawInnResting(uint member);
	void drawInnNapping(uint member);
	void drawInnResult(const Common::Array<Common::String> &lines);

	// ---- editing a character in the training grounds

	void drawEditMenu(Menu &menu);

	/**
	 * A line in the main window
	 */
	void drawMainLine(int row, const Common::String &text, bool centered, int x = 0);

	/**
	 * The classes a character could change to, lettered by class
	 */
	void drawClassChoices(const bool *open);
	void drawNoClassOpen();

	// ---- the character sheet

	void drawSheet(const Character &c);
	void drawSheetMenu(uint message, Menu &menu);

	/**
	 * The small window over the sheet that asks for something
	 * @param lines  Centred lines from the top
	 * @param input  Printed after them, at the left; nullptr for none
	 */
	void drawSheetPopup(const Common::Array<Common::String> &lines, const Common::String *input);

	/**
	 * What the sheet says in that window, between asterisks
	 */
	void drawSheetNotice(const Common::String &text);

	// ---- equipping

	/**
	 * The items of one kind to choose from
	 * @param kindIndex  Which of the six kinds, in the order asked
	 * @param slots      Their places in the pack
	 * @param prompt     Whether to ask which one
	 */
	void drawEquipChoices(const Character &c, int kindIndex, const Common::Array<uint> &slots, bool prompt);

	/**
	 * The notice that a cursed item has put itself on
	 */
	void drawEquipCursed();
	void drawInvokePower(const Character &c, uint slot);

	// ---- the spell books

	void drawSpellBooks(const Character &c, Menu &menu);

	/**
	 * The spells the character knows, one column for each spell level
	 */
	void drawSpellList(const Character &c, bool mage);

	/**
	 * The window of the temple, as it is, to print into
	 */
	TextWindow templeWindow();
	void drawTempleChoose(const Common::Array<uint> &patients);
	void drawTempleDonation(int64 amount);

	/**
	 * @param steps  How far the ritual has got: 1 "murmur" to 4 "invoke"
	 */
	void drawTempleRitual(uint steps);

	/**
	 * A line between asterisks in the temple, with a name put into it
	 */
	void drawTempleSay(uint message, const Common::String &name);

	/**
	 * The window of the trading post, as it is, to print into
	 */
	TextWindow shopWindow();
	void drawShopWho();
	void drawShopMenu(uint member, Menu &menu);

	/**
	 * The six catalogue lines for a member, with the gold left below
	 * @param menu  Receives the menu under the catalogue, unless it is nullptr
	 */
	void drawShopCatalog(uint member, const Shop &shop, Menu *menu);

	/**
	 * The member's pack with the price of a service for each item
	 * @param prompt  Whether to ask which item
	 */
	void drawShopPack(uint member, const Shop &shop, Shop::Service service, bool prompt);

	/**
	 * A line in the trading post
	 * @param stars  Whether to put it between asterisks, as its notices are
	 */
	void drawShopLine(int row, const Common::String &text, bool centered, bool stars);
};

} // End of namespace Wizardry

#endif
