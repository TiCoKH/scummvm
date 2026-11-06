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

#ifndef GOLDBOX_POOLRAD_ECL_POOLRAD_ENGINE_HOST_IMPL_H
#define GOLDBOX_POOLRAD_ECL_POOLRAD_ENGINE_HOST_IMPL_H

#include "common/array.h"
#include "common/list.h"
#include "common/scummsys.h"
#include "common/ptr.h"
#include "goldbox/ecl/ecl_syscall_impl.h"
#include "goldbox/ecl/ecl_memory.h"
#include "goldbox/ecl/pending_operation.h"

namespace Goldbox {

class Engine;
class View;
struct MenuItemList;
class UIElement;

namespace Gfx {
class DaxTile;
}

namespace Data {
class PlayerCharacter;
namespace Effects {
class EffectHostBridge;
}
}

namespace Poolrad {
namespace Views {
namespace Dialogs {
class HorizontalMenu;
class HorizontalInput;
class VerticalMenu;
class PartySelector;
}
}

namespace Data {
class PoolradCharacter;
}

Goldbox::Data::Effects::EffectHostBridge *getEffectHostBridge();

/**
 * Pool of Radiance implementation of the ECL engine host interface.
 *
 * Bridges ECL VM calls to Pool of Radiance engine systems:
 * - Text output   → Message box / Dialog
 * - Player input  → Dialog input fields
 * - Combat        → Combat engine / Dialog
 * - Menus         → Menu dialogs / View selection
 * - Pictures      → Picture display system
 * - Scripts       → ECL VM loader
 * - Wall sets     → Poolrad walldef + DaxTile cache
 *
 * Inherits both EclEngineHost (full game-host contract) and SyscallHandler
 * (low-level VM dispatch contract) so a single instance can be wired directly
 * to EclVM::setSyscallHandler().
 */
class PoolradEngineHostImpl : public ECL::EclSyscallImpl {
public:
    struct WallSetRuntimeState {
        bool loaded = false;
        uint8 walldefBlockId = 0xFF;
        uint8 tileBlockId = 0xFF;
        uint8 chunkIndex = 0;
    };

    /**
     * @param engine Reference to main GoldBox engine (must be PoolradEngine)
     * @param memory Reference to ECL virtual memory for reading/writing results
     */
    PoolradEngineHostImpl(::Goldbox::Engine *engine, ECL::AddressSpace *memory);
    ~PoolradEngineHostImpl() override;

