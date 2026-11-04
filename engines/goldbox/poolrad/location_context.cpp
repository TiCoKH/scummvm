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

#include "goldbox/poolrad/location_context.h"

#include "goldbox/runtime/runtime_exchange.h"

namespace Goldbox {
namespace Poolrad {

LocationContext deriveLocationContextFromSnapshot(
		const ::Goldbox::RuntimeMapSnapshot &snapshot) {
	// Read-only derivation: classify the TYPE from the active script id (the
	// snapshot's mapId is the saved ECL id) and the indoor/3D-view flag, then
	// mirror the render flag and coordinates. No VM/engine state is touched.
	LocationContext ctx;
	const uint8 view3dFlag = snapshot.indoorMode ? 1 : 0;
	ctx.kind = LocationContext::classify(snapshot.mapId, view3dFlag);
	ctx.syncRegion();
	ctx.activeEclId = snapshot.mapId;
	ctx.dungeonPos = snapshot.dungeonPos;
	ctx.dungeonDir = snapshot.dungeonDir & 0x03;
	ctx.wildernessPos = snapshot.wildernessPos;
	ctx.indoorRender = snapshot.indoorMode;
	return ctx;
}

uint8 deriveMapTypeFromContext(const LocationContext &ctx) {
	return ctx.toLegacyMapType();
}

} // namespace Poolrad
} // namespace Goldbox
