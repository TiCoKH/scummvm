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

#include "wizardry/views/town/castle.h"
#include "wizardry/data/globals.h"

namespace Wizardry {
namespace Town {

enum CastleChoice {
	kTavern,
	kInn,
	kTemple,
	kTradingPost,
	kEdgeOfTown
};

void Castle::drawTown(TownScreens &screens) {
	screens.drawCastleMenu(_menu);
}

void Castle::choose(int result, uint16 ascii) {
	switch (result) {
	case kTavern:
		addView("Town.Tavern");
		break;

	case kInn:
	case kTemple:
	case kTradingPost:
		// These need a party, as in the original
		if (_G(party).size())
			notice("NOT IN THIS VERSION YET");   // TODO
		break;

	case kEdgeOfTown:
		notice("NOT IN THIS VERSION YET");       // TODO
		break;

	default:
		break;
	}
}

} // End of namespace Town
} // End of namespace Wizardry
