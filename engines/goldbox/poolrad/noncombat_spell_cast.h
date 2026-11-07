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

#ifndef GOLDBOX_POOLRAD_NONCOMBAT_SPELL_CAST_H
#define GOLDBOX_POOLRAD_NONCOMBAT_SPELL_CAST_H

#include "goldbox/data/damage_system.h"
#include "goldbox/data/effects/effect_system.h"
#include "goldbox/data/spells/spell.h"
#include "goldbox/spells/spell_context.h"
#include "goldbox/spells/spell_handlers.h"

namespace Goldbox {
namespace Poolrad {

/**
 * Magic.CastSpellNC outside combat: the spell context for a camp, map or
 * item cast, with the whole party as the target pool.
 */
class NoncombatSpellCast {
public:
	NoncombatSpellCast(Goldbox::Data::PlayerCharacter *caster,
		bool fromMagicItem,
		Goldbox::Spells::ISpellTargetPicker *picker = nullptr);

	/** Target mode 0: the spell can only be cast in combat. */
	static bool isCombatOnly(Goldbox::Data::Spells::Spells spell);

	Goldbox::Spells::SpellCastResult cast(
		Goldbox::Data::Spells::Spells spell);

private:
	Goldbox::Data::Effects::EffectSystem _effectSystem;
	Goldbox::Data::DamageSystem _damageSystem;
	Goldbox::Spells::SpellContext _context;
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_NONCOMBAT_SPELL_CAST_H
