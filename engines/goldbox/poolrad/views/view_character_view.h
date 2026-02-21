/* ScummVM - Graphic Adventure Engine
 *
 * ViewCharacterView - Character view with horizontal menu
 */

#ifndef GOLDBOX_POOLRAD_VIEWS_VIEWCHARACTERVIEW_H
#define GOLDBOX_POOLRAD_VIEWS_VIEWCHARACTERVIEW_H

#include "goldbox/poolrad/views/view.h"
#include "goldbox/data/player_character.h"
#include "common/array.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/character_profile.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

namespace Dialogs {
class ItemsMenu;
}

class ViewCharacterView : public View {
public:
	ViewCharacterView();
    ViewCharacterView(Goldbox::Poolrad::Data::PoolradCharacter *character);
    ~ViewCharacterView() override;

    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;

private:
    Goldbox::Poolrad::Data::PoolradCharacter *_character;
    Goldbox::MenuItemList _menuList;
    Dialogs::HorizontalMenu *_horizontalMenu;
    Dialogs::CharacterProfile *_profileDialog;
    Dialogs::ItemsMenu *_itemsMenu;

    void buildMenu();
    void drawMenu();
    void handleMenuResult(bool accepted, Common::KeyCode keyCode, int index);

    // Action handlers
    void handleViewItems();
    void handleViewSpells();
    void handleTradeValuables();
    void handleDropValuables();
    void handleRenameCharacter();
    void handleExit();
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
