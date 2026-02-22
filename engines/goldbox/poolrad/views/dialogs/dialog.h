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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_DIALOG_H

#include "goldbox/poolrad/views/view.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class Dialog : public View {
protected:
	bool _isActive = false;
	bool _isVisible = true;

public:
	Dialog(const Common::String &name) : View(name) {}
	virtual ~Dialog() {}

	void activate() { _isActive = true; _isVisible = true; }
	void deactivate() { _isActive = false; _isVisible = false; }
    bool isActive() const { return _isActive; }
    bool isVisible() const { return _isVisible; }

	void show() { _isVisible = true; }
    void hide() { _isVisible = false; }

	void draw() override;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
