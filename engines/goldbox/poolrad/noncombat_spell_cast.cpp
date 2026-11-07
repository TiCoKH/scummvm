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

#include "goldbox/poolrad/noncombat_spell_cast.h"
#include "goldbox/data/effects/effect_handler_base.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/poolrad/ecl/poolrad_engine_host_impl.h"
#include "goldbox/spells/spell_casting.h"
#include "goldbox/spells/spell_duration.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {

NoncombatSpellCast::NoncombatSpellCast(Goldbox::Data::PlayerCharacter *caster,
		bool fromMagicItem, Goldbox::Spells::ISpellTargetPicker *picker)
		: _effectSystem(Goldbox::Data::Effects::g_effectHandler,
			getEffectHostBridge()),
		  _damageSystem(getEffectHostBridge()) {
	_context.caster = caster;
	_context.effectSystem = &_effectSystem;
	_context.damageSystem = &_damageSystem;
	_context.inCombat = false;
	_context.fromMagicItem = fromMagicItem;
	_context.targetPicker = picker;

	// NoncombatCastTargetMenu targets come from the creature list, in order.
	Common::List<Goldbox::Data::PlayerCharacter *> *party =
		VmInterface::getParty();
	if (party) {
		for (Common::List<Goldbox::Data::PlayerCharacter *>::iterator it =
				party->begin(); it != party->end(); ++it)
			if (*it)
				_context.allies.push_back(*it);
	}
}

bool NoncombatSpellCast::isCombatOnly(Goldbox::Data::Spells::Spells spell) {
	const Common::Array<Goldbox::Data::Spells::SpellEntry> &entries =
		Goldbox::Data::Rules::getSpellEntries();
	const uint id = static_cast<uint>(spell);
	return id < entries.size() &&
		entries[id].targetType == Goldbox::Data::Spells::ST_COMBAT;
}

Goldbox::Spells::SpellCastResult NoncombatSpellCast::cast(
		Goldbox::Data::Spells::Spells spell) {
	if (!_context.caster || !Goldbox::Spells::g_spellCasting)
		return Goldbox::Spells::SpellCastResult(Goldbox::Spells::CAST_ERROR);

	_context.casterLevel = Goldbox::Spells::getSpellCastingLevel(
		_context.caster, static_cast<uint8>(spell), _context.fromMagicItem);
	return Goldbox::Spells::g_spellCasting->castSpell(_context, spell);
}

} // namespace Poolrad
} // namespace Goldbox
