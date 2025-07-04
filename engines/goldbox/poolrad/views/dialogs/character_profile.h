/* ScummVM - Graphic Adventure Engine
 *
 * CharacterProfile dialog for displaying character details
 */
#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_CHARACTER_PROFILE_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_CHARACTER_PROFILE_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
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
    CharacterProfile(Goldbox::Poolrad::Data::PoolradCharacter *pc, const Common::String &name = "CharacterProfile");
    virtual ~CharacterProfile() {}

    void draw() override;

public:
    Goldbox::Poolrad::Data::PoolradCharacter *_poolradPc;

    void drawIdentity();
    void drawStats();
    void drawValuables();
    void drawLevelExp();
    void drawCombat();
    void drawItems();
    void drawStatus();
    void drawPortrait();
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