    VmResult loadMonster(uint8 monsterId, uint8 count,
        uint8 graphicId) override;
    VmResult clearMonsters() override;
    VmResult addNpc(uint8 monsterId, uint8 morale) override;
    VmResult displayPicture(uint8 picID) override;
    VmResult drawEncounterStage(uint8 resourceId, uint8 distanceCap,
        uint8 variantId) override;
    int16 parlayMenu(const Common::Array<Common::String> &attitudes) override;
    VmResult encounterMenu(const Common::Array<uint8> &operands) override;
    VmResult redrawEncounterStage(uint8 newDistance) override;
    VmResult spriteOff() override;
    int16 horizontalMenu(const Common::Array<Common::String> &options) override;
    int16 inputNumber(uint8 maxDigits) override;
    Common::String inputString(uint8 maxLength) override;
    int16 verticalMenu(const Common::String &message,
        const Common::Array<Common::String> &options) override;
    int16 selectPartyMember(const Common::String &message) override;
    VmResult beginInputNumberAsync(uint16 resultAddr,
        uint8 maxDigits) override;
    VmResult beginInputStringAsync(uint16 resultAddr,
        uint8 maxLength) override;
    VmResult beginVerticalMenuAsync(uint16 resultAddr,
        const Common::String &message,
        const Common::Array<Common::String> &options) override;
    VmResult beginSelectPartyMemberAsync(
        const Common::String &message) override;
    VmResult handleCallOpcode(uint16 callId) override;
    VmResult executeProgram(uint8 programID) override;
    VmResult readGeoAtPosition() override;
    // Animate the view picture while an ECL horizontal menu waits.
    void tickAsyncMenuPicture();
    VmResult refreshViewport() override;
    VmResult beginDelay() override;
    VmResult advanceClock(uint8 amount) override;
    VmResult robParty(uint8 wholeParty, uint8 percentMoney,
        uint8 itemChance) override;
    VmResult checkParty(uint16 attributeAddr, uint16 effectId,
        uint16 highAddr, uint16 lowAddr) override;
    VmResult hasEffect(uint8 effectId, uint16 resultAddr) override;
    bool hasEffectActive(uint8 effectId) const override;
    bool findItemInParty(uint8 itemId) const override;
    VmResult findSpell(uint8 spellId, uint16 memberAddr,
        uint16 slotAddr) override;
    VmResult startCombat() override;
    void clearTextBox() override;
    VmResult beginPrintAsync(const Common::String &text,
        bool clearBox) override;
    VmResult beginHorizontalMenuAsync(uint16 resultAddr,
        const Common::Array<Common::String> &options) override;
    bool hasPendingAsync() const override;
    bool isPendingAsyncReady() const override;
    VmResult finalizePendingAsync() override;
    VmResult loadScript(uint8 scriptID) override;
    VmResult loadGeoBlock(uint8 blockId) override;
    VmResult loadIconBlock() override;
    VmResult loadWallSet(uint8 blockId, uint8 setSlot) override;
    VmResult onMapDataReady() override;
    VmResult enterShop() override;
    VmResult enterTemple() override;
    VmResult setupTreasure(uint8 copper, uint8 silver, uint8 electrum,
        uint8 gold, uint8 platinum, uint8 gems, uint8 jewelry,
        uint8 itemSetId) override;
    const WallSetRuntimeState &wallSetState(int slot) const;

    /**
     * GB13-009 (8.2): bump the pending-operation request generation so any
     * still-pending completion minted in the OLD context is discarded as
     * stale. Call at context-replacement seams: NEW ECL (loadScript), new
     * battle setup (startCombat) and new-game/load. The bump happens exactly
     * when the old context is being replaced, which is precisely when a
     * leftover completion for that context SHOULD no longer apply. This is the
     * engine-reachable hook for the new-game/load seam, whose stack
     * (_pendingOps) is host-private.
     */
    void bumpPendingGeneration();

    /**
     * GB13-006 (Requirement 4.4): invalidate any pending target/selection
     * reference that belongs to a character who ACTUALLY LEFT the party, so a
     * stale targeting/selection callback cannot act on the departed id.
     * Forwards to the host-private PendingOperationStack (the 8.2 hook). The
     * id is the departed member's stable party index. This is the removal-path
     * wiring the clearMonsters TODO deferred to this task; the combat-roster
     * removal id source stays owned by GB13-011 (task 12).
     */
    void invalidatePendingForCharacter(uint16 charId);

    /**
     * GB13-006 LOAD CHARACTER bit-7 removal (Inst_LOAD_CHAR /
     * PARTY_removeMember / RemoveCreature). Removes @p member from the engine
     * party using the stable capture-next-before-erase strategy, frees its
     * icon slot, updates selection + the VM character window and invalidates
     * pending references for the departed stable id. This is ONLY the
     * party-member removal action; it is deliberately distinct from temporary
     * monster teardown (clearMonsters) and disk-character deletion.
     *
     * @return true when a member was actually removed.
     */
    bool removeLoadedCharacter(Goldbox::Data::PlayerCharacter *member) override;

    /**
     * DIALOG_OpenDoor equivalent: attempt to open a door in the party's
     * current facing direction at the current position using RuntimeGeoBlock.
     * Returns true if a door was opened (wall flag cleared).
     */
    bool tryOpenDoor();

    /**
     * Play a sound effect by legacy sound index.
     * Maps to original PlaySound(ARRAY_SOUND_MAP[id]) / WORD_SOUND_ID_N.
     */
    void playSound(uint8 soundId);

    /**
     * Initialize full default skybox background palette.
     * Mirrors original GFX_SetDefaultColors(6,7,0,0xb) — sets ground,
     * horizon, and other fixed background layer colors. VM sky/ceiling
     * fields overlay on top of this after redraw.
     */
    void setDefaultSkyboxColors();

