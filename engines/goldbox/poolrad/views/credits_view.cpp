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
#include "common/config-manager.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {


static Common::String cr(const char *key) {
	return VmInterface::getString(Common::String("credits.") + key);
}

const char *viewAfterCredits() {
	const bool skip = ConfMan.hasKey("skip_codewheel")
		&& ConfMan.getBool("skip_codewheel");
	return skip ? "Mainmenu" : "Codewheel";
}

CreditsView::CreditsView() : View("Credits"){}

bool CreditsView::msgKeypress(const KeypressMessage &msg) {
	replaceView(viewAfterCredits());
	return true;
}

void CreditsView::draw() {
	Surface s = getSurface();

	drawWindow(1, 1, 38, 3);
	drawWindow(1, 5, 38, 23);
    s.writeStringC(5, 1, 10, cr("scenario"));
    s.writeStringC(26, 1, 14, cr("tsr"));
    s.writeStringC(15, 2, 10, cr("jimward"));
    s.writeStringC(2, 3, 10, cr("scenarioteam"));
    s.writeStringC(1, 5, 10, cr("game"));
    s.writeStringC(18, 5, 14, cr("ssi"));
    s.writeStringC(14, 6, 11, cr("version"));
    s.writeStringC(2, 7, 14, cr("programming"));
    s.writeStringC(20, 7, 14, cr("original"));
    s.writeStringC(2, 8, 11, cr("scotbayless"));
    s.writeStringC(20, 8, 14, cr("programming"));
    s.writeStringC(2, 9, 11, cr("bradmyers"));
    s.writeStringC(20, 9, 11, cr("keithbrors"));
    s.writeStringC(2, 10, 11, cr("russbrown"));
    s.writeStringC(20, 10, 11, cr("bradmyers"));
    s.writeStringC(2, 11, 11, cr("tedgreer"));
    s.writeStringC(2, 13, 14, cr("graphicarts"));
    s.writeStringC(20, 13, 14, cr("encountercoding"));
    s.writeStringC(2, 14, 11, cr("tomwahl"));
    s.writeStringC(20, 14, 11, cr("paulmurray"));
    s.writeStringC(2, 15, 11, cr("fredbutts"));
    s.writeStringC(20, 15, 11, cr("russbrown"));
    s.writeStringC(2, 16, 11, cr("darlamarasco"));
    s.writeStringC(20, 16, 11, cr("victorpenman"));
    s.writeStringC(2, 17, 11, cr("susanhalbleib"));
    s.writeStringC(20, 17, 11, cr("daveshelley"));
    s.writeStringC(2, 19, 14, cr("projectmanager"));
    s.writeStringC(20, 19, 14, cr("developer"));
    s.writeStringC(2, 20, 11, cr("victorpenman"));
    s.writeStringC(20, 20, 11, cr("georgemacdonald"));
    s.writeStringC(2, 22, 14, cr("testing"));
    s.writeStringC(11, 22, 11, cr("testers1"));
    s.writeStringC(2, 23, 11, cr("testers2"));
	delaySeconds(10);

}

bool CreditsView::msgFocus(const FocusMessage &msg) {
	return true;
}

bool CreditsView::msgUnfocus(const UnfocusMessage &msg) {
	return true;
}

void CreditsView::timeout() {
	replaceView(viewAfterCredits());
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
