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

#ifndef GOLDBOX_COMBAT_COMBAT_TURN_H
#define GOLDBOX_COMBAT_COMBAT_TURN_H

#include "common/scummsys.h"
#include "common/array.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/items/character_item.h"

namespace Goldbox {
namespace Combat {

struct CombatGlobals;

/**
 * Returns true if the given combat side is being ambushed.
 *
 * Mirrors COMBAT_IsSideAmbushed: checks bit 0 for CS_PARTY, bit 1 for CS_ENEMY.
 *
 * @param side         The combat side to check
 * @param ambushFlags  D_CombatIsAmbush byte from VM party state
 */
bool isSideAmbushed(Data::CombatSide side, uint8 ambushFlags);

/**
 * Recalculate primary attack count for one character.
 *
 * Mirrors COMBAT_RecalcPrimaryAttacks: resets curPrimaryRoll.attacks from base,
 * uses fireRate for ranged weapons (min 2), runs ES_COMBAT_RATE_MODIFIER,
 * caps by ammo stack if ranged, then conditionally writes back based on unknownBool.
 */
void recalcPrimaryAttacks(Data::ADnDCharacter *adnd);

/**
 * Compute the per-round secondary attack count with even-turn bonus.
 *
 * Mirrors COMBAT_CalcAttackCountWithEvenTurnBonus:
 *   - Odd turnCounter (= even-numbered turn) grants +1 attack before halving.
 *   - Result = (baseAttackCount + bonus) / 2.
 *
 * @param attacks      Raw attack count (EFFECT_SET18_EXCHANGE_VALUE after effect processing)
 * @param turnCounter  Current round counter (COMBAT_TURN_COUNTER global in original)
 * @return             Effective attacks this round
 */
uint8 calcAttackCountWithEvenTurnBonus(uint8 attacks, uint8 turnCounter);

/**
 * Reset per-turn CombatAction fields for one character.
 *
 * Mirrors original COMBAT_InitCharacterTurnState.
 * Loads sec_attack into globals->effectSet18 (isMovement=false), runs
 * ES_COMBAT_RATE_MODIFIER (effect set 18: HASTE, SLOW, IMMOBILIZED)
 * whose handlers modify globals->effectSet18.value, then reads the result
 * back through calcAttackCountWithEvenTurnBonus into cs.attackCount.
 * Move budget is set directly after the effect set (not part of the exchange).
 *
 * @param ch             Character to initialise (must have valid combatState)
 */
void initCharacterTurnState(Data::PlayerCharacter *ch);

/**
 * Reset turn state for every character in the roster.
 */
void initAllTurnStates(Common::Array<Data::PlayerCharacter *> &roster);

/**
 * Select the next character to act this round.
 *
 * Mirrors COMBAT_SelectNextActor: returns the enabled character with the
 * highest initiative value that has not yet acted (initiative != 0xFF).
 * Returns nullptr when all characters have acted (round complete) or a
 * side has no members.
 *
 * @param roster   Full combat roster
 * @param globals  Combat globals (reads sideCount to detect round end)
 * @return         Next actor, or nullptr if the round is over
 */
Data::PlayerCharacter *selectNextActor(
    const Common::Array<Data::PlayerCharacter *> &roster,
    const CombatGlobals &globals);

/**
 * Mirrors COMBAT_ResolveAttack.
 *
 * Outer attack wrapper: adjusts facing for both combatants, triggers
 * ranged-attack animations, sets attacker->combatState->target, calls
 * resolveAttackSequence, consumes the supplied item stack, recalculates
 * combat stats, and resets action state.
 *
 * The view callbacks (drawCombatInfo, updateCharacterFacingAndRedraw,
 * animateRangedAttack) are invoked through the supplied delegate so the
 * data layer stays free of rendering code.
 *
 * @param attacker      Attacking character (must have valid combatState)
 * @param target        Defending character
 * @param useBehindAC   True forces rear-AC path; false allows normal facing logic
 * @param item          Explicitly supplied ranged item (may be nullptr)
 * @param result        Set to true when the action is fully resolved
 * @param view          View delegate for facing/animation callbacks (may be nullptr)
 */
struct CombatViewDelegate;

void resolveAttack(Data::PlayerCharacter *attacker,
                   Data::PlayerCharacter *target,
                   bool useBehindAC,
                   Data::Items::CharacterItem *item,
                   bool *result,
                   CombatViewDelegate *view);

/**
 * Mirrors COMBAT_ResolveAttackSequence.
 *
 * Executes the full attack resolution for one attacker/target pair.
 * Handles the negative-effect fast path, large-target weapon swap,
 * AC selection (front/rear/backstab), per-slot attack loops, damage,
 * and action-state reset.
 *
 * @param attacker       Attacking character (must have valid combatState)
 * @param target         Defending character
 * @param useBehindAC    True if the attack comes from behind (use rear AC)
 * @param result         Set to true when all attack slots are exhausted
 */
void resolveAttackSequence(Data::PlayerCharacter *attacker,
                           Data::PlayerCharacter *target,
                           bool useBehindAC,
                           bool *result,
                           CombatViewDelegate *view);

/**
 * Mirrors UTIL_RollToHit.
 *
 * Removes Blur from attacker, rolls d20, applies natural-1/20 rules,
 * runs ES_ATTACKER_TO_HIT (10) and ES_DEFENDER_TO_HIT (16), reads the
 * side-specific THAC0/damage bonus from VM globals, and tests the
 * modified roll against targetAC.
 *
 * Stores the raw d20 result in globals.attackRoll.
 *
 * @param attacker      Attacking character
 * @param defender      Defending character
 * @param targetAC      Defender's effective AC to test against
 */
bool rollToHit(Data::PlayerCharacter *attacker,
               Data::PlayerCharacter *defender,
               uint8 targetAC);

/**
 * Mirrors COMBAT_RollAttackDamage.
 *
 * Rolls damage dice from attacker's current roll for the given slot,
 * applies strength bonus, stores result in globals.damage, and calls
 * applyDamage() on the target.
 */
void rollAttackDamage(Data::PlayerCharacter *attacker,
                      Data::PlayerCharacter *target,
                      uint8 slot);

/**
 * Mirrors COMBAT_AdjustAcForFacingAndRange.
 *
 * Applies range penalty (+1 AC per 2 tiles beyond melee) and facing
 * bonus (-2 AC for rear arc) to *targetAC.
 */
void adjustAcForFacingAndRange(Data::PlayerCharacter *attacker,
                               Data::PlayerCharacter *target,
                               uint8 *targetAC);

/**
 * View delegate interface for resolveAttack presentation callbacks.
 *
 * The data layer calls these at the appropriate points in the attack
 * sequence. The view layer provides a concrete implementation; pass
 * nullptr to skip all presentation (e.g. in unit tests).
 */
struct CombatViewDelegate {
    virtual ~CombatViewDelegate() {}

