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

#include "wizardry/game/spells.h"

namespace Wizardry {

// Spell level and what the spell is aimed at
static const struct {
	uint8 level;
	uint8 target;
} SPELL_INFO[kSpellCount + 1] = {
	{ 0, Spells::kTargetNone },
	// Mage
	{ 1, Spells::kTargetGroup },    // HALITO
	{ 1, Spells::kTargetNone },     // MOGREF
	{ 1, Spells::kTargetGroup },    // KATINO
	{ 1, Spells::kTargetNone },     // DUMAPIC
	{ 2, Spells::kTargetGroup },    // DILTO
	{ 2, Spells::kTargetNone },     // SOPIC
	{ 3, Spells::kTargetGroup },    // MAHALITO
	{ 3, Spells::kTargetGroup },    // MOLITO
	{ 4, Spells::kTargetGroup },    // MORLIS
	{ 4, Spells::kTargetGroup },    // DALTO
	{ 4, Spells::kTargetGroup },    // LAHALITO
	{ 5, Spells::kTargetNone },     // MAMORLIS
	{ 5, Spells::kTargetNone },     // MAKANITO
	{ 5, Spells::kTargetGroup },    // MADALTO
	{ 6, Spells::kTargetGroup },    // LAKANITO
	{ 6, Spells::kTargetGroup },    // ZILWAN
	{ 6, Spells::kTargetNone },     // MASOPIC
	{ 6, Spells::kTargetNone },     // HAMAN
	{ 7, Spells::kTargetNone },     // MALOR
	{ 7, Spells::kTargetNone },     // MAHAMAN
	{ 7, Spells::kTargetNone },     // TILTOWAIT
	// Priest
	{ 1, Spells::kTargetNone },     // KALKI
	{ 1, Spells::kTargetMember },   // DIOS
	{ 1, Spells::kTargetGroup },    // BADIOS
	{ 1, Spells::kTargetNone },     // MILWA
	{ 1, Spells::kTargetNone },     // PORFIC
	{ 2, Spells::kTargetNone },     // MATU
	{ 2, Spells::kTargetNone },     // CALFO
	{ 2, Spells::kTargetGroup },    // MANIFO
	{ 2, Spells::kTargetGroup },    // MONTINO
	{ 3, Spells::kTargetNone },     // LOMILWA
	{ 3, Spells::kTargetMember },   // DIALKO
	{ 3, Spells::kTargetNone },     // LATUMAPIC
	{ 3, Spells::kTargetNone },     // BAMATU
	{ 4, Spells::kTargetMember },   // DIAL
	{ 4, Spells::kTargetGroup },    // BADIAL
	{ 4, Spells::kTargetMember },   // LATUMOFIS
	{ 4, Spells::kTargetNone },     // MAPORFIC
	{ 5, Spells::kTargetMember },   // DIALMA
	{ 5, Spells::kTargetGroup },    // BADIALMA
	{ 5, Spells::kTargetGroup },    // LITOKAN
	{ 5, Spells::kTargetNone },     // KANDI
	{ 5, Spells::kTargetMember },   // DI
	{ 5, Spells::kTargetGroup },    // BADI
	{ 6, Spells::kTargetGroup },    // LORTO
	{ 6, Spells::kTargetMember },   // MADI
	{ 6, Spells::kTargetGroup },    // MABADI
	{ 6, Spells::kTargetNone },     // LOKTOFEIT
	{ 7, Spells::kTargetNone },     // MALIKTO
	{ 7, Spells::kTargetMember }    // KADORTO
};

int Spells::level(int spell) {
	return (spell >= 1 && spell <= kSpellCount) ? SPELL_INFO[spell].level : 0;
}

Spells::Target Spells::target(int spell) {
	return (spell >= 1 && spell <= kSpellCount) ? (Target)SPELL_INFO[spell].target : kTargetNone;
}

Common::String Spells::name(const MessageFile &messages, int spell) {
	Common::String result = messages.get(5000 + spell);
	if (!result.empty() && result[0] == '*')
		result.deleteChar(0);
	return result;
}

bool Spells::canCast(const Character &c, int spell) {
	const int lv = level(spell);
	if (!lv || !c.knowsSpell(spell))
		return false;
	return (isMage(spell) ? c.mageSlots[lv - 1] : c.priestSlots[lv - 1]) > 0;
}

void Spells::spend(Character &c, int spell) {
	const int lv = level(spell);
	if (!lv)
		return;

	int16 &slots = isMage(spell) ? c.mageSlots[lv - 1] : c.priestSlots[lv - 1];
	if (slots > 0)
		--slots;
}

Common::Array<int> Spells::find(const MessageFile &messages, const Character &c, const Common::String &typed) {
	Common::Array<int> result;
	if (typed.empty())
		return result;

	for (int spell = 1; spell <= kSpellCount; ++spell) {
		if (name(messages, spell) == typed) {
			result.push_back(spell);
			return result;
		}
	}

	for (int spell = 1; spell <= kSpellCount; ++spell) {
		if (c.knowsSpell(spell) && name(messages, spell).hasPrefix(typed))
			result.push_back(spell);
	}

	return result;
}

} // End of namespace Wizardry
