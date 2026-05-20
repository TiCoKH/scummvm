/* ScummVM - Graphic Adventure Engine
 *
 * Reusable Poolrad load/save dialog.
 */

#include "common/config-manager.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/path.h"
#include "goldbox/events.h"
#include "goldbox/poolrad/views/dialogs/load_save_dialog.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

namespace {

static Common::Path getSavePath() {
	Common::Path savePath = ConfMan.getPath("savepath");
	if (savePath.empty())
		savePath = ConfMan.getPath("currentpath");
	return savePath;
}

} // namespace

char LoadSaveDialog::toUpperAscii(char c) {
	if (c >= 'a' && c <= 'z')
		return static_cast<char>(c - ('a' - 'A'));
	return c;
}

Common::String LoadSaveDialog::slotName(char slotLetter) {
	return Common::String::format("SAVGAM%c.DAT", toUpperAscii(slotLetter));
}

LoadSaveDialog::LoadSaveDialog(const Common::String &name)
	: Dialog(name), _mode(kModeLoad), _statusText(), _titleText("Load Game"),
	  _slotMenu(nullptr) {
	setBounds(Window(0, 0, 39, 24));

	VerticalMenuConfig cfg;
	cfg.promptTxt = Common::String();
	cfg.promptOptions = nullptr;
	cfg.menuItemList = &_slotItems;
	cfg.headColor = 13;
	cfg.textColor = 10;
	cfg.selectColor = 15;
	cfg.xStart = 1;
	cfg.yStart = 2;
	cfg.xEnd = 38;
	cfg.yEnd = 22;
	cfg.title = _titleText;
	cfg.asHeader = true;

	_slotMenu = new VerticalMenu(name + "_Slots", cfg);
	subView(_slotMenu);

	rebuildSlotList();
}

LoadSaveDialog::~LoadSaveDialog() {
	if (_slotMenu) {
		delete _slotMenu;
		_slotMenu = nullptr;
	}
}

void LoadSaveDialog::setMode(Mode mode) {
	_mode = mode;
	_titleText = (_mode == kModeSave) ? "Save Game" : "Load Game";
	setStatusText((_mode == kModeSave)
		? "Select a slot to save"
		: "Select a slot to load");
	rebuildSlotList();
}

void LoadSaveDialog::setStatusText(const Common::String &text) {
	_statusText = text;
	redraw();
}

bool LoadSaveDialog::handleKeypressDirect(const KeypressMessage &msg) {
	if (msg.keycode < Common::KEYCODE_a || msg.keycode > Common::KEYCODE_j)
		return false;

	const int slotIndex = msg.keycode - Common::KEYCODE_a;
	const char slotLetter = static_cast<char>('A' + slotIndex);

	if (_mode == kModeLoad && !slotExists(slotLetter)) {
		setStatusText(Common::String::format("Slot %c is empty", slotLetter));
		return true;
	}

	if (_parent) {
		g_events->postMenuResult(_parent->getName(), true, msg.keycode,
			slotIndex, Common::String(), true, false);
		return true;
	}

	return false;
}

bool LoadSaveDialog::slotExists(char slotLetter) const {
	const Common::Path savePath = getSavePath();
	Common::File file;
	return file.open(savePath / slotName(slotLetter));
}

void LoadSaveDialog::rebuildSlotList() {
	_slotLabels.clear();
	_slotItems.items.clear();
	_slotItems.currentSelection = 0;

	for (int i = 0; i < 10; ++i) {
		const char slotLetter = static_cast<char>('A' + i);
		const bool isUsed = slotExists(slotLetter);
		Common::String label = Common::String::format("%c - %s",
			slotLetter, isUsed ? "Used" : "Empty");
		_slotLabels.push_back(label);

		MenuItem item;
		item.shortcut = slotLetter;
		item.text = label;
		item.active = (_mode == kModeSave) || isUsed;
		item.shortcutFirst = true;
		_slotItems.items.push_back(item);
	}

	if (_slotMenu) {
		_slotMenu->rebuild(&_slotItems, _titleText);
	}
}

void LoadSaveDialog::activate() {
	Dialog::activate();
	rebuildSlotList();
	if (_slotMenu)
		_slotMenu->activate();
}

void LoadSaveDialog::deactivate() {
	if (_slotMenu)
		_slotMenu->deactivate();
	Dialog::deactivate();
}

void LoadSaveDialog::draw() {
	if (!isVisible())
		return;

	Surface s = getSurface();
	drawWindow(1, 1, 38, 22);
	if (_slotMenu)
		_slotMenu->draw();

	s.clearBox(0, 23, 39, 24, 0);
	s.writeStringC(0, 23, 13, _statusText);
}

void LoadSaveDialog::handleMenuResult(const MenuResultMessage &result) {
	if (!_parent)
		return;

	if (!result._success) {
		g_events->postMenuResult(_parent->getName(), false,
			result._keyCode, 0, Common::String(), true, false);
		return;
	}

	const int slotIndex = result._hasIntValue ? result._intValue : -1;
	if (slotIndex < 0 || slotIndex >= 10) {
		g_events->postMenuResult(_parent->getName(), false,
			result._keyCode, 0, Common::String(), true, false);
		return;
	}

	g_events->postMenuResult(_parent->getName(), true, result._keyCode,
		slotIndex, Common::String(), true, false);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
