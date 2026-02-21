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

const unsigned char SCENARIO_CREATED_BY[] = "scenario created by:";
const unsigned char TSR_INC[] = "tsr, inc.";
const unsigned char JIM_WARD[] = "jim ward";
const unsigned char DAVID_COOK_STEVE_WINTER_MIKE_BREAULT[] = "david cook,steve winter,mike breault";
const unsigned char GAME_CREATED_BY[] = "game created by:";
const unsigned char SSI_SPECIAL_PROJECTS[] = "ssi special projects";
const unsigned char VERSION_13[] = "version 1.3";
const unsigned char PROGRAMMING[] = "programming:";
const unsigned char ORIGINAL[] = "original";
const unsigned char SCOT_BAYLESS[] = "scot bayless";
const unsigned char BRAD_MYERS[] = "brad myers";
const unsigned char KEITH_BRORS[] = "keith brors";
const unsigned char RUSS_BROWN[] = "russ brown";
const unsigned char TED_GREER[] = "ted greer";
const unsigned char GRAPHIC_ARTS[] = "graphic arts:";
const unsigned char ENCOUNTER_CODING[] = "encounter coding:";
const unsigned char TOM_WAHL[] = "tom wahl";
const unsigned char PAUL_MURRAY[] = "paul murray";
const unsigned char FRED_BUTTS[] = "fred butts";
const unsigned char DARLA_MARASCO[] = "darla marasco";
const unsigned char VICTOR_PENMAN[] = "victor penman";
const unsigned char SUSAN_HALBLEIB[] = "susan halbleib";
const unsigned char DAVE_SHELLEY[] = "dave shelley";
const unsigned char PROJECT_MANAGER[] = "project manager";
const unsigned char DEVELOPER[] = "developer:";
const unsigned char GEORGE_MAC_DONALD[] = "george mac donald";
const unsigned char TESTING[] = "testing:";
const unsigned char JOEL_BILLINGS_STEVE_SALYER[] = "joel billings,steve salyer";
const unsigned char JAMES_KUCERA_ROBERT_DALY_RICK_WHITE[] = "james kucera,robert daly,rick white";

CreditsView::CreditsView() : View("Credits"){}

bool CreditsView::msgKeypress(const KeypressMessage &msg) {
	replaceView("Codewheel");
	return true;
}

void CreditsView::draw() {
	Surface s = getSurface();

	drawWindow(1, 1, 38, 3);
	drawWindow(1, 5, 38, 23);
    s.writeStringC(5, 1, 10, SCENARIO_CREATED_BY);
    s.writeStringC(26, 1, 14, TSR_INC);
    s.writeStringC(15, 2, 10, JIM_WARD);
    s.writeStringC(2, 3, 10, DAVID_COOK_STEVE_WINTER_MIKE_BREAULT);
    s.writeStringC(1, 5, 10, GAME_CREATED_BY);
    s.writeStringC(18, 5, 14, SSI_SPECIAL_PROJECTS);
    s.writeStringC(14, 6, 11, VERSION_13);
    s.writeStringC(2, 7, 14, PROGRAMMING);
    s.writeStringC(20, 7, 14, ORIGINAL);
    s.writeStringC(2, 8, 11, SCOT_BAYLESS);
    s.writeStringC(20, 8, 14, PROGRAMMING);
    s.writeStringC(2, 9, 11, BRAD_MYERS);
    s.writeStringC(20, 9, 11, KEITH_BRORS);
    s.writeStringC(2, 10, 11, RUSS_BROWN);
    s.writeStringC(20, 10, 11, BRAD_MYERS);
    s.writeStringC(2, 11, 11, TED_GREER);
    s.writeStringC(2, 13, 14, GRAPHIC_ARTS);
    s.writeStringC(20, 13, 14, ENCOUNTER_CODING);
    s.writeStringC(2, 14, 11, TOM_WAHL);
    s.writeStringC(20, 14, 11, PAUL_MURRAY);
    s.writeStringC(2, 15, 11, FRED_BUTTS);
    s.writeStringC(20, 15, 11, RUSS_BROWN);
    s.writeStringC(2, 16, 11, DARLA_MARASCO);
    s.writeStringC(20, 16, 11, VICTOR_PENMAN);
    s.writeStringC(2, 17, 11, SUSAN_HALBLEIB);
    s.writeStringC(20, 17, 11, DAVE_SHELLEY);
    s.writeStringC(2, 19, 14, PROJECT_MANAGER);
    s.writeStringC(20, 19, 14, DEVELOPER);
    s.writeStringC(2, 20, 11, VICTOR_PENMAN);
    s.writeStringC(20, 20, 11, GEORGE_MAC_DONALD);
    s.writeStringC(2, 22, 14, TESTING);
    s.writeStringC(11, 22, 11, JOEL_BILLINGS_STEVE_SALYER);
    s.writeStringC(2, 23, 11, JAMES_KUCERA_ROBERT_DALY_RICK_WHITE);
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
