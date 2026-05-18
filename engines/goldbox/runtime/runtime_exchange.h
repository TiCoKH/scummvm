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

#ifndef GOLDBOX_RUNTIME_RUNTIME_EXCHANGE_H
#define GOLDBOX_RUNTIME_RUNTIME_EXCHANGE_H

#include "common/scummsys.h"
#include "goldbox/core/global.h"

namespace Goldbox {

struct RuntimeMapSnapshot {
	bool valid = false;

	GameState gameState = GS_START_MENU;
	uint8 mapId = 0;
	bool indoorMode = true;
	uint8 mapType = 1;

	uint16 dungeonX = 0;
	uint16 dungeonY = 0;
	uint8 dungeonDir = 0;

	uint8 wildernessX = 0;
	uint8 wildernessY = 0;

	uint8 clockHour = 0;
	uint8 clockMinute = 0;

	bool searchActive = false;
	bool hideCoords = false;

	uint8 pictureHeadId = 0xFF;
	uint8 pictureBodyId = 0xFF;

	bool skyboxRedraw = false;
	bool positionDirty = false;
	bool characterRedraw = false;
	bool statusRedraw = false;
};

class RuntimeExchange {
public:
	enum IntentKind {
		kIntentNone = 0,
		kIntentMove,
		kIntentSearch,
		kIntentEncamp
	};

	struct Intent {
		IntentKind kind = kIntentNone;
		int16 value = 0;
	};

	RuntimeExchange();
	virtual ~RuntimeExchange();

	virtual bool captureMapSnapshot(RuntimeMapSnapshot &out) const;
	virtual bool submitIntent(const Intent &intent);
	bool pollIntent(Intent &out);

private:
	bool _hasPendingIntent;
	Intent _pendingIntent;
};

} // namespace Goldbox

#endif // GOLDBOX_RUNTIME_RUNTIME_EXCHANGE_H
