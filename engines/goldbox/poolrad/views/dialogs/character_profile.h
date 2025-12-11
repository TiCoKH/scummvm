/* ScummVM - Graphic Adventure Engine
 *
 * CharacterProfile dialog for displaying character details
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

    CharacterProfile(Goldbox::Poolrad::Data::PoolradCharacter *pc, const Common::String &name = "CharacterProfile");
    virtual ~CharacterProfile() {}

    void draw() override;
    void handleMenuResult(bool success, Common::KeyCode key, short value) override;

    // Partial redraw helpers: clear only the relevant area and redraw section
    void redrawStats();
    void redrawValuables();
    void redrawCombat();
    void redrawName();
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
