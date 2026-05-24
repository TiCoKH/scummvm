/* ScummVM - Graphic Adventure Engine
 *
 * Reusable Poolrad load/save dialog.
 */

#include "common/file.h"
#include "common/fs.h"
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

static int keyToSlotIndex(const KeypressMessage &msg) {
	const Common::KeyCode key = msg.keycode;
	if (key >= Common::KEYCODE_a && key <= Common::KEYCODE_j)
		return (int)(key - Common::KEYCODE_a);

	char ascii = msg.ascii;
	if (ascii >= 'a' && ascii <= 'z')
		ascii = static_cast<char>(ascii - ('a' - 'A'));
	if (ascii >= 'A' && ascii <= 'J')
		return (int)(ascii - 'A');

	return -1;
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
	Common::String foundLoadSlots;

	for (int i = 0; i < 10; ++i) {
		const char slotLetter = static_cast<char>('A' + i);
		const bool used = slotExists(slotLetter);
		if (_mode == kModeLoad && used) {
			if (!foundLoadSlots.empty())
				foundLoadSlots += ",";
			foundLoadSlots += slotLetter;
		}
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

	if (_mode == kModeLoad) {
		debug("LoadSaveDialog: loadable legacy slots [%s]",
			foundLoadSlots.empty() ? "none" : foundLoadSlots.c_str());
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

	debug("LoadSaveDialog::msgKeypress key=%d ascii=%d active=%d slotMenuActive=%d",
		(int)msg.keycode, (int)msg.ascii, _isActive ? 1 : 0,
		(_slotMenu && _slotMenu->isActive()) ? 1 : 0);

	if (msg.keycode == Common::KEYCODE_ESCAPE) {
		deactivate();
		const Common::String targetViewName = _parent ? _parent->getName() : Common::String();
		g_events->postMenuResult(targetViewName, false,
			msg.keycode, 0, Common::String(), true, false);
		return true;
	}

	// Handle direct slot hotkeys here (A..J) to avoid backend-specific
	// ascii/keycode translation issues in submenus.
	const int slotIndex = keyToSlotIndex(msg);
	if (slotIndex >= 0) {
		int selectionIndex = -1;
		for (int i = 0; i < (int)_slotSelectionToIndex.size(); ++i) {
			if (_slotSelectionToIndex[i] == slotIndex) {
				selectionIndex = i;
				break;
			}
		}

		debug("LoadSaveDialog::msgKeypress slot hotkey %c -> slotIndex=%d selectionIndex=%d",
			(char)('A' + slotIndex), slotIndex, selectionIndex);

		if (selectionIndex >= 0) {
			// Reuse the same path as submenu selection events: selection index in
			// dialog-space is remapped to slot index in handleMenuResult().
			handleMenuResult(MenuResultMessage(getName(), true,
				msg.keycode, selectionIndex));
		}
		return true;
	}

	if (!_slotMenu || !_slotMenu->isActive())
		return true;

	return _slotMenu->msgKeypress(msg);
}

bool LoadSaveDialog::slotExists(char slotLetter) const {
	const Common::Path savePath = getSavePath();
	if (savePath.empty())
		return false;

	const Common::Path upperPath = savePath / slotName(slotLetter);
	Common::FSNode upperNode(upperPath);
	if (upperNode.exists() && !upperNode.isDirectory())
		return true;

	// Be tolerant on case-sensitive filesystems.
	const Common::Path lowerPath =
		savePath / Common::String::format("savgam%c.dat", toUpperAscii(slotLetter));
	Common::FSNode lowerNode(lowerPath);
	return lowerNode.exists() && !lowerNode.isDirectory();
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
	const Common::String targetViewName = _parent ? _parent->getName() : Common::String();

	debug("LoadSaveDialog::handleMenuResult success=%d key=%d hasInt=%d int=%d",
		result._success ? 1 : 0, (int)result._keyCode,
		result._hasIntValue ? 1 : 0, result._hasIntValue ? result._intValue : -1);

	if (!result._success) {
		g_events->postMenuResult(targetViewName, false,
			result._keyCode, 0, Common::String(), true, false);
		return;
	}

	const int selectionIndex = result._hasIntValue ? result._intValue : -1;
	if (selectionIndex < 0 || selectionIndex >= (int)_slotSelectionToIndex.size()) {
		g_events->postMenuResult(targetViewName, false,
			result._keyCode, 0, Common::String(), true, false);
		return;
	}

	const int slotIndex = _slotSelectionToIndex[selectionIndex];
	debug("LoadSaveDialog::handleMenuResult selectionIndex=%d -> slotIndex=%d (%c)",
		selectionIndex, slotIndex, (char)('A' + slotIndex));

	Poolrad::PoolradEngine *engine = Poolrad::g_engine;
	if (!engine) {
		setStatusText((_mode == kModeSave) ?
			"Save failed: engine unavailable" :
			"Load failed: engine unavailable");
		g_events->postMenuResult(targetViewName, false, result._keyCode,
			slotIndex, Common::String(), true, false);
		return;
	}

	if (Poolrad::g_engine)
		Poolrad::g_engine->setLegacyMenuStatus(static_cast<uint8>(slotIndex));

	const char slotLetter = static_cast<char>('A' + slotIndex);
	Common::String errorMessage;
	bool ok = false;

	if (_mode == kModeSave) {
		debug("LoadSaveDialog::handleMenuResult calling saveGameSlotX86(%c)",
			slotLetter);
		ok = engine->saveGameSlotX86(slotLetter, errorMessage);
	} else {
		debug("LoadSaveDialog::handleMenuResult calling loadGameSlotX86(%c)",
			slotLetter);
		ok = engine->loadGameSlotX86(slotLetter, errorMessage);
	}

	if (!ok) {
		warning("%s failed for slot %c: %s",
			(_mode == kModeSave) ? "Save" : "Load",
			slotLetter, errorMessage.c_str());
		setStatusText(Common::String::format("%s failed: %s",
			(_mode == kModeSave) ? "Save" : "Load", errorMessage.c_str()));
		if (_slotMenu && !_slotMenu->isActive())
			_slotMenu->activate();
		redraw();
		g_events->postMenuResult(targetViewName, false, result._keyCode,
			slotIndex, Common::String(), true, false);
		return;
	}

	if (_slotMenu && _slotMenu->isActive())
		_slotMenu->deactivate();
	deactivate();

	g_events->postMenuResult(targetViewName, true, result._keyCode,
		slotIndex, Common::String(), true, false);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
