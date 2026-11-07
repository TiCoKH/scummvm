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

#include "goldbox/poolrad/views/dialogs/spell_target_picker.h"
#include "goldbox/events.h"
#include "goldbox/poolrad/views/dialogs/party_selector.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

SpellTargetPicker::SpellTargetPicker(const Common::String &name)
		: Dialog(name), _selector(nullptr), _castTarget(nullptr),
		  _done(false), _picked(false), _partyIndex(-1) {
	PartySelectorConfig cfg;
	cfg.promptText = "Cast Spell on whom";
	cfg.allowExit = true;
	_selector = new PartySelector(name + "Selector", cfg);
	subView(_selector);
	_selector->deactivate();
}

SpellTargetPicker::~SpellTargetPicker() {
	if (_selector) {
		_selector->setParent(nullptr);
		delete _selector;
		_selector = nullptr;
	}
}

bool SpellTargetPicker::pickSingleTarget(
		const Common::Array<Goldbox::Data::PlayerCharacter *> &candidates,
		Goldbox::Data::PlayerCharacter *&picked) {
	picked = nullptr;
	if (!_selector || !g_events)
		return false;

	if (!_castTarget)
		_castTarget = VmInterface::getSelectedCharacter();

	// DisplayNoncombatInterface: the spell list leaves the screen first.
	if (g_events->focusedView())
		g_events->focusedView()->redraw();

	activate();
	_selector->setInitialCharacter(_castTarget);
	_selector->activate();
	_selector->redraw();

	_done = false;
	_picked = false;
	_partyIndex = -1;
	while (!_done)
		if (!g_events->pumpModalInputFrame())
			break;

	deactivate();

	// The selector's index counts the creature list, which is the pool order.
	if (_picked && _partyIndex >= 0 && _partyIndex < (int)candidates.size())
		picked = candidates[_partyIndex];

	_castTarget = picked;
	if (g_events->focusedView())
		g_events->focusedView()->redraw();
	return picked != nullptr;
}

void SpellTargetPicker::deactivate() {
	if (_selector)
		_selector->deactivate();
	Dialog::deactivate();
}

void SpellTargetPicker::draw() {
	if (isActive() && _selector)
		_selector->draw();
}

bool SpellTargetPicker::msgKeypress(const KeypressMessage &msg) {
	if (!isActive() || !_selector)
		return false;
	_selector->send(msg);
	return true;
}

void SpellTargetPicker::handleMenuResult(const MenuResultMessage &result) {
	_picked = result._success && result._hasIntValue;
	_partyIndex = result._hasIntValue ? result._intValue : -1;
	_done = true;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
