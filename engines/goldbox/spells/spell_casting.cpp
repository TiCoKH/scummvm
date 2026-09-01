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

#include "goldbox/spells/spell_casting.h"
#include "goldbox/data/spells/spell_book.h"
#include "goldbox/data/effects/effect_system.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/engine.h"

namespace Goldbox {
namespace Spells {

// Mirrors SPELL_ComputeDuration. Returns an 8-bit duration value.
// Six spell IDs use special dice formulas; all others use the table.
uint8 computeSpellDuration(uint8 spellId, uint8 casterLevel, bool inCombat) {
    switch (spellId) {
    case 0x28: // 40: 1d6 * 10
        return static_cast<uint8>(
            (Goldbox::g_engine ? Goldbox::g_engine->rollDice(1, 6) : 3) * 10);

    case 0x39: // 57: 5d4
    case 0x3D: // 61: 5d4
        return static_cast<uint8>(
            Goldbox::g_engine ? Goldbox::g_engine->rollDice(5, 4) : 10);

    case 0x3B: // 59: 1d4 * 10 + 40
        return static_cast<uint8>(
            (Goldbox::g_engine ? Goldbox::g_engine->rollDice(1, 4) : 2) * 10 + 40);

    case 0x3F: // 63: combat = 2d10*10, non-combat = (1d10+10)*10
        if (inCombat)
            return static_cast<uint8>(
                (Goldbox::g_engine ? Goldbox::g_engine->rollDice(2, 10) : 10) * 10);
        else
            return static_cast<uint8>(
                ((Goldbox::g_engine ? Goldbox::g_engine->rollDice(1, 10) : 5) + 10) * 10);

    case 0x43: // 67: fixed
        return 160;

    default: {
        const Common::Array<Goldbox::Data::Spells::SpellEntry> &entries =
            Goldbox::Data::Rules::getSpellEntries();
        if (spellId >= entries.size())
            return 0;
        const Goldbox::Data::Spells::SpellEntry &entry = entries[spellId];
        // Preserve 8-bit wrap semantics of the original.
        return static_cast<uint8>(entry.fixedDuration +
            entry.perLvlDuration * casterLevel);
    }
    }
}

SpellCastingService::SpellCastingService()
    : _combatTargeter(nullptr), _nonCombatTargeter(nullptr) {
    _registry.setHandler(kHandlerCureLightWounds, &_cureLightWoundsHandler);
    _registry.setHandler(kHandlerBurningHands,    &_burningHandsHandler);
    _registry.setHandler(kHandlerCharmPerson,     &_charmPersonHandler);
    _registry.setHandler(kHandlerEnlarge,         &_enlargeHandler);
    _registry.setHandler(kHandlerReduce,          &_reduceHandler);
    _registry.setHandler(kHandlerFriends,         &_friendsHandler);
    _registry.setHandler(kHandlerMagicMissile,    &_magicMissileHandler);
    _registry.setHandler(kHandlerShield,          &_shieldHandler);
    _registry.setHandler(kHandlerShockingGrasp,   &_shockingGraspHandler);
    _registry.setHandler(kHandlerSleep,           &_sleepHandler);
    _registry.setHandler(kHandlerHoldPerson,      &_holdPersonHandler);
    _registry.setHandler(kHandlerResistFire,      &_resistFireHandler);
    _registry.setHandler(kHandlerSilence15Radius, &_silence15RadiusHandler);
    _registry.setHandler(kHandlerSlowPoison,      &_slowPoisonHandler);
    _registry.setHandler(kHandlerSnakeCharm,      &_snakeCharmHandler);
    _registry.setHandler(kHandlerSpiritualHammer, &_spiritualHammerHandler);
    _registry.setHandler(kHandlerMirrorImage,     &_mirrorImageHandler);
    _registry.setHandler(kHandlerStinkingCloud,   &_stinkingCloudHandler);
    _registry.setHandler(kHandlerStrength,        &_strengthHandler);
    _registry.setHandler(kHandlerAnimateDead,     &_animateDeadHandler);
    _registry.setHandler(kHandlerCureBlindness,    &_cureBlindnessHandler);
    _registry.setHandler(kHandlerDispelMagicShared, &_dispelMagicHandler);
    _registry.setHandler(kHandlerPrayer,            &_prayerHandler);
    _registry.setHandler(kHandlerRemoveCurse,       &_removeCurseHandler);
    _registry.setHandler(kHandlerBestowCurse,       &_bestowCurseHandler);
    _registry.setHandler(kHandlerBlink,             &_blinkHandler);
    _registry.setHandler(kHandlerFireball,           &_fireballHandler);
    _registry.setHandler(kHandlerHaste,              &_hasteHandler);
    _registry.setHandler(kHandlerSlow,               &_slowHandler);
    _registry.setHandler(kHandlerLightningBolt,      &_lightningBoltHandler);
    _registry.setHandler(kHandlerSpellID60,          &_spellID60Handler);
    _registry.setHandler(kHandlerSpellID57,          &_spellID57Handler);
    _registry.setHandler(kHandlerSpellID59,          &_spellID59Handler);
    _registry.setHandler(kHandlerSpellID61,          &_spellID61Handler);
    _registry.setHandler(kHandlerSpellID62,          &_spellID62Handler);
    _registry.setHandler(kHandlerSpellID63,          &_spellID63Handler);
    _registry.setHandler(kHandlerSpellID65,          &_spellID65Handler);
    _registry.setHandler(kHandlerSpellID58,          &_spellID58Handler);
    _registry.setHandler(kHandlerRestore,            &_restoreHandler);
    _registry.setHandler(kHandlerBreathWeapon,       &_breathWeaponHandler);

    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_CL1_CURE_LT_WOUNDS,
        kHandlerCureLightWounds);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL1_BURNING_HANDS,
        kHandlerBurningHands);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL1_CHARM_PERSON,
        kHandlerCharmPerson);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL1_ENLARGE,
        kHandlerEnlarge);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL1_REDUCE,
        kHandlerReduce);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL1_FRIENDS,
        kHandlerFriends);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL1_MAGIC_MISSILE,
        kHandlerMagicMissile);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL1_SHIELD,
        kHandlerShield);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL1_SHOCKING_GRASP,
        kHandlerShockingGrasp);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL1_SLEEP,
        kHandlerSleep);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_CL2_HOLD_PERSON,
        kHandlerHoldPerson);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL3_HOLD_PERSON,
        kHandlerHoldPerson);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_CL2_RESIST_FIRE,
        kHandlerResistFire);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_CL2_SILENCE_15R,
        kHandlerSilence15Radius);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_CL2_SLOW_POISON,
        kHandlerSlowPoison);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_CL2_SNAKE_CHARM,
        kHandlerSnakeCharm);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_CL2_SPIRIT_HAMMER,
        kHandlerSpiritualHammer);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL2_MIRROR_IMAGE,
        kHandlerMirrorImage);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL2_STINKING_CLOUD,
        kHandlerStinkingCloud);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL2_STRENGTH,
        kHandlerStrength);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_CL3_ANIMATE_DEAD,
        kHandlerAnimateDead);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_CL3_CURE_BLINDNESS,
        kHandlerCureBlindness);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_CL3_DISPEL_MAGIC,
        kHandlerDispelMagicShared);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL3_DISPEL_MAGIC,
        kHandlerDispelMagicShared);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_CL3_PRAYER,
        kHandlerPrayer);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_CL3_REMOVE_CURSE,
        kHandlerRemoveCurse);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_CL3_BESTOW_CURSE,
        kHandlerBestowCurse);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL3_BLINK,
        kHandlerBlink);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL3_FIREBALL,
        kHandlerFireball);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL3_HASTE,
        kHandlerHaste);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL3_SLOW,
        kHandlerSlow);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MUL3_LIGHTNING_BOLT,
        kHandlerLightningBolt);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MI4,
        kHandlerSpellID60);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MI1,
        kHandlerSpellID57);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MI2,
        kHandlerSpellID58);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MI3,
        kHandlerSpellID59);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MI5,
        kHandlerSpellID61);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MI6,
        kHandlerSpellID62);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MI7,
        kHandlerSpellID63);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MI9,
        kHandlerSpellID65);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_CL7_RESTORATION,
        kHandlerRestore);
    _registry.setHandlerForSpell(Goldbox::Data::Spells::SP_MI10,
        kHandlerBreathWeapon);
}