    /** Draw the attacker's combat info panel. Mirrors CombatView::drawCombatInfo. */
    virtual void drawCombatInfo(Data::PlayerCharacter *attacker) = 0;

    /**
     * Update character facing and redraw.
     * redrawMode 0 = restore old tiles, 1 = draw current.
     * Mirrors CombatView::updateCharacterFacingAndRedraw.
     */
    virtual void updateCharacterFacingAndRedraw(Data::PlayerCharacter *ch,
                                                uint8 direction,
                                                uint8 redrawMode,
                                                bool restoreOld) = 0;

    /** Animate a ranged attack from attacker to target using item. */
    virtual void animateRangedAttack(Data::PlayerCharacter *attacker,
                                     Data::PlayerCharacter *target,
                                     Data::Items::CharacterItem *item) = 0;

    /**
     * Display the result of one attack swing.
     * Mirrors COMBAT_drawAttackResult.
     *
     * facingMode: 0=normal, 1=from behind, 2=backstab, 3=helpless
     * damage:        actual damage applied
     * damageDisplay: damage value shown in message (may differ)
     * resultType:    0=miss, non-zero=hit
     */
    virtual void drawAttackResult(Data::PlayerCharacter *attacker,
                                  Data::PlayerCharacter *target,
                                  uint8 facingMode,
                                  uint8 damage,
                                  uint8 damageDisplay,
                                  uint8 resultType) = 0;
};

/**
 * Mirrors COMBAT_ExecuteMultiAttack.
 *
 * Performs a sweeping melee attack against all eligible range-1 targets.
 * Only executes when:
 *   - attacker->combatState->attackCount < attacker->combatState->maxTargets
 *   - target is enabled (valid living combatant)
 *   - target is exactly range 1 from attacker
 *   - eligible adjacent targets exceed attacker's current attackCount
 *
 * The explicitly selected target is moved to the front of the target order
 * so it is always attacked first. Each eligible target (up to maxTargets)
 * receives one primary attack with attackCount forced to 1.
 *
 * Calls showSweepMessage(attacker) for the "sweeps" presentation before
 * the attack loop; pass nullptr to skip.
 *
 * @return true if the multi-attack was executed, false if conditions not met.
 */
bool executeMultiAttack(Data::PlayerCharacter *attacker,
                        Data::PlayerCharacter *selectedTarget,
                        CombatContext &ctx,
                        CombatViewDelegate *view);

/**
 * Mirrors UTIL_SurrenderSetup.
 *
 * Removes the two status effects (0x4A, 0x4B) that interfere with the
 * surrender/flee state. Called at the start of checkSurrender() and
 * again on moral failure to ensure they are absent after a failed attempt.
 */
void surrenderSetup(Data::PlayerCharacter *ch);

/**
 * Mirrors COMBAT_CheckSurrender.
 *
 * Evaluates whether ch surrenders this turn based on morale, HP loss,
 * party morale threshold, and movement reach vs opposing side.
 *
 * Outcomes (mutually exclusive):
 *   - Already fleeing: sets moralFailure=true, shows "forced to flee" via
 *     view, returns false.
 *   - npc <= 0x7F: not eligible for morale processing, returns false.
 *   - First/second morale gate not passed: returns false.
 *   - movementReach < opposingReach AND intelligence > 5: sets character
 *     status to S_UNCONSCIOUS ("Surrenders"), ends actor turn, returns true.
 *   - movementReach >= opposingReach: sets moralFailure=true, removes
 *     effects 0x4A/0x4B, returns false.
 *
 * The view delegate is used only for the "forced to flee" message.
 * Pass nullptr to skip presentation.
 */
bool checkSurrender(Data::PlayerCharacter *ch,
                    CombatContext &ctx,
                    CombatViewDelegate *view);

/**
 * View delegate for castSpell presentation callbacks.
 * Pass nullptr to skip all presentation (e.g. AI casting, unit tests).
 */
struct CastSpellViewDelegate {
    virtual ~CastSpellViewDelegate() {}

