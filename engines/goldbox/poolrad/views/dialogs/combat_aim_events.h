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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_AIM_EVENTS_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_AIM_EVENTS_H

#include "common/str.h"
#include "goldbox/combat/combat_aim.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
} // namespace Data

namespace Poolrad {
namespace Views {
namespace Dialogs {

/** Commands of the Aim: menu (COMBAT_TargetCommandMenu keys). */
enum AimCommand {
    AIM_NEXT = 'N',
    AIM_PREV = 'P',
    AIM_MANUAL = 'M',
    AIM_TARGET = 'T',
    AIM_CENTER = 'C',
    AIM_EXIT = 'E'
};

/** Final result of an aim run, delivered once to the host. */
struct AimOutcome {
    // A target was committed (spell pick recorded, or an attack resolved).
    bool committed;
    // Weapon mode only: the attack ended the actor's actions.
    bool turnComplete;
    Combat::AimPick pick;

    AimOutcome() : committed(false), turnComplete(false) {}
};

/** Typed events from the Aim menu and the free cursor to their controller. */
class CombatAimListener {
public:
    virtual ~CombatAimListener() {}
    virtual void onAimCommand(AimCommand command) = 0;
    virtual void onManualAimCursor(TilePos cell,
                                   const Combat::AimCell &info) = 0;
    virtual void onManualAimFinished(bool picked,
                                     const Combat::AimPick &pick) = 0;
};

/** Presentation and completion events from the aim controller. */
class CombatAimHost {
public:
    virtual ~CombatAimHost() {}

    /**
     * COMBAT_FocusCharacter(3) and drawCombatInfo on the aimed creature;
     * null turns the aim cursor off.
     */
    virtual void aimShowTarget(::Goldbox::Data::PlayerCharacter *target) = 0;
    /** Free cursor on cell; occupant may be null (info panel cleared). */
    virtual void aimShowCell(TilePos cell,
            ::Goldbox::Data::PlayerCharacter *occupant) = 0;
    /** The aim cursor flies between two cells. */
    virtual void aimFlyCursor(TilePos from, TilePos to) = 0;
    /** Center: recentre the view on a cell. */
    virtual void aimCenterOn(TilePos cell) = 0;
    /** The "Range = n" line above the prompt; empty clears it. */
    virtual void aimShowRange(const Common::String &text) = 0;
    /** The aim run is over; called exactly once per begin. */
    virtual void aimFinished(const AimOutcome &outcome) = 0;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_AIM_EVENTS_H