    bool hasStaticMapPayload() const { return _staticMapPayloadLoaded; }
    uint8 staticMapPayloadBlockId() const { return _staticMapPayloadBlockId; }
    Common::Span<const uint8> staticMapPayload() const {
        return Common::Span<const uint8>(_staticMapPayloadBuffer,
            ARRAYSIZE(_staticMapPayloadBuffer));
    }

private:
    uint8 allocateMonsterIconSlot() const;

    /**
     * GB13-010 — run post-combat settlement once, in the finalize seam BETWEEN
     * the GB13-011 outcome capture (teardownBattle) and the host monster free
     * (clearMonsters). Builds the immutable PostCombatInput from the still-live
     * roster (party/NPC entries first, host monsters last), runs the pure
     * settlement core (settlePostCombat), then applies the deterministic result
     * to the party XP fields, the member status transitions and the shared
     * TreasurePool (pooled battle currency). The loot/summary PRESENTATION
     * (TreasureDialog / GS_AFTER_COMBAT) is driven by the view layer and is not
     * wired here. Guarded by _combatSettled so a duplicated finalize completion
     * cannot award rewards twice.
     *
     * @param defeat true on a GS_END_GAME (party-defeated) teardown.
     */
    void settleCombatLoot(bool defeat);

    // GB13-006: keep the VM character window (party count + selected-pc index)
    // coherent with the engine party list and selection in one place.
    void _syncPartyWindow(Goldbox::Data::PlayerCharacter *selection);

    // GB13-002: present one synchronous encounter/parley hotkey menu and return
    // the selected index (-1 on break). Writes no VM memory, so a reopened
    // encounter prompt costs no premature write.
    int16 _runEncounterHotkeyMenu(const Common::Array<Common::String> &options);
    // WriteGbvm: one value per VM address, with the VM write hooks.
    void _writeVmValue(uint16 vmAddr, uint16 value);
    // MenuHorizontal animation of the PIC shown in the view.
    bool _tickMenuPicture();
    // DisplayMessageWindow inside the encounter loop: show the text and
    // pump frames until the text box has finished drawing it.
    void _showEncounterMessage(const Common::String &text, bool clearBox);

    // Skybox background layer colors (set by GFX_SetDefaultColors,
    // updated from VM $49FD/$49FE when scripts change them).
    uint8 _colorFloor   = 6;
    uint8 _colorHorizon = 7;
    uint8 _colorSkyline = 0;
    uint8 _colorSky     = 0x0b;

    // Owned DaxTile instances for walldef tile atlases (slots 1-3, 0-based idx)
    Common::ScopedPtr<Goldbox::Gfx::DaxTile> _walldefTiles[3];
    WallSetRuntimeState _wallSetStates[3];
    Common::List<Data::PoolradCharacter *> _loadedMonsters;
    bool _monstersAppendedToParty = false;
    Common::Array<uint8> _monsterIconSlots;
    uint8 _nextMonsterIconSlot = 26;

    // GB13-006: dynamic icon slots owned by recruited NPCs (ADD NPC). These are
    // PERSISTENT party-member icons, tracked separately from _monsterIconSlots
    // so temporary monster teardown (clearMonsters) never releases a recruited
    // NPC's icon. Each slot is released when the owning member leaves via
    // removeLoadedCharacter (LOAD CHARACTER bit 7).
    Common::Array<uint8> _recruitIconSlots;

    // GB13-009 (8.1): host-owned typed pending-operation stack. Introduced
    // ALONGSIDE the per-kind async booleans below; it generalizes them and the
    // hasPendingAsync()/isPendingAsyncReady()/finalizePendingAsync() hooks.
    // 8.1 wires the stack in without removing the booleans so current async
    // menu/shop/combat/delay/print flows keep working unchanged. The booleans
    // are migrated/retired in 8.2/8.3.
    ECL::PendingOperationStack _pendingOps;

