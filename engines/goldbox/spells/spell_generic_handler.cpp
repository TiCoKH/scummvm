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
 */

#include "goldbox/spells/spell_generic_handler.h"

#include "goldbox/combat/combat_globals.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/damage_system.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/runtime/effect_host_bridge.h"
#include "goldbox/data/effects/effect_system.h"
#include "goldbox/data/rules/saving_throw.h"
#include "goldbox/engine.h"

namespace Goldbox {
namespace Spells {

SpellCastResult GenericSpellHandler::execute(const SpellContext &context,
		const SpellDefinition &definition,
		const TargetSelection &targets) const {
	if (!definition.entry || !context.effectSystem)
		return SpellCastResult(CAST_ERROR);

	const Goldbox::Data::Spells::SpellEntry &entry = *definition.entry;

	uint16 duration = entry.fixedDuration;
	if (entry.perLvlDuration != 0 && context.casterLevel > 0)
		duration += (uint16)(entry.perLvlDuration * context.casterLevel);

	// ST_CASTER spells select no targets explicitly (see spell_targeter.cpp);
	// fall back to the caster so the effect still lands somewhere.
	Common::Array<Goldbox::Data::PlayerCharacter *> singleCaster;
	const Common::Array<Goldbox::Data::PlayerCharacter *> *list =
		&targets.targetCharacters;
	if (list->empty() && context.caster) {
		singleCaster.push_back(context.caster);
		list = &singleCaster;
	}

	if (list->empty())
		return SpellCastResult(CAST_INVALID_TARGET);

	// need_ar: fixedRange == 255 means the spell requires an attack roll
	// (mirrors SPELL_ApplyOnTargets need_ar == 0xFF check).
	const bool needAttackRoll = (entry.fixedRange == 255);

	Goldbox::Data::Effects::EffectHostBridge *bridge =
		context.effectSystem->getHostBridge();

	for (uint i = 0; i < list->size(); ++i) {
		Goldbox::Data::PlayerCharacter *target = (*list)[i];
		if (!target)
			continue;

		// --- Attack roll (need_ar path) ---
		if (needAttackRoll && context.combat) {
			const uint8 thac0 = static_cast<uint8>(
				context.combat->attackRoll ? context.combat->attackRoll : 20);
			const uint8 roll = Goldbox::g_engine ?
				static_cast<uint8>(Goldbox::g_engine->rollDice(1, 20)) : 10;
			if (roll < thac0)
				continue;
		}

		// --- Saving throw ---
		bool saved = false;
		if (entry.savingEffect != Goldbox::Data::Spells::DMG_NO_SAVE) {
			Goldbox::Data::ADnDCharacter &adnd =
				static_cast<Goldbox::Data::ADnDCharacter &>(*target);
			saved = Goldbox::Data::Rules::checkSavingThrow(adnd,
					context.combat,
					context.effectSystem->getHandler(),
					bridge,
					entry.saveType, 0);
		}

		if (saved && entry.savingEffect == Goldbox::Data::Spells::DMG_NEGATES)
			continue;

		// --- Direct damage (need_ar spells with no effectId) ---
		if (needAttackRoll && entry.effectId == 0 && context.damageSystem) {
			const Goldbox::Data::DamageModifier mod =
				(saved && entry.savingEffect == Goldbox::Data::Spells::DMG_HALF)
				? Goldbox::Data::DAMAGE_HALF
				: Goldbox::Data::DAMAGE_NORMAL;
			context.damageSystem->applyLegacy(*target, 1, mod, saved);
			continue;
		}

		// --- Persistent effect ---
		if (entry.effectId == 0)
			continue;

		Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();
		if (!fx)
			continue;

		context.effectSystem->addOrRefreshEffect(*fx, *target,
				entry.effectId, duration, 0xFF, true);

		if (!definition.effectMessage.empty() && bridge)
			bridge->postEffectMessage(target, definition.effectMessage, true);
	}

	return SpellCastResult(CAST_OK);
}

} // namespace Spells
} // namespace Goldbox
