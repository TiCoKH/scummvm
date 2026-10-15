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

#ifndef GOLDBOX_COMBAT_COMBAT_SPELL_TARGETER_H
#define GOLDBOX_COMBAT_COMBAT_SPELL_TARGETER_H

#include "goldbox/spells/spell_targeter.h"

namespace Goldbox {
namespace Combat {

/**
 * Combat target selection for spells.
 *
 * Mirrors COMBAT_spellTargetDispatch / COMBAT_spellTargetSelection. In
 * combat only the low nibble of the spell's area-of-effect code is used:
 *   0        caster only
 *   0xF      one aimed target, or the occupants of the aimed tile when no
 *            combatant was chosen
 *   <8, >14  (area & 3) + 1 picks, duplicates not allowed
 *   8..14    pick a tile, then every combatant within radius (area & 7)
 *
 * Player casts aim through SpellContext::aimer; AI casts (and player casts
 * without an aimer) choose automatically.
 */
class CombatSpellTargeter : public Spells::ISpellTargeter {
public:
    bool selectTarget(const Spells::SpellContext &context,
                      const Spells::SpellDefinition &definition,
                      Spells::TargetSelection &result) override;
};

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_SPELL_TARGETER_H
