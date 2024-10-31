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
//#include "goldbox/keymapping.h"
#include "goldbox/poolrad/views/credits_view.h"


namespace Goldbox {
namespace Poolrad {
namespace Views {

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

CreditsView::CreditsView() : View("Credits"){}

bool CreditsView::msgKeypress(const KeypressMessage &msg) {
	replaceView("Codewheel");
	return true;
}

void CreditsView::draw() {
	Surface s = getSurface();

	drawWindow(1, 1, 38, 3);
	drawWindow(1, 5, 38, 23);
	s.writeStringC(SCENARIO_CREATED_BY, 10, 5, 1);
    s.writeStringC(TSR_INC, 14, 26, 1);
    s.writeStringC(JIM_WARD, 10, 15, 2);
    s.writeStringC(DAVID_COOK_STEVE_WINTER_MIKE_BREAULT, 10, 2, 3);
    s.writeStringC(GAME_CREATED_BY, 10, 1, 5);
    s.writeStringC(SSI_SPECIAL_PROJECTS, 14, 18, 5);
    s.writeStringC(VERSION_13, 11, 14, 6);
    s.writeStringC(PROGRAMMING, 14, 2, 7);
    s.writeStringC(ORIGINAL, 14, 20, 7);
    s.writeStringC(SCOT_BAYLESS, 11, 2, 8);
	s.writeStringC(PROGRAMMING, 14, 20, 8);
    s.writeStringC(BRAD_MYERS, 11, 2, 9);
    s.writeStringC(KEITH_BRORS, 11, 20, 9);
    s.writeStringC(RUSS_BROWN, 11, 2, 10);
	s.writeStringC(BRAD_MYERS, 11, 20, 10);
    s.writeStringC(TED_GREER, 11, 2, 11);
    s.writeStringC(GRAPHIC_ARTS, 14, 2, 13);
    s.writeStringC(ENCOUNTER_CODING, 14, 20, 13);
    s.writeStringC(TOM_WAHL, 11, 2, 14);
    s.writeStringC(PAUL_MURRAY, 11, 20, 14);
    s.writeStringC(FRED_BUTTS, 11, 2, 15);
	s.writeStringC(RUSS_BROWN, 11, 20, 15);
    s.writeStringC(DARLA_MARASCO, 11, 2, 16);
    s.writeStringC(VICTOR_PENMAN, 11, 20, 16);
    s.writeStringC(SUSAN_HALBLEIB, 11, 2, 17);
    s.writeStringC(DAVE_SHELLEY, 11, 20, 17);
    s.writeStringC(PROJECT_MANAGER, 14, 2, 19);
    s.writeStringC(DEVELOPER, 14, 20, 19);
	s.writeStringC(VICTOR_PENMAN, 11, 2, 20);
    s.writeStringC(GEORGE_MAC_DONALD, 11, 20, 20);
    s.writeStringC(TESTING, 14, 2, 22);
    s.writeStringC(JOEL_BILLINGS_STEVE_SALYER, 11, 11, 22);
    s.writeStringC(JAMES_KUCERA_ROBERT_DALY_RICK_WHITE, 11, 2, 23);
	delaySeconds(10);

}

bool CreditsView::msgFocus(const FocusMessage &msg) {
	return true;
}

bool CreditsView::msgUnfocus(const UnfocusMessage &msg) {
	return true;
}

void CreditsView::timeout() {
	replaceView("Codewheel");
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
