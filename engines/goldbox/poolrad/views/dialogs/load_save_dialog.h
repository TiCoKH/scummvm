/* ScummVM - Graphic Adventure Engine
 *
 * Reusable Poolrad load/save dialog.
 */
#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_LOAD_SAVE_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_LOAD_SAVE_DIALOG_H

#include "common/array.h"
#include "common/str.h"
#include "goldbox/core/menu_item.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class LoadSaveDialog : public Dialog {
public:
	enum Mode {
		kModeLoad,
		kModeSave
	};

private:
	Mode _mode;
	Common::String _statusText;
	Common::String _titleText;
	Goldbox::MenuItemList _slotItems;
	Common::Array<uint8> _slotSelectionToIndex;
	HorizontalMenu *_slotMenu;

	void rebuildSlotMenu();
	bool slotExists(char slotLetter) const;
	static char toUpperAscii(char c);
	static Common::String slotName(char slotLetter);

public:
	LoadSaveDialog(const Common::String &name = "LoadSaveDialog");
	~LoadSaveDialog() override;

	void setMode(Mode mode);
	Mode getMode() const { return _mode; }
	void setStatusText(const Common::String &text);
	bool msgKeypress(const KeypressMessage &msg) override;

	void activate() override;
	void deactivate() override;
	void draw() override;
	void handleMenuResult(const MenuResultMessage &result) override;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_LOAD_SAVE_DIALOG_H
