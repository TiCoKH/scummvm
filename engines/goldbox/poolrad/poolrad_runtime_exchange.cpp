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

#include "goldbox/poolrad/poolrad_runtime_exchange.h"

#include "goldbox/core/tile_pos.h"
#include "goldbox/ecl/runtime_layout.h"
#include "goldbox/poolrad/data/poolrad_vm_layout.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/poolrad/views/in_game_view.h"

namespace Goldbox {
namespace Poolrad {

PoolradRuntimeExchange::PoolradRuntimeExchange(PoolradEngine *engine) :
		_engine(engine) {
}

PoolradRuntimeExchange::~PoolradRuntimeExchange() {
}

bool PoolradRuntimeExchange::captureMapSnapshot(
		::Goldbox::RuntimeMapSnapshot &out) const {
	if (!_engine)
		return false;

	const ECL::AddressSpace *mem = _engine->getEclMemory();
	if (!mem)
		return false;

	const Goldbox::VmLayout &vmLayout = Data::getPoolradVmLayout();
	const Goldbox::VmGlobalLayout &globalLayout =
		Data::getPoolradGlobalVmLayout();
	const ECL::EclRuntimeLayout &runtimeLayout =
		Data::getPoolradEclRuntimeLayout();

	out.valid = true;
	out.gameState = _engine->getGameState();
	out.mapId = mem->read8(vmLayout.field(kVmFieldSavedEclId).vmAddr);
	out.indoorMode =
		(mem->read8(vmLayout.field(kVmFieldIndoorModeFlag).vmAddr) != 0);
	out.mapType = mem->read8(vmLayout.field(kVmFieldIndoorModeFlag).vmAddr);
	out.wallNibble = mem->read8(globalLayout.field(kVmGlobalFieldMapWallType).vmAddr);
	out.eventId = mem->read8(globalLayout.field(kVmGlobalFieldMapSquareInfo).vmAddr);

	out.dungeonPos = MapPos(
		static_cast<int8>(mem->read8(
			globalLayout.field(kVmGlobalFieldDungeonX).vmAddr)),
		static_cast<int8>(mem->read8(
			globalLayout.field(kVmGlobalFieldDungeonY).vmAddr)));
	out.dungeonDir = mem->read8(
		globalLayout.field(kVmGlobalFieldDungeonDir).vmAddr) & 0x03;

	out.wildernessPos = TilePos(
		mem->read8(vmLayout.field(kVmFieldWildernessX).vmAddr),
		mem->read8(vmLayout.field(kVmFieldWildernessY).vmAddr));

	out.clockHour = mem->read8(vmLayout.field(kVmFieldClockHour).vmAddr);
	const uint8 minuteOnes =
		mem->read8(vmLayout.field(kVmFieldClockMinuteOnes).vmAddr);
	const uint8 minuteTens =
		mem->read8(vmLayout.field(kVmFieldClockMinuteTens).vmAddr);
	out.clockMinute = static_cast<uint8>(minuteTens * 10 + minuteOnes);

	out.searchActive =
		((mem->read8(globalLayout.field(kVmGlobalFieldSearchFlags).vmAddr) & 1)
		!= 0);
	out.hideCoords =
		(mem->read8(vmLayout.field(kVmFieldHideCoordsOrAutomapDisable).vmAddr)
		!= 0);

	out.pictureHeadId =
		mem->read8(globalLayout.field(kVmGlobalFieldPictureHeadId).vmAddr);
	out.pictureBodyId = mem->read8(Data::poolradPortraitBodyIdVmAddr());

	const uint16 skyboxRedrawAddr =
		runtimeLayout.field(ECL::kEclRuntimeSkyboxRedrawFlag);
	if (ECL::EclRuntimeLayout::isValidVmAddr(skyboxRedrawAddr))
		out.skyboxRedraw = (mem->read8(skyboxRedrawAddr) != 0);

	// Read color registers for indoor 3D viewport rendering.
	out.colorFlagFloor = mem->read8(
		globalLayout.field(kVmGlobalFieldColorFlagFloor).vmAddr);
	out.colorFlagHorizon = mem->read8(
		globalLayout.field(kVmGlobalFieldColorFlagHorizon).vmAddr);

	const uint16 positionDirtyAddr =
		runtimeLayout.field(ECL::kEclRuntimePositionDirtyFlag);
	if (ECL::EclRuntimeLayout::isValidVmAddr(positionDirtyAddr))
		out.positionDirty = (mem->read8(positionDirtyAddr) != 0);

	const uint16 charRedrawAddr =
		runtimeLayout.field(ECL::kEclRuntimeCharacterRedrawFlag);
	if (ECL::EclRuntimeLayout::isValidVmAddr(charRedrawAddr))
		out.characterRedraw = (mem->read8(charRedrawAddr) != 0);

	const uint16 statusRedrawAddr =
		runtimeLayout.field(ECL::kEclRuntimeStatusRedrawFlag);
	if (ECL::EclRuntimeLayout::isValidVmAddr(statusRedrawAddr))
		out.statusRedraw = (mem->read8(statusRedrawAddr) != 0);

	return true;
}

bool PoolradRuntimeExchange::submitIntent(const Intent &intent) {
	if (!_engine)
		return false;

	switch (intent.kind) {
	case kIntentMove:
		if (_engine->queueInGameCommand(Views::InGameView::kCmdMove))
			return true;
		break;
	case kIntentSearch:
		if (_engine->queueInGameCommand(Views::InGameView::kCmdSearch))
			return true;
		break;
	case kIntentEncamp:
		if (_engine->queueInGameCommand(Views::InGameView::kCmdEncamp))
			return true;
		break;
	default:
		break;
	}

	return RuntimeExchange::submitIntent(intent);
}

} // namespace Poolrad
} // namespace Goldbox
