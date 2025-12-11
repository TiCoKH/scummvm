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

#ifndef GOLDBOX_POOLRAD_VIEWS_TITLE_VIEW_H
#define GOLDBOX_POOLRAD_VIEWS_TITLE_VIEW_H

#include "goldbox/gfx/pic.h"
#include "goldbox/poolrad/views/view.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

class TitleView : public View {
private:
	Goldbox::Gfx::Pic *_pic1 = nullptr;
	Goldbox::Gfx::Pic *_pic2 = nullptr;
	uint8 _state = 0;

public:
	TitleView() : View("Title") {}
	virtual ~TitleView() {}

	bool msgFocus(const FocusMessage &msg) override;
	bool msgKeypress(const KeypressMessage &msg) override;
	void draw() override;
	void timeout() override;
};
 
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
