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

#ifndef GOLDBOX_POOLRAD_SPELL_PREPARATION_H
#define GOLDBOX_POOLRAD_SPELL_PREPARATION_H

#include "common/array.h"
#include "goldbox/data/spells/spell.h"

namespace Goldbox {
namespace Data {
namespace Items {
struct CharacterItem;
}
}

namespace Poolrad {
namespace Data {
class PoolradCharacter;
}

/**
 * Engine-light core for the camp spell-preparation flow (GB13-019).
 *
 * Owns the eligibility rules (CanDoSpellActivity / GenerateSpellList), the
 * 21-slot memorized model with bit-7 pending marks, and the scroll scribe
 * queue. The SpellBookDialog / InGameView drive it; the immediate cast path
 * routes through g_spellCasting and is not modelled here.
 *
 * Pending (bit 7) memorize slots and scribe payloads become usable only after
 * rest completes them (GB13-018). This core exposes the completion helpers
 * rest calls, but never starts rest timers itself.
 */
class SpellPreparation {
public:
    // Spell-activity kinds, mirroring ACTION_CheckSpellActionType.
    enum Activity {
        ACT_CAST = 0,
        ACT_MEMORIZE = 1,
        ACT_SCRIBE = 2
    };

    // One selectable memorize entry (GenerateSpellList output). legacyIndex is
    // the 0-based knownSpells[] index; spellId is the Spells enum value.
    struct ListEntry {
        int legacyIndex;
        Goldbox::Data::Spells::Spells spellId;

        ListEntry() : legacyIndex(-1), spellId(Goldbox::Data::Spells::SP_NONE) {}
        ListEntry(int idx, Goldbox::Data::Spells::Spells id)
            : legacyIndex(idx), spellId(id) {}
    };

    // One selectable scribe payload: the owning scroll plus the effect slot
    // (0..2) and the spell it teaches.
    struct ScribeEntry {
        Goldbox::Data::Items::CharacterItem *scroll;
        int payloadSlot;
        Goldbox::Data::Spells::Spells spellId;

        ScribeEntry()
            : scroll(nullptr), payloadSlot(-1),
              spellId(Goldbox::Data::Spells::SP_NONE) {}
    };

    explicit SpellPreparation(Data::PoolradCharacter *character);

    // CanDoSpellActivity: does the selected character have this activity at
    // all? A non-caster (no spell slots) can do nothing; scribing also needs a
    // magic-user spell book (memorize cleric/mage per available slots).
    bool canDoActivity(Activity activity) const;

    /**
     * Camp.CanDoSpellActivity: the refusal text shown after the character's
     * name, or nullptr when the activity is allowed. Casting is refused in an
     * anti-magic area; an animated or disabled character can do nothing.
     */
    const char *activityRefusal(Activity activity, bool antimagicArea) const;

    /**
     * Camp.CampSpellPrepTime: rest minutes this character needs for its
     * pending memorizing and scribing, 15 per pending spell level plus a base
     * of 4 hours (6 when any pending spell is level 3). Stores the base hours
     * in the character's spell preparation timer.
     */
    uint16 campPrepTime();

    /** Spell slots for a spell's class and level. */
    uint8 slotCapacity(Goldbox::Data::Spells::Spells spell) const {
        return slotCapacityForSpell(spell);
    }

    /** Spell slots of a class and level (Camp: slots[typ, lvl]). */
    uint8 slotAllowance(Goldbox::Data::Spells::SpellClass spellClass,
                        uint8 level) const;

    /**
     * Camp.SpellSlotsRemaining: allowance minus memorized and pending spells
     * of that class and level; the byte wraps when more are memorized.
     */
    uint8 slotsRemaining(Goldbox::Data::Spells::SpellClass spellClass,
                         uint8 level) const;

    /** Any spell on a scroll (Magic.AddAllScrollSpells(False) not empty). */
    bool hasScrollSpells() const;
    /** Any scroll spell queued for scribing. */
    bool hasPendingScribe() const;
    /** A scroll spell matching @p spell is already queued for scribing. */
    bool isScribing(Goldbox::Data::Spells::Spells spell) const;
    /**
     * Camp.CampScribeMenu: mark the first unmarked scroll payload holding
     * @p spell as pending. Returns false when there is none.
     */
    bool markScribe(Goldbox::Data::Spells::Spells spell);

