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

#include "goldbox/runtime/runtime_exchange.h"

namespace Goldbox {

RuntimeExchange::RuntimeExchange() :
		_hasPendingIntent(false),
		_hasPendingAsync(false) {
}

RuntimeExchange::~RuntimeExchange() {
}

bool RuntimeExchange::captureMapSnapshot(RuntimeMapSnapshot &out) const {
	(void)out;
	return false;
}

bool RuntimeExchange::submitIntent(const Intent &intent) {
	_pendingIntent = intent;
	_hasPendingIntent = (_pendingIntent.kind != kIntentNone);
	return _hasPendingIntent;
}

bool RuntimeExchange::pollIntent(Intent &out) {
	if (!_hasPendingIntent)
		return false;

	out = _pendingIntent;
	_hasPendingIntent = false;
	_pendingIntent.kind = kIntentNone;
	_pendingIntent.value = 0;
	return true;
}

void RuntimeExchange::signalAsync(AsyncTag tag, int16 result, bool success) {
	_pendingAsync.tag = tag;
	_pendingAsync.result = result;
	_pendingAsync.success = success;
	_hasPendingAsync = true;
}

bool RuntimeExchange::pollAsync(AsyncCompletion &out) {
	if (!_hasPendingAsync)
		return false;

	out = _pendingAsync;
	_hasPendingAsync = false;
	_pendingAsync.tag = kAsyncNone;
	_pendingAsync.result = 0;
	_pendingAsync.success = false;
	return true;
}

bool RuntimeExchange::hasAsync(AsyncTag tag) const {
	return _hasPendingAsync && _pendingAsync.tag == tag;
}

} // namespace Goldbox
