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

#ifndef WIZARDRY_VIEWS_TOWN_SHOP_H
#define WIZARDRY_VIEWS_TOWN_SHOP_H

#include "wizardry/views/town/town_view.h"

namespace Wizardry {
namespace Town {

/**
 * Boltac's Trading Post: buying from a catalogue of six lines, selling,
 * and having items uncursed or identified.
 */
class ShopView : public TownView {
private:
	enum State {
		kWho,           ///< which member enters
		kMenu,
		kCatalog,       ///< the catalogue and its menu
		kBuyWhich,      ///< which line to buy
		kBuyConfirm,    ///< an item the class cannot use: yes or no
		kPack,          ///< the pack: which item to sell, uncurse or identify
		kSellConfirm,   ///< an equipped item: yes or no
		kSaid           ///< something was said; a pause, then on
	};

	State _state = kWho;
	State _afterSaid = kMenu;
	uint _member = 0;
	Shop _shop;
	Shop::Service _service = Shop::kSell;
	int _chosen = 0;           ///< item record when buying, place in the pack otherwise
	Common::String _said;
	bool _saidStars = true;

	void say(uint message, State next, bool stars = true);
	void buy();
	void doService();
	void openPack(Shop::Service service);

protected:
	void drawTown(TownScreens &screens) override;
	void choose(int result, uint16 ascii) override;
	void timeout() override;

public:
	ShopView();

	bool msgFocus(const FocusMessage &msg) override;
};

} // End of namespace Town
} // End of namespace Wizardry

#endif
