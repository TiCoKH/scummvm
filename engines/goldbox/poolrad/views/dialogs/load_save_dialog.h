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
