/* ScummVM - Graphic Adventure Engine
 *
 * Reusable Poolrad load/save dialog.
 */

#include "common/file.h"
#include "common/path.h"
#include "goldbox/events.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/poolrad/views/dialogs/load_save_dialog.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

namespace {

static Common::Path getSavePath() {
	if (Poolrad::g_engine)
		return Poolrad::g_engine->resolveSavePath();

	warning("Poolrad engine unavailable while resolving legacy save path");
	return Common::Path();
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
	setBounds(Window(0, 24, 39, 24));
	setStatusText("Load Which Game:");
	rebuildSlotMenu();
}

LoadSaveDialog::~LoadSaveDialog() {
	if (_slotMenu) {
		_slotMenu->setParent(nullptr);
		delete _slotMenu;
		_slotMenu = nullptr;
	}
}

void LoadSaveDialog::setMode(Mode mode) {
	_mode = mode;
	_titleText = (_mode == kModeSave) ? "Save Game" : "Load Game";
	setStatusText((_mode == kModeSave) ? "Save Which Game:" : "Load Which Game:");
	rebuildSlotMenu();
}

void LoadSaveDialog::setStatusText(const Common::String &text) {
	_statusText = text;
	redraw();
}

void LoadSaveDialog::rebuildSlotMenu() {
	_slotItems.items.clear();
	_slotItems.currentSelection = 0;
	_slotSelectionToIndex.clear();

	for (int i = 0; i < 10; ++i) {
		const char slotLetter = static_cast<char>('A' + i);
		const bool used = slotExists(slotLetter);
		if (_mode == kModeLoad && !used)
			continue;

		MenuItem item;
		item.shortcut = slotLetter;
		item.text = Common::String();
		item.active = true;
		item.shortcutFirst = true;
		_slotItems.items.push_back(item);
		_slotSelectionToIndex.push_back(static_cast<uint8>(i));
	}

	if (_slotMenu) {
		_slotMenu->setParent(nullptr);
		delete _slotMenu;
		_slotMenu = nullptr;
	}

	if (_slotItems.items.empty()) {
		if (_mode == kModeLoad)
			setStatusText("Load Which Game: none");
		return;
	}

	HorizontalMenuConfig cfg;
	cfg.promptTxt = _statusText + " ";
	cfg.menuItemList = &_slotItems;
	cfg.textColor = 10;
	cfg.selectColor = 15;
	cfg.promptColor = 13;
	cfg.allowNumPad = false;
	cfg.suppressUnhandledKeys = true;
	cfg.backgroundColor = 0;

	_slotMenu = new HorizontalMenu(getName() + "_Slots", cfg);
	subView(_slotMenu);

	if (isActive())
		_slotMenu->activate();
}

bool LoadSaveDialog::msgKeypress(const KeypressMessage &msg) {
	if (!_isActive)
		return false;

	if (!_slotMenu || !_slotMenu->isActive())
		return false;

	return _slotMenu->msgKeypress(msg);
}

bool LoadSaveDialog::slotExists(char slotLetter) const {
	const Common::Path savePath = getSavePath();
	Common::File file;
	return file.open(savePath / slotName(slotLetter));
}

void LoadSaveDialog::activate() {
	Dialog::activate();
	if (_slotMenu)
		_slotMenu->activate();
	redraw();
}

void LoadSaveDialog::deactivate() {
	if (_slotMenu && _slotMenu->isActive())
		_slotMenu->deactivate();
	Dialog::deactivate();
}

void LoadSaveDialog::draw() {
	if (!isVisible())
		return;

	if (_slotMenu && _slotMenu->isActive()) {
		_slotMenu->draw();
		return;
	}

	Surface s = getSurface();
	s.clearBox(0, 0, 39, 0, 0);
	s.writeStringC(0, 0, 13, _statusText);
}

void LoadSaveDialog::handleMenuResult(const MenuResultMessage &result) {
	if (!_parent)
		return;

	if (!result._success) {
		g_events->postMenuResult(_parent->getName(), false,
			result._keyCode, 0, Common::String(), true, false);
		return;
	}

	const int selectionIndex = result._hasIntValue ? result._intValue : -1;
	if (selectionIndex < 0 || selectionIndex >= (int)_slotSelectionToIndex.size()) {
		g_events->postMenuResult(_parent->getName(), false,
			result._keyCode, 0, Common::String(), true, false);
		return;
	}

	const int slotIndex = _slotSelectionToIndex[selectionIndex];
	if (Poolrad::g_engine)
		Poolrad::g_engine->setLegacyMenuStatus(static_cast<uint8>(slotIndex));

	g_events->postMenuResult(_parent->getName(), true, result._keyCode,
		slotIndex, Common::String(), true, false);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
