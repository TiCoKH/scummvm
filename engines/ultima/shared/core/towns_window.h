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


#ifndef ULTIMA_SHARED_CORE_TOWNSWINDOW_H
#define ULTIMA_SHARED_CORE_TOWNSWINDOW_H

#include "common/rect.h"

namespace Ultima {
namespace Shared {

class TownsWindow;

/**
 * Extended derived rect class support FMTowns Ultima games window data
 */
class TownsWindow : public Common::Rect {
public:
	Common::Rect _rect;
	int _maxChars, _maxLines;
	int _xIdent, _yIdent;
	int16 _textColor, _borderColor, _backColor;
	int _textStyle;
	bool _isReady, _withBackground, _something;

	TownsWindow() : Common::Rect() {}

	TownsWindow(int16 x1, int16 y1, int16 x_size, int16 y_size, int16 max_c, int16 max_l, int16 text_c, int16 border_c, int16 back_c) {
		_textColor = text_c;
		_borderColor = border_c;
		_backColor = back_c;
		if(((x_size-6) / 6) < max_c) _maxChars = (x_size-6) / 6;
		else _maxChars = max_c;
		if(((y_size-4) / 16) < max_l) _maxLines = (y_size-4) / 16;
		else _maxLines = max_l;
		_rect = Rect(x1, y1, x1+x_size, y1+y_size);
		_isReady = true;
		_withBackground = true;
		_something = true;
		_textStyle = 0;
	}
};

} // End of namespace Shared
} // End of namespace Ultima

#endif