    // Map 'C' gate (Explore.MapMainMenu + Camp.NoncombatCastMenu).
    enum NoncombatCastCheck {
        NCC_ALLOWED = 0,
        NCC_NOT_OKAY,           // 3D map, status not Okay: silently ignored
        NCC_ANTIMAGIC_AREA,     // "cannot cast spells in this area"
        NCC_NO_CONDITION,       // "is in no condition to cast any spells"
        NCC_NOTHING_MEMORIZED   // "has no spells memorized"
    };

    // The original does not check the class: a non-caster simply has nothing
    // memorized.
    NoncombatCastCheck checkNoncombatCast(bool dungeonMap,
        bool antimagicArea) const;

    // GenerateSpellList: known spells eligible for memorization, filtered so
    // only spells whose class/level has a nonzero slot capacity appear.
    Common::Array<ListEntry> generateMemorizeList() const;

    // --- 21-slot memorized model -------------------------------------------

    // Usable (non-pending) memorized spells, i.e. slot values 1..0x7F.
    uint8 countUsableMemorized() const;
    // Pending (bit-7) memorize marks awaiting rest.
    uint8 countPendingMemorized() const;
    // Total occupied slots (usable + pending); the 21-slot capacity bound.
    uint8 countOccupiedSlots() const;

    // Remaining class/level slot capacity for the given spell, counting both
    // usable and pending entries against spellSlots. Zero means at capacity.
    uint8 remainingSlotsForSpell(Goldbox::Data::Spells::Spells spell) const;

    // Queue one pending memorize of spell into the first free slot. Fails (and
    // mutates nothing) when the spell is not known, the class/level slot count
    // is exhausted, or all 21 slots are occupied. On success the slot holds
    // (spellId | 0x80) and the list is re-sorted by spell id.
    bool queueMemorize(Goldbox::Data::Spells::Spells spell);

    // Discard every pending memorize mark (the camp "No" / cancel path). Usable
    // slots are left untouched.
    void cancelPendingMemorize();

    // Rest completion (GB13-018 driver): clear bit 7 on one pending slot,
    // making it usable. Returns false when no pending work remains.
    bool completeOnePendingMemorize();

    // --- Scroll scribe queue -----------------------------------------------

    // GenerateSpellList for scribing: readable scroll payloads whose spell is
    // not yet known and the character may learn. Only unmarked payloads appear.
    Common::Array<ScribeEntry> generateScribeList() const;

    // Mark one scroll payload pending (set bit 7 on the effect slot). Fails
    // when the spell is already known or the payload slot is invalid.
    bool queueScribe(Goldbox::Data::Items::CharacterItem *scroll, int payloadSlot);

    // Discard pending scribe marks on all inventory scrolls (cancel path).
    void cancelPendingScribe();

    // Rest completion: teach and remove exactly one pending scribe payload.
    // Scribing takes priority over memorization (rest calls this first).
    //
    // The next safe payload is located and the spell captured BEFORE the
    // payload is removed, so a scroll freed by its last payload is never
    // touched afterwards (the Pascal freed-scroll bug is avoided). Returns
    // false when no pending scribe work remains.
    bool completeOnePendingScribe();

    // True while any pending memorize or scribe work is outstanding. Rest uses
    // this to decide whether preparation still has work to do.
    bool hasPendingWork() const;

private:
    // Sort the usable+pending occupied slots by spell id, packing them toward
    // slot 0 (CHARACTER_SortMemorizedSpells). Pending bit 7 is preserved.
    void sortMemorizedSlots();

    bool canLearnSpell(Goldbox::Data::Spells::Spells spell) const;
    uint8 slotCapacityForSpell(Goldbox::Data::Spells::Spells spell) const;

    Data::PoolradCharacter *_character;
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_SPELL_PREPARATION_H