SpellCastResult SpellCastingService::castSpell(SpellContext &context,
        Goldbox::Data::Spells::Spells spell) {
    const SpellDefinition *definition = _registry.getDefinition(spell);
    if (!definition || !definition->entry)
        return SpellCastResult(CAST_ERROR);

    SpellCastResult ok = validate(context, *definition);
    if (ok.status != CAST_OK)
        return ok;

    ISpellTargeter *targeter = getTargeter(context);
    if (!targeter)
        return SpellCastResult(CAST_INVALID_TARGET);

    TargetSelection targets;
    if (!targeter->selectTarget(context, *definition, targets))
        return SpellCastResult(CAST_INVALID_TARGET);

    // kHandlerGeneric spells (the vast majority) have no dedicated
    // handler registered; GenericSpellHandler drives their saving throw +
    // effect apply directly from SpellEntry data (see spell_resolver.cpp).
    const ISpellHandler *handler = _registry.getHandler(definition->handlerId);
    if (!handler)
        handler = &_fallbackHandler;

    SpellCastResult result = handler->execute(context, *definition, targets);
    if (result.status == CAST_OK && context.spellBook) {
        uint8 count = context.spellBook->getMemorized(spell);
        if (count > 0)
            context.spellBook->setMemorized(spell, count - 1);
    }
    return result;
}

void SpellCastingService::setCombatTargeter(ISpellTargeter *targeter) {
    _combatTargeter = targeter;
}

void SpellCastingService::setNonCombatTargeter(ISpellTargeter *targeter) {
    _nonCombatTargeter = targeter;
}

SpellCastResult SpellCastingService::validate(const SpellContext &context,
        const SpellDefinition &definition) const {
    if (!context.spellBook)
        return SpellCastResult(CAST_ERROR);

    if (!context.spellBook->isKnown(definition.id))
        return SpellCastResult(CAST_NOT_KNOWN);

    if (context.spellBook->getMemorized(definition.id) == 0)
        return SpellCastResult(CAST_NOT_MEMORIZED);

    return SpellCastResult(CAST_OK);
}

ISpellTargeter *SpellCastingService::getTargeter(
        const SpellContext &context) const {
    if (context.inCombat)
        return _combatTargeter;
    return _nonCombatTargeter;
}

} // namespace Spells
} // namespace Goldbox