    // Async VM_YIELD state for horizontal menu.
    bool _asyncMenuPending = false;
    uint16 _asyncMenuResultAddr = 0;
    Common::ScopedPtr<Goldbox::MenuItemList> _asyncMenuModel;
    Goldbox::UIElement *_asyncMenuSink = nullptr;
    Views::Dialogs::HorizontalMenu *_asyncHorizontalMenu = nullptr;

    // Async VM_YIELD state for shop/temple/treasure dialog.
    bool _asyncShopPending = false;
    mutable bool _asyncShopWasActivated = false;

    // Async VM_YIELD state for tactical combat view.
    bool _asyncCombatPending = false;

    // GB13-010: exactly-once settlement guard. Set when settleCombatLoot runs
    // for the active battle; cleared at the next startCombat so two
    // consecutive fights each settle once. A duplicated finalize completion
    // sees this true and skips re-awarding rewards.
    bool _combatSettled = false;
    mutable bool _asyncCombatWasActivated = false;

    // Async VM_YIELD state for DELAY opcode.
    bool _asyncDelayPending = false;
    uint32 _asyncDelayEndTime = 0;

    // Final sprite-stage portrait/picture reveal that should occur during
    // the following async DELAY, before the VM resumes past the delay.
    mutable bool _pendingEncounterHeadReveal = false;
    mutable uint32 _pendingEncounterHeadRevealTime = 0;

    // Async VM_YIELD state for PRINT/PRINTCLEAR letter-pacing.
    bool _asyncPrintPending = false;
    uint32 _asyncPrintEndTime = 0;

    // GB13-003: async VM_YIELD state for INPUT NUMBER / INPUT STRING. The
    // typed input kind and result address are owned here, mirroring the
    // _asyncMenu* booleans. A typed unsigned value carries the full 0..65535
    // domain separate from cancellation so an invalid/cancelled entry never
    // mutates VM memory.
    enum InputKind : uint8 {
        kInputNone = 0,
        kInputNumber,
        kInputString
    };
    bool _asyncInputPending = false;
    InputKind _asyncInputKind = kInputNone;
    uint16 _asyncInputResultAddr = 0;
    uint8 _asyncInputMaxLen = 0;
    Goldbox::UIElement *_asyncInputSink = nullptr;
    Views::Dialogs::HorizontalInput *_asyncInputDialog = nullptr;
    ECL::RequestKey _asyncInputKey;

    // GB13-003: async VM_YIELD state for VERTICAL MENU. Result address and the
    // decoded option list are owned here; success writes the index, cancel/
    // quit write nothing.
    bool _asyncVertMenuPending = false;
    uint16 _asyncVertMenuResultAddr = 0;
    Goldbox::UIElement *_asyncVertMenuSink = nullptr;
    Views::Dialogs::VerticalMenu *_asyncVertMenu = nullptr;
    Common::Array<Common::String> _asyncVertMenuOptions;
    Common::ScopedPtr<Goldbox::MenuItemList> _asyncVertMenuModel;
    ECL::RequestKey _asyncVertMenuKey;

    // GB13-003: async VM_YIELD state for WHO (party-member selection). WHO
    // selects the current character by stable identity -- the saved party
    // index resolves back to the owning PlayerCharacter* at finalize time. No
    // numeric VM destination is written.
    bool _asyncWhoPending = false;
    Goldbox::UIElement *_asyncWhoSink = nullptr;
    Views::Dialogs::PartySelector *_asyncWhoDialog = nullptr;
    ECL::RequestKey _asyncWhoKey;

    // Shared teardown for the GB13-003 input/menu/WHO async sinks.
    void teardownInputAsync();
    void teardownVertMenuAsync();
    void teardownWhoAsync();

    // Runtime static-map payload buffer (legacy PTR_GEO_BUFF equivalent).
    // Stores 4 x 0x100 map planes (NE, SW, events, doors) copied from GEO.
    // This is intentionally distinct from VM bank0 metadata fields.
    uint8 _staticMapPayloadBuffer[0x400];
    bool _staticMapPayloadLoaded = false;
    uint8 _staticMapPayloadBlockId = 0xFF;
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_ECL_POOLRAD_ENGINE_HOST_IMPL_H
