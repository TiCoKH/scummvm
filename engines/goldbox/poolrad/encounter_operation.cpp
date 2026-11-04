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

#include "goldbox/poolrad/encounter_operation.h"

namespace Goldbox {
namespace Poolrad {

EncounterOperation::EncounterOperation()
	: _config(), _state(), _destAddr(0), _monsterId(0), _monsterPicId(0),
	  _requestedDist(0), _complete(false), _outcome(ECL::kEncounterCombat) {
}

void EncounterOperation::build(uint8 monsterId, uint8 requestedDist,
		uint8 monsterPicId, uint16 destAddr,
		const Common::Array<uint8> &resp,
		const Common::Array<Common::String> &messages,
		uint8 fleePar, uint8 fleeMon) {
	_monsterId = monsterId;
	_requestedDist = requestedDist;
	_monsterPicId = monsterPicId;
	_destAddr = destAddr;

	for (int i = 0; i < 5; ++i)
		_config.resp[i] = (static_cast<uint>(i) < resp.size()) ? resp[i] : 0;
	for (int i = 0; i < 3; ++i)
		_messages[i] = (static_cast<uint>(i) < messages.size())
				? messages[i] : Common::String();

	_config.fleePar = fleePar;
	_config.fleeMon = fleeMon;

	_complete = false;
	_outcome = ECL::kEncounterCombat;
}

void EncounterOperation::begin(uint8 maxMonsterDist, uint8 view3dFlag,
		const ECL::MoveExtremes &extremes) {
	_config.hiMove = extremes.hiMove;
	_config.loMove = extremes.loMove;

	// Clamp the computed stage distance down to the requested distance
	// (eclvm.pas:1094-1095).
	uint8 dist = maxMonsterDist;
	if (_requestedDist < dist)
		dist = _requestedDist;

	_state.monsterDist = dist;
	_state.view3dFlag = view3dFlag;
	_complete = false;
	_outcome = ECL::kEncounterCombat;
}

ECL::EncounterStep EncounterOperation::applySelection(uint8 rawSel) {
	const ECL::EncounterStep step =
		ECL::encounterResolveSelection(_config, _state, rawSel);

	_state.monsterDist = step.monsterDist;
	if (step.terminal) {
		_complete = true;
		_outcome = step.outcome;
	}
	return step;
}

bool EncounterOperation::usesParleyMenu() const {
	return ECL::encounterUsesParleyMenu(_state);
}

uint8 EncounterOperation::currentMessageSlot() const {
	bool nonEmpty[3];
	for (int i = 0; i < 3; ++i)
		nonEmpty[i] = !_messages[i].empty();
	return ECL::encounterMessageSlot(_state.monsterDist, nonEmpty);
}

const Common::String &EncounterOperation::message(uint8 oneBasedSlot) const {
	if (oneBasedSlot < 1 || oneBasedSlot > 3)
		return _messages[0];
	return _messages[oneBasedSlot - 1];
}

} // namespace Poolrad
} // namespace Goldbox
