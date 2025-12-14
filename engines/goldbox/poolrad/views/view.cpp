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

#include "goldbox/poolrad/views/view.h"
#include "goldbox/engine.h"


namespace Goldbox {
namespace Poolrad {
namespace Views {

void View::checkFocusedControl(const Common::Point &mousePos) {
	if (_focusedElement) {
		if (!_focusedElement->getBounds().contains(mousePos)) {
			_focusedElement->send(UnfocusMessage());
			_focusedElement = nullptr;
		}

	} else {
		for (UIElement *child : _children) {
			if (child->getBounds().contains(mousePos)) {
				_focusedElement = child;
				child->send(FocusMessage());
				break;
			}
		}
	}
}

UIElement *View::getElementAtPos(const Common::Point &pos) const {
	for (UIElement *child : _children) {
		if (child->getBounds().contains(pos))
			return child;
	}

	return nullptr;
}

void View::drawFrame(const Common::Rect &r, uint32 bgColor) {
	Surface s = getSurface();

	s.writeSymbol(20, r.left, r.top, bgColor);
	for (int x = r.left + 1; x <= r.right - 1; x++) {
		s.writeSymbol(22, bgColor);
	}
	s.writeSymbol(20, bgColor);
	for (int y = r.top + 1; y <= r.bottom - 1; y++) {
		s.writeSymbol(21, r.left, y, bgColor);
		s.writeSymbol(21, r.right, y, bgColor);
	}
	s.writeSymbol(20, r.left, r.bottom, bgColor);
	for (int x = r.left + 1; x <= r.right - 1; x++)
		s.writeSymbol(22, bgColor);
	s.writeSymbol(20, bgColor);
}

void View::drawWindow(uint8 left, uint8 top, uint8 right, uint8 bottom) {
	drawFrame(Common::Rect(left - 1, top - 1, right + 1, bottom + 1));
	Surface s = getSurface();
	s.clearBox(left, top, right, bottom, 0);
}

bool View::msgFocus(const FocusMessage &msg) {
	_focusedElement = nullptr;
	return UIElement::msgFocus(msg);
}

bool View::msgUnfocus(const UnfocusMessage &msg) {
	if (_focusedElement)
		_focusedElement->send(UnfocusMessage());

	return UIElement::msgUnfocus(msg);
}

bool View::msgMouseMove(const MouseMoveMessage &msg) {
	checkFocusedControl(msg._pos);
	return true;
}

bool View::msgMouseDown(const MouseDownMessage &msg) {
	UIElement *child = getElementAtPos(msg._pos);
	return child ? child->send(msg) : false;
}

bool View::msgMouseUp(const MouseUpMessage &msg) {
	UIElement *child = getElementAtPos(msg._pos);
	return child ? child->send(msg) : false;
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
