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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_CHARACTER_PROFILE_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_CHARACTER_PROFILE_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/portrait_display.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Data {
    class PlayerCharacter;
}
namespace Poolrad {
namespace Data {
    class PoolradCharacter;
}
namespace Views {
namespace Dialogs {

class CharacterProfile : public Dialog {
public:
    Goldbox::Poolrad::Data::PoolradCharacter *_poolradPc;
    Goldbox::Poolrad::Views::Dialogs::PortraitDisplay _portraitDisplay;

    CharacterProfile(const Common::String &name = "CharacterProfile");
    virtual ~CharacterProfile() {}

    void activate() override;
    void draw() override;
    void handleMenuResult(const MenuResultMessage &result) override;

    void redrawStats();
    void redrawValuables();
    void redrawCombat();
    void redrawName();
    void setHighlightedStat(int index);
    void drawIdentity();
    void drawName();
    void drawStats();
    void drawValuables();
    void drawLevelExp();
    void drawCombat();
    void drawItems();
    void drawStatus();
    void drawPortrait();
   // void loadPortraitGraphics();

private:
    // Head DAX block IDs: portrait head ID (1-14) -> DAX block ID
    static const uint8 kHeadDaxBlockIds[14];
    // Body DAX block IDs: portrait body ID (1-12) -> DAX block ID
    static const uint8 kBodyDaxBlockIds[12];
    int _highlightedStat;

    /**
     * Format damage text in "XdY" or "XdY+Z" format.
     * @return Formatted damage string from current primary weapon damage values.
     */
    Common::String formatDamageText() const;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
