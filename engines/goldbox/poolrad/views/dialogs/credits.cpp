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

#include "common/system.h"
#include "graphics/palette.h"
#include "goldbox/poolrad/views/dialogs/credits.h"
//#include "wasteland/core/file.h"
//#include "wasteland/wasteland1/files/vertical_xor_stream.h"
#include "goldbox/keymapping.h"
//#include "wasteland/wasteland1/core/text_decoder.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

#define TITLE_W 288
#define TITLE_H 128

const char SCENARIO_CREATED_BY[] = "scenario created by:";
const char TSR_INC[] = "tsr, inc.";
const char JIM_WARD[] = "jim ward";
const char DAVID_COOK_STEVE_WINTER_MIKE_BREAULT[] = "david cook,steve winter,mike breault";
const char GAME_CREATED_BY[] = "game created by:";
const char SSI_SPECIAL_PROJECTS[] = "ssi special projects";
const char VERSION_13[] = "version 1.3";
const char PROGRAMMING[] = "programming:";
const char ORIGINAL[] = "original";
const char SCOT_BAYLESS[] = "scot bayless";
const char BRAD_MYERS[] = "brad myers";
const char KEITH_BRORS[] = "keith brors";
const char RUSS_BROWN[] = "russ brown";
const char TED_GREER[] = "ted greer";
const char GRAPHIC_ARTS[] = "graphic arts:";
const char ENCOUNTER_CODING[] = "encounter coding:";
const char TOM_WAHL[] = "tom wahl";
const char PAUL_MURRAY[] = "paul murray";
const char FRED_BUTTS[] = "fred butts";
const char DARLA_MARASCO[] = "darla marasco";
const char VICTOR_PENMAN[] = "victor penman";
const char SUSAN_HALBLEIB[] = "susan halbleib";
const char DAVE_SHELLEY[] = "dave shelley";
const char PROJECT_MANAGER[] = "project manager";
const char DEVELOPER[] = "developer:";
const char GEORGE_MAC_DONALD[] = "george mac donald";
const char TESTING[] = "testing:";
const char JOEL_BILLINGS_STEVE_SALYER[] = "joel billings,steve salyer";
const char JAMES_KUCERA_ROBERT_DALY_RICK_WHITE[] = "james kucera,robert daly,rick white";

Credits::Credits() : Dialog("Credits"){}

bool Credits::msgAction(const ActionMessage &msg) {
//	if (msg._action == KEYBIND_SELECT)
//		replaceView("Roster");
	return true;
}

void Credits::draw() {
	Surface s = getSurface();
	s.clear();

	drawWindow(1, 1, 38, 3);
	drawWindow(1, 5, 38, 23);
	s.writeStringC(SCENARIO_CREATED_BY, 10, 1, 5);
    s.writeStringC(TSR_INC, 14, 1, 26);
    s.writeStringC(JIM_WARD, 10, 2, 15);
    s.writeStringC(DAVID_COOK_STEVE_WINTER_MIKE_BREAULT, 10, 3, 2);
    s.writeStringC(GAME_CREATED_BY, 10, 5, 1);
    s.writeStringC(SSI_SPECIAL_PROJECTS, 14, 5, 18);
    s.writeStringC(VERSION_13, 11, 6, 14);
    s.writeStringC(PROGRAMMING, 14, 7, 2);
    s.writeStringC(ORIGINAL, 14, 7, 20);
    s.writeStringC(SCOT_BAYLESS, 11, 8, 2);
	s.writeStringC(PROGRAMMING, 14, 8, 20);
    s.writeStringC(BRAD_MYERS, 11, 9, 2);
    s.writeStringC(KEITH_BRORS, 11, 9, 20);
    s.writeStringC(RUSS_BROWN, 11, 10, 2);
	s.writeStringC(BRAD_MYERS, 11, 10, 20);
    s.writeStringC(TED_GREER, 11, 11, 2);
    s.writeStringC(GRAPHIC_ARTS, 14, 13, 2);
    s.writeStringC(ENCOUNTER_CODING, 14, 13, 20);
    s.writeStringC(TOM_WAHL, 11, 14, 2);
    s.writeStringC(PAUL_MURRAY, 11, 14, 20);
    s.writeStringC(FRED_BUTTS, 11, 15, 2);
	s.writeStringC(RUSS_BROWN, 11, 15, 20);
    s.writeStringC(DARLA_MARASCO, 11, 16, 2);
    s.writeStringC(VICTOR_PENMAN, 11, 16, 20);
    s.writeStringC(SUSAN_HALBLEIB, 11, 17, 2);
    s.writeStringC(DAVE_SHELLEY, 11, 17, 20);
    s.writeStringC(PROJECT_MANAGER, 14, 19, 2);
    s.writeStringC(DEVELOPER, 14, 19, 20);
	s.writeStringC(VICTOR_PENMAN, 11, 20, 2);
    s.writeStringC(GEORGE_MAC_DONALD, 11, 20, 20);
    s.writeStringC(TESTING, 14, 22, 2);
    s.writeStringC(JOEL_BILLINGS_STEVE_SALYER, 11, 22, 11);
    s.writeStringC(JAMES_KUCERA_ROBERT_DALY_RICK_WHITE, 11, 23, 2);

}

bool Credits::msgFocus(const FocusMessage &msg) {
	Dialog::msgFocus(msg);
	return true;
}

bool Credits::msgUnfocus(const UnfocusMessage &msg) {
	return true;
}

void Credits::timeout() {
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
