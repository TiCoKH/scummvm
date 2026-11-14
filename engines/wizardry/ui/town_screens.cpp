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

#include "wizardry/game/inn.h"
#include "wizardry/game/training.h"
#include "wizardry/ui/maze_view.h"
#include "wizardry/ui/town_screens.h"

namespace Wizardry {

// Message numbers of Wizardry I
enum {
	kMsgInvoke1 = 251,
	kMsgInvoke2 = 252,
	kMsgInvokeYesNo = 253,
	kMsgEquipWhich = 256,
	kMsgEquipNone = 257,
	kMsgSelectKind = 258,
	kMsgKindNames = 259,         // weapon, armor, shield, helm, gauntlets, misc. item
	kMsgCursed = 265,
	kMsgReordering = 266,
	kMsgEventPress = 101,
	kMsgRosterDeadOut = 300,     // "D/O": dead, and lying in the maze
	kMsgRosterDead = 301,
	kMsgRosterOut = 302,
	kMsgRosterPress = 303,
	kMsgCannotQualify = 309,
	kMsgCannotQualifyPress = 310,
	kMsgEditMenu = 318,
	kMsgBonus = 323,
	kMsgPointsHelp1 = 324,
	kMsgPointsHelp2 = 325,
	kMsgTrainingGrounds = 333,
	kMsgTrainingMenu = 334,
	kMsgArmorClassLow = 400,     // "LO"
	kMsgEntering = 500,
	kMsgSheetMage = 603,
	kMsgSheetPriest = 604,
	kMsgSheetStrength = 605,     // the labels of the sheet follow, to 618
	kMsgEnteringName = 501,
	kMsgEdgeMenu = 502,
	kMsgCamp = 652,
	kMsgCampMenu = 653,
	kMsgButtonsHere = 710,
	kMsgMarkedThrough = 711,
	kMsgPressOne = 712,
	kMsgMazeCommands = 717,
	kMsgWhoWill = 800,
	kMsgChestReturn = 801,
	kMsgChest = 825,
	kMsgChestMenu = 826,
	kMsgPressReturn = 900,
	kMsgOptionsOf = 1027,
	kMsgSpellsEquals = 901,
	kMsgBookMage = 902,
	kMsgBookPriest = 903,
	kMsgBookMenu = 904,
	kMsgMageSpells = 905,
	kMsgPriestSpells = 906,
	kMsgSpellNames = 5000,       // + spell; a '*' in front starts a spell level
	kMsgRaceNames = 9999,        // + race
	kMsgAttributeNames = 10020,  // + attribute
	kMsgPoison = 401,
	kMsgPartyHeader = 402,
	kMsgCastle = 403,
	kMsgCastleMenu1 = 404,
	kMsgCastleMenu2 = 405,
	kMsgLetterSelects = 406,
	kMsgEnterPassword = 410,
	kMsgInn = 417,
	kMsgWhoWillStay = 418,
	kMsgInnReturnToLeave = 419,
	kMsgWelcome = 420,
	kMsgFirstRoom = 422,
	kMsgPoolOrLeave = 427,
	kMsgRecuperating = 436,
	kMsgHitPoints = 437,
	kMsgGold = 438,
	kMsgNapping = 439,
	kMsgInnResultLeave = 440,
	kMsgYouHaveGold = 443,
	kMsgTempleWelcome = 512,
	kMsgTempleWho = 513,
	kMsgTempleLeave = 514,
	kMsgDonation = 519,
	kMsgWhoWillTithe = 520,
	kMsgTitheLeave = 521,
	kMsgMurmur = 525,            // then chant, pray, invoke
	kMsgUseless = 550,
	kMsgGoldLeft = 561,
	kMsgCatalogMenu = 562,
	kMsgSell = 569,              // then uncurse, identify
	kMsgWhich = 572,
	kMsgShopWelcome = 573,
	kMsgShopYouHave = 574,
	kMsgShopGold = 575,
	kMsgShopMenu = 576,
	kMsgShopTitle = 577,
	kMsgShopWho = 578,
	kMsgShopLeave = 579,
	kMsgRemoveWho = 413,
	kMsgReturnToLeave = 414,
	kMsgTavern = 415,
	kMsgTavernMenu = 416,
	kMsgAlignmentNames = 10009,  // + alignment
	kMsgClassNames = 10030,      // + class
	kMsgStatusNames = 10040      // + status
};

TextWindow TownScreens::openWindow(int x, int y, int width, int height) {
	TextWindow window(_screen, Common::Rect(x, y, x + width, y + height), true);
	window.clear();
	return window;
}

void TownScreens::drawCampBackground(bool withParty) {
	_screen.clear();
	openWindow(1, 1, 38, 22);

	const Common::String &title = _messages.get(kMsgCamp);
	TextWindow heading(_screen, TextWindow::centeredBounds(title.size() - 1, 0, 3), true);
	heading.clear();
	heading.PRINTSTR(title);

	if (withParty)
		drawParty();
}

void TownScreens::drawCampMenu(Menu &menu) {
	const Common::String &options = _messages.get(kMsgCampMenu);

	TextWindow window(_screen, TextWindow::centeredBounds(Menu::textWidth(options), 4, 6), true);
	window.clear();
	menu.print(window, options);
}

void TownScreens::drawReorder(const Common::Array<uint> &chosen, bool asking) {
	TextWindow window = openWindow(8, 4, 24, 10);
	window.printCenteredLine(_messages.get(kMsgReordering));

	for (uint i = 0; i < _party.size(); ++i) {
		window.MVCURSOR(0, i + 2);
		window.printInt(i + 1, 1);
		window.PRINTCHR(')');

		if (i < chosen.size()) {
			window.PRINTCHR(' ');
			window.PRINTSTR(_party.member(chosen[i]).name);
		} else if (i == chosen.size() && asking) {
			window.PRINTCHR('>');
		}
	}
}

void TownScreens::drawBackground(bool withParty) {
	_screen.clear();

	// The main window is empty in town
	openWindow(1, 1, 38, 22);

	const Common::String &title = _messages.get(kMsgCastle);
	TextWindow heading(_screen, TextWindow::centeredBounds(title.size() - 1, 0, 3), true);
	heading.clear();
	heading.PRINTSTR(title);

	if (withParty)
		drawParty();
}

void TownScreens::drawParty() {
	TextWindow window = openWindow(0, 16, 40, 9);
	window.PRINTSTR(0, 0, _messages.get(kMsgPartyHeader));

	for (uint i = 0; i < _party.size(); ++i)
		printPartyLine(window, i);
}

void TownScreens::printPartyLine(TextWindow &window, uint index) {
	const Character &c = _party.member(index);

	window.MVCURSOR(0, index + 1);
	window.printInt(index + 1, 1);
	window.PRINTCHR(' ');
	window.PRINTSTR(c.name);
	while (window.cursorX() < 18)
		window.PRINTCHR(' ');

	window.PRINTSTR(_messages.get(kMsgAlignmentNames + c.alignment).substr(0, 1));
	window.PRINTCHR('-');
	window.PRINTSTR(_messages.get(kMsgClassNames + c.classType).substr(0, 3));
	window.PRINTCHR(' ');

	if (c.armorClass < -9)
		window.PRINTSTR(_messages.get(kMsgArmorClassLow));
	else
		window.printInt(c.armorClass, 2);

	window.printInt(c.hp, 5);
	window.PRINTCHR(' ');

	if (c.status != kStatusOk)
		window.PRINTSTR(_messages.get(kMsgStatusNames + c.status).substr(0, 6));
	else if (c.poison)
		window.PRINTSTR(_messages.get(kMsgPoison));
	else
		window.printInt(c.hpMax, 4);
}

void TownScreens::drawNotice(const Common::String &text) {
	TextWindow window(_screen, TextWindow::centeredBounds(text.size(), 9, 3), true);
	window.clear();
	window.printCenteredLine(text);
}

void TownScreens::drawCastleMenu(Menu &menu) {
	const Common::String options = _messages.get(kMsgCastleMenu1) + _messages.get(kMsgCastleMenu2);

	TextWindow window(_screen, TextWindow::centeredBounds(Menu::textWidth(options), 4, 7), true);
	window.clear();
	menu.print(window, options);
}

void TownScreens::drawTavernMenu(Menu &menu) {
	const Common::String &title = _messages.get(kMsgTavern);
	const Common::String &options = _messages.get(kMsgTavernMenu);
	const int width = MAX<int>(title.size() + 1, Menu::textWidth(options));

	TextWindow window(_screen, TextWindow::centeredBounds(width, 4, 9), true);
	window.clear();
	window.printCenteredLine(title);
	window.newLine();
	menu.print(window, options);
}

void TownScreens::drawLettered(const Common::Array<uint> &rosterNumbers) {
	// Two columns of ten, lettered from A
	TextWindow list = openWindow(0, 9, 40, 12);
	for (uint i = 0; i < rosterNumbers.size(); ++i) {
		if (rosterNumbers[i] == kNobody)
			continue;
		list.MVCURSOR((i / 10) * 19, i % 10);
		list.PRINTCHR('A' + i);
		list.PRINTSTR(") ");
		list.PRINTSTR(_party.rosterEntry(rosterNumbers[i]).name);
	}
}

void TownScreens::printLong(TextWindow &window, int64 value) {
	// Right-justified in twelve columns
	window.PRINTSTR(Common::String::format("%12lld", (long long)value));
}

void TownScreens::drawTavernAdd(const Common::Array<uint> &available) {
	TextWindow prompt = openWindow(2, 6, 36, 4);
	prompt.printCenteredLine(_messages.get(kMsgLetterSelects));

	drawLettered(available);
}

void TownScreens::drawTavernRemove() {
	TextWindow window = openWindow(8, 7, 24, 4);
	window.printCenteredLine(_messages.get(kMsgRemoveWho));
	window.printCenteredLine(_messages.get(kMsgReturnToLeave));
}

void TownScreens::drawTavernPassword(const Common::Array<uint> &available, uint letter, uint typed) {
	drawTavernAdd(available);

	// An arrow marks whose password is wanted
	TextWindow list(_screen, Common::Rect(0, 9, 40, 21), true);
	list.PRINTSTR((letter / 10) * 19, letter % 10, "-->");

	TextWindow prompt = openWindow(2, 6, 36, 4);
	prompt.PRINTSTR(_messages.get(kMsgEnterPassword));
	for (uint i = 0; i < typed && prompt.cursorX() < prompt.width() - 1; ++i)
		prompt.PRINTCHR('X');
}

void TownScreens::drawInnWho() {
	TextWindow window = openWindow(0, 2, 40, 13);
	window.printCenteredLine(_messages.get(kMsgInn));
	window.newLine();
	window.printCenteredLine(_messages.get(kMsgWhoWillStay));
	window.printCenteredLine(_messages.get(kMsgInnReturnToLeave));
}

void TownScreens::drawInnRooms(uint member) {
	const Character &c = _party.member(member);
	TextWindow window = openWindow(0, 2, 40, 13);

	window.printCenteredLine(substCaret(_messages.get(kMsgWelcome), c.name));
	window.newLine();
	for (uint id = kMsgFirstRoom; id <= kMsgPoolOrLeave; ++id) {
		if (id == kMsgPoolOrLeave)
			window.newLine();
		window.PRINTSTR(_messages.get(id));
		window.newLine();
	}
	window.newLine();
	window.PRINTSTR(substCaret(_messages.get(kMsgYouHaveGold),
		Common::String::format("%lld", (long long)c.gold)));
}

void TownScreens::drawInnResting(uint member) {
	const Character &c = _party.member(member);
	TextWindow window = openWindow(0, 2, 40, 13);

	window.printCenteredLine(substCaret(_messages.get(kMsgRecuperating), c.name));
	window.newLine();
	window.newLine();
	window.PRINTSTR(_messages.get(kMsgHitPoints));
	window.printInt(c.hp, 5);
	window.PRINTCHR('/');
	window.printInt(c.hpMax, 5);
	window.PRINTCHR(')');
	window.newLine();
	window.PRINTSTR(_messages.get(kMsgGold));
	window.PRINTSTR(Common::String::format("%lld", (long long)c.gold));
}

void TownScreens::drawInnNapping(uint member) {
	TextWindow window = openWindow(0, 2, 40, 13);
	window.printCenteredLine(substCaret(_messages.get(kMsgNapping), _party.member(member).name));
}

void TownScreens::drawInnResult(const Common::Array<Common::String> &lines) {
	TextWindow window = openWindow(0, 2, 40, 13);

	for (uint i = 0; i < lines.size(); ++i) {
		window.PRINTSTR(lines[i]);
		window.newLine();
	}

	window.MVCURSOR(0, 9);
	window.printCenteredLine(_messages.get(kMsgInnResultLeave));
}

void TownScreens::drawPopup(const Common::String &text, int y) {
	TextWindow window(_screen, TextWindow::centeredBounds(text.size(), y, 3), true);
	window.clear();
	window.printCenteredLine(text);
}

void TownScreens::drawEdgeMenu(Menu &menu) {
	const Common::String &options = _messages.get(kMsgEdgeMenu);

	TextWindow window(_screen, TextWindow::centeredBounds(Menu::textWidth(options), 4, 7), true);
	window.clear();
	menu.print(window, options);
}

void TownScreens::drawEnteringMaze() {
	TextWindow window = openWindow(0, 10, 40, 4);
	window.printCenteredLine(_messages.get(kMsgEntering));
	window.printCenteredLine(_messages.get(kMsgEnteringName));
}

void TownScreens::drawTrainingBackground() {
	_screen.clear();
	openWindow(1, 1, 38, 22);
}

void TownScreens::drawTrainingMenu(Menu &menu) {
	const Common::String &title = _messages.get(kMsgTrainingGrounds);
	TextWindow heading(_screen, TextWindow::centeredBounds(title.size() - 1, 0, 3), true);
	heading.clear();
	heading.PRINTSTR(title);

	const Common::String &options = _messages.get(kMsgTrainingMenu);
	TextWindow window(_screen, TextWindow::centeredBounds(Menu::textWidth(options), 8, 6), true);
	window.clear();
	menu.print(window, options);
}

void TownScreens::drawRoster() {
	TextWindow window = openWindow(1, 1, 38, 22);
	int row = 0;

	for (uint i = 0; i < _party.rosterSize(); ++i) {
		const Character &c = _party.rosterEntry(i);
		if (c.isFree())
			continue;

		window.PRINTSTR(0, row++, c.name);
		while (window.cursorX() < 16)
			window.PRINTCHR(' ');
		window.PRINTCHR('L');
		window.printInt(c.level, 3);
		window.PRINTCHR(' ');
		window.PRINTSTR(_messages.get(kMsgAlignmentNames + c.alignment).substr(0, 1));
		window.PRINTCHR('-');
		window.PRINTSTR(_messages.get(kMsgClassNames + c.classType).substr(0, 3));
		window.PRINTCHR(' ');
		window.PRINTSTR(_messages.get(kMsgRaceNames + c.race).substr(0, 3));
		window.PRINTCHR(' ');

		// Out: with a party, or lying in the maze
		const bool out = c.lostLevel != 0 || c.inMaze;
		if (c.status < kStatusDead) {
			if (out)
				window.PRINTSTR(_messages.get(kMsgRosterOut));
		} else {
			window.PRINTSTR(_messages.get(out ? kMsgRosterDeadOut : kMsgRosterDead));
		}
	}

	drawPopup(_messages.get(kMsgRosterPress), 21);
}

void TownScreens::drawCharacterHeader(const Character &c, int y) {
	TextWindow window = openWindow(0, y, 40, 3);

	window.PRINTSTR(c.name);
	while (window.cursorX() < 16)
		window.PRINTCHR(' ');
	window.PRINTCHR('L');
	window.printInt(c.level, 3);
	window.PRINTCHR(' ');

	if (c.alignment == kAlignNone)
		window.PRINTCHR('?');
	else
		window.PRINTSTR(_messages.get(kMsgAlignmentNames + c.alignment).substr(0, 1));
	window.PRINTCHR('-');

	if (c.classType == kClassNone)
		window.PRINTSTR("???");
	else
		window.PRINTSTR(_messages.get(kMsgClassNames + c.classType).substr(0, 3));
	window.PRINTCHR(' ');

	if (c.race == kRaceNone)
		window.PRINTSTR("???");
	else
		window.PRINTSTR(_messages.get(kMsgRaceNames + c.race));
}

void TownScreens::drawTrainingPrompt(const Common::String &text) {
	TextWindow window = openWindow(0, 21, 40, 3);
	window.printCenteredLine(text);
}

void TownScreens::drawTrainingInput(const Common::String &text) {
	TextWindow window = openWindow(0, 10, 40, 3);
	window.PRINTCHR('>');
	window.PRINTSTR(text);
}

void TownScreens::drawTrainingChoices(uint firstMessage, uint count) {
	TextWindow window = openWindow(13, 8, 14, count + 2);

	for (uint i = 0; i < count; ++i) {
		window.MVCURSOR(0, i);
		window.PRINTCHR('A' + i);
		window.PRINTSTR(") ");
		window.PRINTSTR(_messages.get(firstMessage + i));
	}
}

void TownScreens::drawTrainingPoints(const int16 *attributes, int bonus, int cursor, const bool *eligible, bool help) {
	if (help) {
		TextWindow window = openWindow(0, 20, 40, 4);
		window.printCenteredLine(_messages.get(kMsgPointsHelp1));
		window.printCenteredLine(_messages.get(kMsgPointsHelp2));
	}

	// Names right-justified in eight columns, the number after them
	TextWindow stats = openWindow(7, 6, 13, 10);
	for (int row = 0; row <= 7; ++row) {
		if (row == 6)
			continue;

		const Common::String &name = _messages.get(row == 7 ? (uint)kMsgBonus : kMsgAttributeNames + row);
		stats.PRINTSTR(MAX<int>(8 - (int)name.size(), 0), row, name);
		stats.MVCURSOR(9, row);
		stats.printInt(row == 7 ? bonus : attributes[row], 2);
	}
	stats.MVCURSOR(8, cursor);
	stats.PRINTCHR('_');

	TextWindow classes = openWindow(20, 6, 13, 10);
	for (int i = 0; i < kClassCount; ++i) {
		if (!eligible[i])
			continue;
		classes.MVCURSOR(0, i);
		classes.PRINTCHR('A' + i);
		classes.PRINTSTR(") ");
		classes.PRINTSTR(_messages.get(kMsgClassNames + i));
	}
}

void TownScreens::drawEditMenu(Menu &menu) {
	const Common::String &options = _messages.get(kMsgEditMenu);

	TextWindow window(_screen, TextWindow::centeredBounds(Menu::textWidth(options), 8, 7), true);
	window.clear();
	menu.print(window, options);
}

void TownScreens::drawMainLine(int row, const Common::String &text, bool centered, int x) {
	TextWindow window(_screen, Common::Rect(1, 1, 39, 23), true);
	window.MVCURSOR(x, row);
	if (centered)
		window.printCenteredLine(text);
	else
		window.PRINTSTR(text);
}

void TownScreens::drawClassChoices(const bool *open) {
	TextWindow window = openWindow(13, 11, 14, 10);

	for (int i = 0; i < kClassCount; ++i) {
		if (!open[i])
			continue;
		window.PRINTCHR('A' + i);
		window.PRINTSTR(") ");
		window.PRINTSTR(_messages.get(kMsgClassNames + i));
		window.newLine();
	}
}

void TownScreens::drawNoClassOpen() {
	TextWindow window = openWindow(0, 10, 40, 4);
	window.printCenteredLine(_messages.get(kMsgCannotQualify));
	window.printCenteredLine(_messages.get(kMsgCannotQualifyPress));
}

void TownScreens::drawSheet(const Character &c) {
	TextWindow window = openWindow(0, 4, 40, 17);

	window.PRINTSTR(c.name);
	window.PRINTSTR(" L");
	window.printInt(c.level, 3);
	window.PRINTCHR(' ');
	window.PRINTSTR(_messages.get(kMsgAlignmentNames + c.alignment).substr(0, 1));
	window.PRINTCHR('-');
	window.PRINTSTR(_messages.get(kMsgClassNames + c.classType).substr(0, 3));
	window.PRINTCHR(' ');
	window.PRINTSTR(_messages.get(kMsgRaceNames + c.race));
	// TODO: the marks of honour after the race

	// Three columns: an attribute, then gold, experience, hit points and
	// status, then level, age and armour class. The original prints this
	// as one run of text that wraps at the edge of the window.
	uint label = kMsgSheetStrength;

	window.MVCURSOR(0, 2);
	window.PRINTSTR(_messages.get(label++));           // strength
	window.printInt(c.attributes[kStrength], 3);
	window.PRINTSTR(_messages.get(label++));           // gold
	printLong(window, c.gold);
	window.PRINTSTR(_messages.get(label++));           // level
	window.printInt(c.level, 4);

	window.MVCURSOR(0, 3);
	window.PRINTSTR(_messages.get(label++));           // IQ
	window.printInt(c.attributes[kIQ], 3);
	window.PRINTSTR(_messages.get(label++));           // experience
	printLong(window, c.experience);
	window.PRINTSTR(_messages.get(label++));           // age, in years
	window.printInt(c.age / 52, 4);

	window.MVCURSOR(0, 4);
	window.PRINTSTR(_messages.get(label++));           // piety
	window.printInt(c.attributes[kPiety], 3);

	window.MVCURSOR(0, 5);
	window.PRINTSTR(_messages.get(label++));           // vitality
	window.printInt(c.attributes[kVitality], 3);
	window.PRINTSTR(_messages.get(label++));           // hit points
	window.printInt(c.hp, 5);
	window.PRINTCHR('/');
	window.printInt(c.hpMax, 4);
	window.PRINTSTR(_messages.get(label++));           // armour class
	window.printInt(c.armorClass, 3);

	window.MVCURSOR(0, 6);
	window.PRINTSTR(_messages.get(label++));           // agility
	window.printInt(c.attributes[kAgility], 3);

	window.MVCURSOR(0, 7);
	window.PRINTSTR(_messages.get(label++));           // luck
	window.printInt(c.attributes[kLuck], 3);
	window.PRINTSTR(_messages.get(label++));           // status
	window.PRINTSTR(_messages.get(kMsgStatusNames + c.status));
	if (c.poison > 0)
		window.PRINTSTR(_messages.get(label));

	// Spells that can still be cast, by spell level
	window.MVCURSOR(0, 9);
	window.PRINTSTR(_messages.get(kMsgSheetMage));
	for (int i = 0; i < 7; ++i) {
		window.printInt(c.mageSlots[i], 1);
		if (i < 6)
			window.PRINTCHR('/');
	}
	window.PRINTSTR(_messages.get(kMsgSheetPriest));
	for (int i = 0; i < 7; ++i) {
		window.printInt(c.priestSlots[i], 1);
		if (i < 6)
			window.PRINTCHR('/');
	}

	// The pack in two columns.
	// * equipped, - equipped and cursed, ? not identified, # of no use to this class
	for (int i = 0; i < c.itemCount; ++i) {
		const ItemSlot &slot = c.items[i];
		if (slot.id < 0 || slot.id >= (int)_items.size())
			continue;

		window.MVCURSOR((i % 2) * 19, i / 2 + 11);
		window.printInt(i + 1, 1);
		window.PRINTCHR(')');
		if (slot.equipped)
			window.PRINTCHR(_items[slot.id].cursed ? '-' : '*');
		else if (!slot.identified)
			window.PRINTCHR('?');
		else
			window.PRINTCHR(_items[slot.id].usableBy(c) ? ' ' : '#');
		window.PRINTSTR(_messages.get(ItemList::nameMessage(slot.id, slot.identified)));
	}
}

void TownScreens::drawSheetMenu(uint message, Menu &menu) {
	TextWindow window = openWindow(0, 21, 40, 4);
	menu.print(window, _messages.get(message));
}

void TownScreens::drawSheetPopup(const Common::Array<Common::String> &lines, const Common::String *input) {
	TextWindow window = openWindow(13, 8, 26, 5);

	for (uint i = 0; i < lines.size(); ++i)
		window.printCenteredLine(lines[i]);
	if (input)
		window.PRINTSTR(*input);
}

void TownScreens::drawSheetNotice(const Common::String &text) {
	TextWindow window = openWindow(13, 8, 26, 5);
	window.MVCURSOR(0, 1);
	window.printCenteredLine("* " + text + " *");
}

void TownScreens::drawMaze(const MazeLevel &level, const MazeState &state, bool windows, bool party) {
	_screen.clear();

	TextWindow view = openWindow(1, 1, 38, 22);
	MazeViewDrawer drawer(view, level, state, state.lightDuration > 0);
	drawer.draw();

	if (windows) {
		TextWindow title = openWindow(0, 0, 40, 3);
		title.PRINTSTR(_messages.get(kMsgMazeCommands));
	}

	if (party)
		drawParty();
}

void TownScreens::drawMazePopup(const Common::String &text, const Common::String &second) {
	const int width = MAX(text.size(), second.size());
	const int half = (width + 1) / 2;
	const int rows = second.empty() ? 1 : 2;

	TextWindow window = openWindow(19 - half, 10, half * 2 + 2, rows + 2);
	window.PRINTSTR(text);
	if (!second.empty())
		window.PRINTSTR(0, 1, second);
}

TextWindow TownScreens::battleWindow() {
	return TextWindow(_screen, Common::Rect(0, 5, 40, 11), true);
}

void TownScreens::drawBattle(const Battle &battle, const CellImage *picture) {
	_screen.clear();
	openWindow(1, 1, 38, 22);

	// The picture sits in the middle of the main window, below the text
	if (picture && !picture->empty())
		_screen.addImage(*picture, 2 + 15, 2 + 9);

	TextWindow enemies = openWindow(2, 0, 36, 6);
	for (int g = 1; g < Battle::kGroupCount; ++g) {
		if (battle.group(g).aliveCount <= 0)
			continue;

		int able = 0;
		const Common::String line = battle.groupLine(g, able);
		enemies.printInt(g, 1);
		enemies.PRINTSTR(") ");
		enemies.PRINTSTR(line);
		enemies.MVCURSOR(enemies.width() - 6, enemies.cursorY());
		enemies.PRINTSTR(" (");
		enemies.printInt(able, 1);
		enemies.PRINTCHR(')');
		enemies.newLine();
	}

	battleWindow().clear();
	drawParty();
}

void TownScreens::drawBattleOptions(const Character &c, const Common::String &options, Menu &menu) {
	int rows = 1;
	for (uint i = 0; i < options.size(); ++i)
		rows += options[i] == '/';

	const Common::String title = c.name + _messages.get(kMsgOptionsOf);
	const int width = MAX<int>(title.size(), Menu::textWidth(options));

	TextWindow window(_screen, TextWindow::centeredBounds(width, 8, rows + 4), true);
	window.clear();
	window.printCenteredLine(title);
	window.newLine();
	menu.print(window, options);
}

TextWindow TownScreens::chestWindow() {
	return TextWindow(_screen, Common::Rect(0, 2, 40, 8), true);
}

void TownScreens::drawChest(Menu &menu) {
	TextWindow window = chestWindow();
	window.clear();
	window.printCenteredLine(_messages.get(kMsgChest));
	window.MVCURSOR(0, 2);
	menu.print(window, _messages.get(kMsgChestMenu));
}

void TownScreens::drawChestWho(uint deed) {
	TextWindow window = chestWindow();
	window.clear();
	window.printCenteredLine(_messages.get(kMsgChest));
	window.MVCURSOR(0, 2);
	window.printCenteredLine(substCaret(_messages.get(kMsgWhoWill), _messages.get(deed)));
	window.printCenteredLine(_messages.get(kMsgChestReturn));
}

uint TownScreens::drawEventText(const Common::Array<Common::String> &lines, uint first) {
	// The page ends before a line that starts with '@', or after 13 lines
	uint end = first;
	while (end < lines.size() && end - first < 13 && !(end > first && lines[end].hasPrefix("@")))
		++end;

	// The window is as high as the longest page of the text
	uint longest = 1, run = 0;
	for (uint i = 0; i < lines.size(); ++i) {
		run = lines[i].hasPrefix("@") ? 1 : run + 1;
		longest = MAX(longest, run);
	}
	longest = MIN<uint>(longest, 13);

	TextWindow window = openWindow(0, 0, 40, longest + 2);
	int width = 0;
	for (uint i = first; i < end; ++i) {
		Common::String line = lines[i];
		if (line.hasPrefix("@"))
			line.deleteChar(0);

		int x;
		if (line.hasPrefix("$")) {
			// At the left; a line under it lines up with a full line
			line.deleteChar(0);
			x = 0;
			width = window.width();
		} else if (line.hasPrefix("^")) {
			// Under the line before
			line.deleteChar(0);
			x = window.width() / 2 - (width + 1) / 2;
		} else {
			width = line.size();
			x = window.width() / 2 - (width + 1) / 2;
		}
		window.PRINTSTR(x, i - first, line);
	}

	drawPopup(_messages.get(kMsgEventPress), longest + 3);
	return end;
}

void TownScreens::drawEventLine(const Common::String &line, const Common::String &second) {
	TextWindow window = openWindow(0, 0, 40, second.empty() ? 3 : 4);
	window.printCenteredLine(line);
	if (!second.empty())
		window.printCenteredLine(second);
}

void TownScreens::drawElevator(int floors) {
	TextWindow window = openWindow(0, 0, 40, 5);
	window.printCenteredLine(_messages.get(kMsgButtonsHere));
	window.printCenteredLine(substCaret(_messages.get(kMsgMarkedThrough), Common::String((char)('A' + floors - 1))));
	window.printCenteredLine(_messages.get(kMsgPressOne));
}

void TownScreens::drawMainText(const Common::Array<Common::String> &lines) {
	_screen.clear();
	TextWindow window = openWindow(1, 1, 38, 22);
	window.MVCURSOR(0, MAX<int>((window.height() - (int)lines.size()) / 2, 0));
	for (uint i = 0; i < lines.size(); ++i)
		window.printCenteredLine(lines[i]);
}

void TownScreens::drawEquipChoices(const Character &c, int kindIndex, const Common::Array<uint> &slots, bool prompt) {
	TextWindow window = openWindow(8, 8, 24, 15);

	window.printCenteredLine(substCaret(_messages.get(kMsgSelectKind), _messages.get(kMsgKindNames + kindIndex)));
	window.newLine();

	for (uint i = 0; i < slots.size(); ++i) {
		const ItemSlot &slot = c.items[slots[i]];
		window.printInt(i + 1, 1);
		window.PRINTCHR(')');
		// - cursed, ? not identified
		window.PRINTCHR(slot.cursed ? '-' : (slot.identified ? ' ' : '?'));
		window.PRINTSTR(_messages.get(ItemList::nameMessage(slot.id, slot.identified)));
		window.newLine();
	}

	if (prompt) {
		window.MVCURSOR(0, window.cursorY() + 1);
		window.printCenteredLine(_messages.get(kMsgEquipWhich));
		window.printCenteredLine(_messages.get(kMsgEquipNone));
	}
}

void TownScreens::drawEquipCursed() {
	TextWindow window = openWindow(8, 8, 24, 15);
	window.MVCURSOR(0, 4);
	window.printCenteredLine(_messages.get(kMsgCursed));
}

void TownScreens::drawInvokePower(const Character &c, uint slot) {
	TextWindow window = openWindow(4, 12, 32, 8);
	const ItemSlot &item = c.items[slot];

	window.printCenteredLine(_messages.get(kMsgInvoke1));
	window.printCenteredLine(substCaret(_messages.get(kMsgInvoke2),
		_messages.get(ItemList::nameMessage(item.id, item.identified))));
	window.newLine();
	window.printCenteredLine(_messages.get(kMsgInvokeYesNo));
}

void TownScreens::drawSpellBooks(const Character &c, Menu &menu) {
	TextWindow window = openWindow(0, 7, 40, 17);

	for (int book = 0; book < 2; ++book) {
		const int16 *slots = book ? c.priestSlots : c.mageSlots;
		window.PRINTSTR(_messages.get(book ? kMsgBookPriest : kMsgBookMage));
		window.PRINTSTR(_messages.get(kMsgSpellsEquals));
		for (int i = 0; i < 7; ++i) {
			if (i)
				window.PRINTCHR('/');
			window.printInt(slots[i], 1);
		}
		window.newLine();
	}

	window.MVCURSOR(8, 4);
	menu.print(window, _messages.get(kMsgBookMenu));
}

void TownScreens::drawSpellList(const Character &c, bool mage) {
	TextWindow window = openWindow(0, 7, 40, 17);
	const uint first = mage ? 1 : 22, last = mage ? 21 : 50;

	// Four spell levels side by side, the others below them
	int level = 0, x = 0, y = 0;
	for (uint spell = first; spell <= last; ++spell) {
		Common::String name = _messages.get(kMsgSpellNames + spell);

		if (!name.empty() && name[0] == '*') {
			x = (level % 4) * 9;
			y = (level / 4) * 7;
			if (++level == 4)
				++x;   // as the original has it
			name.deleteChar(0);
		}

		if (c.knowsSpell(spell))
			window.PRINTSTR(x, y++, name);
	}

	// The heading sits at the lower right
	const Common::String &title = _messages.get(mage ? kMsgMageSpells : kMsgPriestSpells);
	const Common::String &press = _messages.get(kMsgPressReturn);
	window.PRINTSTR(window.width() - title.size(), 13, title);
	window.PRINTSTR(window.width() - press.size(), 14, press);
}

TextWindow TownScreens::templeWindow() {
	return TextWindow(_screen, Common::Rect(2, 4, 38, 12), true);
}

void TownScreens::drawTempleChoose(const Common::Array<uint> &patients) {
	TextWindow window = templeWindow();
	window.clear();
	window.printCenteredLine(_messages.get(kMsgTempleWelcome));
	window.printCenteredLine(_messages.get(kMsgTempleWho));
	window.printCenteredLine(_messages.get(kMsgTempleLeave));

	// The list lies over the lower part of the temple's window
	drawLettered(patients);
}

void TownScreens::drawTempleDonation(int64 amount) {
	TextWindow window = templeWindow();
	window.clear();
	window.PRINTSTR(_messages.get(kMsgDonation));
	window.PRINTSTR(Common::String::format("%lld", (long long)amount));
	window.newLine();
	window.newLine();
	window.printCenteredLine(_messages.get(kMsgWhoWillTithe));
	window.printCenteredLine(_messages.get(kMsgTitheLeave));
}

void TownScreens::drawTempleRitual(uint steps) {
	TextWindow window = templeWindow();
	window.clear();
	for (uint i = 0; i < steps && i < 4; ++i)
		window.PRINTSTR(_messages.get(kMsgMurmur + i));
}

void TownScreens::drawTempleSay(uint message, const Common::String &name) {
	TextWindow window = templeWindow();
	window.MVCURSOR(0, 5);
	window.printCenteredLine("* " + substCaret(_messages.get(message), name) + " *");
}

TextWindow TownScreens::shopWindow() {
	return TextWindow(_screen, Common::Rect(0, 2, 40, 16), true);
}

void TownScreens::drawShopWho() {
	TextWindow window = shopWindow();
	window.clear();
	window.printCenteredLine(_messages.get(kMsgShopTitle));
	window.newLine();
	window.printCenteredLine(_messages.get(kMsgShopWho));
	window.printCenteredLine(_messages.get(kMsgShopLeave));
}

void TownScreens::drawShopMenu(uint member, Menu &menu) {
	const Character &c = _party.member(member);
	TextWindow window = shopWindow();
	window.clear();

	window.PRINTSTR(substCaret(_messages.get(kMsgShopWelcome), c.name));
	window.newLine();
	window.PRINTSTR(_messages.get(kMsgShopYouHave));
	window.PRINTSTR(Common::String::format("%lld", (long long)c.gold));
	window.PRINTSTR(_messages.get(kMsgShopGold));
	window.newLine();
	window.newLine();
	menu.print(window, _messages.get(kMsgShopMenu));
}

void TownScreens::drawShopCatalog(uint member, const Shop &shop, Menu *menu) {
	const Character &c = _party.member(member);
	TextWindow window = shopWindow();
	window.clear();

	for (int line = 0; line < Shop::kCatalogLines; ++line) {
		const int id = shop._lines[line];
		window.MVCURSOR(0, line);
		window.printInt(line + 1, 1);
		window.PRINTCHR(')');
		window.PRINTSTR(_messages.get(ItemList::nameMessage(id, true)));
		window.MVCURSOR(18, line);
		printLong(window, _items[id].price);
		if (!_items[id].usableBy(c))
			window.PRINTSTR(_messages.get(kMsgUseless));
	}

	window.MVCURSOR(0, 7);
	window.PRINTSTR(Common::String::format("%lld", (long long)c.gold));
	window.PRINTSTR(_messages.get(kMsgGoldLeft));

	if (menu) {
		window.MVCURSOR(0, 10);
		menu->print(window, _messages.get(kMsgCatalogMenu));
	}
}

void TownScreens::drawShopPack(uint member, const Shop &shop, Shop::Service service, bool prompt) {
	const Character &c = _party.member(member);
	TextWindow window = shopWindow();
	window.clear();

	for (int i = 0; i < c.itemCount; ++i) {
		const ItemSlot &slot = c.items[i];
		window.MVCURSOR(0, i);
		window.printInt(i + 1, 1);
		window.PRINTCHR(')');

		// * equipped, - equipped and cursed, ? not identified, # of no use to this class
		if (slot.equipped)
			window.PRINTCHR(slot.cursed ? '-' : '*');
		else if (!slot.identified)
			window.PRINTCHR('?');
		else
			window.PRINTCHR(_items[slot.id].usableBy(c) ? ' ' : '#');

		window.PRINTSTR(_messages.get(ItemList::nameMessage(slot.id, slot.identified)));
		window.MVCURSOR(19, i);
		printLong(window, shop.servicePrice(service, slot));
	}

	if (prompt) {
		window.MVCURSOR(0, 9);
		window.PRINTSTR(_messages.get(kMsgSell + service));
		window.PRINTSTR(_messages.get(kMsgWhich));
	}
}

void TownScreens::drawShopLine(int row, const Common::String &text, bool centered, bool stars) {
	TextWindow window = shopWindow();
	window.CLRRECT(0, row, window.width(), row + 1);
	window.MVCURSOR(0, row);

	const Common::String line = stars ? "* " + text + " *" : text;
	if (centered)
		window.printCenteredLine(line);
	else
		window.PRINTSTR(line);
}

} // End of namespace Wizardry
