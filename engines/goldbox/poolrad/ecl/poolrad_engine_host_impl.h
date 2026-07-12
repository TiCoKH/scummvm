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
#include "common/scummsys.h"
#include "common/ptr.h"
#include "goldbox/ecl/ecl_syscall_impl.h"
#include "goldbox/ecl/ecl_memory.h"

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
    VmResult displayPicture(uint8 picID) override;
    VmResult drawEncounterStage(uint8 resourceId, uint8 distanceCap,
        uint8 variantId) override;
    VmResult redrawEncounterStage(uint8 newDistance) override;
    VmResult spriteOff() override;
    int16 horizontalMenu(const Common::Array<Common::String> &options) override;
    VmResult handleCallOpcode(uint16 callId) override;
    VmResult readGeoAtPosition() override;
    VmResult refreshViewport() override;
    VmResult beginDelay() override;
    VmResult advanceClock(uint8 amount) override;
    VmResult checkParty(uint16 attributeAddr, uint16 effectId,
        uint16 highAddr, uint16 lowAddr) override;
    VmResult hasEffect(uint8 effectId, uint16 resultAddr) override;
    bool hasEffectActive(uint8 effectId) const override;
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
    void buildUnifiedCombatRoster(
        Common::Array<Goldbox::Data::PlayerCharacter *> &roster,
        int &partyCount) const;

    uint8 allocateMonsterIconSlot() const;

    // Skybox background layer colors (set by GFX_SetDefaultColors,
    // updated from VM $49FD/$49FE when scripts change them).
    uint8 _colorFloor   = 6;
    uint8 _colorHorizon = 7;
    uint8 _colorSkyline = 0;
    uint8 _colorSky     = 0x0b;

    // Owned DaxTile instances for walldef tile atlases (slots 1-3, 0-based idx)
    Common::ScopedPtr<Goldbox::Gfx::DaxTile> _walldefTiles[3];
    WallSetRuntimeState _wallSetStates[3];
    Common::Array<Data::PoolradCharacter *> _loadedMonsters;
    // Non-owning enemy roster used as modern MONSTER_LOAD_READY signal.
    // Ownership remains in _loadedMonsters.
    Common::Array<Goldbox::Data::PlayerCharacter *> _enemy;
    Common::Array<Goldbox::Data::PlayerCharacter *> _combatRoster;
    Common::Array<uint8> _monsterIconSlots;
    uint8 _nextMonsterIconSlot = 26;

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
