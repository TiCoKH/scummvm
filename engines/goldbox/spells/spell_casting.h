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

#ifndef GOLDBOX_SPELLS_SPELL_CASTING_H
#define GOLDBOX_SPELLS_SPELL_CASTING_H

#include "goldbox/spells/spell_context.h"
#include "goldbox/spells/spell_duration.h"
#include "goldbox/spells/spell_registry.h"
#include "goldbox/spells/spell_generic_handler.h"
#include "goldbox/spells/spell_handlers.h"
#include "goldbox/spells/spell_targeter.h"

namespace Goldbox {
namespace Spells {

class SpellCastingService {
public:
    SpellCastingService();

    SpellCastResult castSpell(SpellContext &context,
                              Goldbox::Data::Spells::Spells spell);

    void setCombatTargeter(ISpellTargeter *targeter);
    void setNonCombatTargeter(ISpellTargeter *targeter);

    SpellRegistry &registry() { return _registry; }
    const SpellRegistry &registry() const { return _registry; }

private:
    SpellCastResult validate(const SpellContext &context,
                             const SpellDefinition &definition) const;
    ISpellTargeter *getTargeter(const SpellContext &context) const;

    SpellRegistry _registry;
    ISpellTargeter *_combatTargeter;
    ISpellTargeter *_nonCombatTargeter;
    GenericSpellHandler _fallbackHandler;
    CureLightWoundsHandler _cureLightWoundsHandler;
    BurningHandsHandler _burningHandsHandler;
    CharmPersonHandler _charmPersonHandler;
    EnlargeHandler _enlargeHandler;
    ReduceHandler _reduceHandler;
    FriendsHandler _friendsHandler;
    MagicMissileHandler _magicMissileHandler;
    ShieldHandler _shieldHandler;
    ShockingGraspHandler _shockingGraspHandler;
    SleepHandler _sleepHandler;
    HoldPersonHandler _holdPersonHandler;
    ResistFireHandler _resistFireHandler;
    Silence15RadiusHandler _silence15RadiusHandler;
    SlowPoisonHandler _slowPoisonHandler;
    SnakeCharmHandler _snakeCharmHandler;
    SpiritualHammerHandler _spiritualHammerHandler;
    MirrorImageHandler _mirrorImageHandler;
    StinkingCloudHandler _stinkingCloudHandler;
    StrengthHandler _strengthHandler;
    AnimateDeadHandler _animateDeadHandler;
    CureBlindnessHandler _cureBlindnessHandler;
    DispelMagicHandler _dispelMagicHandler;
    PrayerHandler _prayerHandler;
    RemoveCurseHandler _removeCurseHandler;
    BestowCurseHandler _bestowCurseHandler;
    BlinkHandler _blinkHandler;
    FireballHandler _fireballHandler;
    // Haste removes E_POOLRAD_SLOWED (0x2A) from allies; Slow removes raw
    // haste effect (0x27) from enemies.
    HasteSlowHandler _hasteHandler{0x2A, false};
    HasteSlowHandler _slowHandler{0x27, true};
    LightningBoltHandler _lightningBoltHandler;
    SpellID60Handler _spellID60Handler;
};

} // namespace Spells
} // namespace Goldbox

#endif // GOLDBOX_SPELLS_SPELL_CASTING_H