    /** Scroll/focus the viewport on the caster and refresh the combat info panel. */
    virtual void focusCaster(Data::PlayerCharacter *caster) = 0;

    /** Display "Begins Casting" message for a timed spell. */
    virtual void showBeginsCasting(Data::PlayerCharacter *caster) = 0;
};

/**
 * Mirrors COMBAT_CastSpell (data/game-rule layer only).
 *
 * Caller must supply a non-zero spellId; spell selection (DIALOG_Spells)
 * is the view layer's responsibility.
 *
 * Two paths:
 *   castTime / 3 == 0  → immediate: execute useSpell(), end actor turn,
 *                         return result of endTurn.
 *   castTime / 3 > 0   → timed: store spellId in combatState, consume
 *                         initiative (strict <; floor at 1), return true.
 *
 * Rejects spells with whenCast == IN_CAMP (combat-unavailable).
 * Presentation callbacks are routed through CastSpellViewDelegate.
 *
 * @param actor     The casting character (globals.attacker / PTR_SELECTED_CHAR)
 * @param spellId   Non-zero spell to cast
 * @param aiControl True when called from AI (suppresses view callbacks)
 * @param view      View delegate (may be nullptr)
 * @return          True when the action was consumed (spell cast or queued)
 */
bool castSpell(Data::PlayerCharacter *actor,
               uint8 spellId,
               bool aiControl,
               CastSpellViewDelegate *view);

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_TURN_H
