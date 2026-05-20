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

#ifndef GOLDBOX_POOLRAD_POOLRAD_H
#define GOLDBOX_POOLRAD_POOLRAD_H

#include "goldbox/engine.h"
#include "goldbox/poolrad/views/views.h"
#include "goldbox/poolrad/views/mainscreen_view.h"
#include "goldbox/gfx/icon_manager.h"
#include "goldbox/poolrad/effect_handler.h"
#include "goldbox/gfx/walldef_surface_builder.h"
#include "goldbox/ecl/ecl_vm.h"
#include "goldbox/runtime/runtime_exchange.h"
#include "goldbox/poolrad/ecl/poolrad_game_config.h"
#include "goldbox/poolrad/ecl/poolrad_engine_host_impl.h"
#include "goldbox/ecl/runtime_layout.h"
#include "goldbox/data/daxblock.h"
#include "common/ptr.h"
//#include "goldbox/poolrad/data/character.h"
//#include "goldbox/poolrad/files/game_archive.h"
//#include "goldbox/poolrad/data/saved.h"
//#include "goldbox/poolrad/gfx/pics.h"

namespace Goldbox {
namespace Data {
}
}

namespace Goldbox {
namespace Poolrad {

class PoolradRuntimeExchange;

namespace Data {
typedef ::Goldbox::Data::DaxBlockGeo DaxBlockGeo;
}

struct DebugWallSetState {
	bool loaded = false;
	uint8 walldefBlockId = 0xFF;
	uint8 tileBlockId = 0xFF;
	uint8 chunkIndex = 0;
};

const int MAX_CHARACTERS = 8;
const int MAX_PC_IN_PARTY = 6;
const int MAX_NPC_IN_PARTY = 2;

class PoolradEngine : public Goldbox::Engine {
private:
	Poolrad::Views::Views *_views = nullptr;
	Gfx::IconManager *_iconManager = nullptr;
	uint16 _mapX = 0, _mapY = 0;
	EffectHandler _effectsRuntime;

	// -------------------------------------------------------------------
	// ECL VM runtime
	// -------------------------------------------------------------------

	/** Flags tracking async ECL completion states. */
	struct EclRuntimeFlags {
		bool wallsetReady  = false;
		bool geoReady      = false;
		bool mapDataReady  = false;
		bool screenRefresh = false;
		bool eclReady      = false;
		bool suspended     = false;
	};

	PoolradGameConfig                           _eclConfig;
	Common::ScopedPtr<ECL::EclVM>              _eclVm;
	Common::ScopedPtr<PoolradEngineHostImpl>   _eclHost;
	Common::ScopedPtr<PoolradRuntimeExchange>  _runtimeExchange;
	EclRuntimeFlags                            _eclFlags;
	bool                                       _mapRuntimeNeedsInit = false;

	/**
	 * Shared engine/VM runtime state (legacy globals mirror).
	 *
	 * This intentionally centralizes variables used by both the ECL VM and
	 * host UI loop, mirroring original GB_EngineMain globals:
	 * BYTE_GAME_STATE, BOOL_STATE_LOADED, BYTE_MAP_ID, PTR_CHARACTER,
	 * BOOL_SUSPEND_FLAG, BOOL_3D_REDRAW, BOOL_PICTURE_READY.
	 */
	struct LegacySharedRuntimeState {
		GameState byteGameState = GS_START_MENU;
		bool boolStateLoaded = false;
		uint8 byteMapId = 0;
		void *ptrCharacter = nullptr;
		bool boolSuspendFlag = false;
		bool bool3dRedraw = true;
		bool boolPictureReady = false;
	};

	LegacySharedRuntimeState _legacySharedState;

	bool isMapRuntimeState(GameState state) const;
	Views::InGameView *getInGameView();
	void initializeMapRuntimeForState(GameState state);
	void refreshLegacySharedRuntimeState();
	VmResult runEclEntryPoint(ECL::EclRuntimeFieldId entryField,
			uint32 maxSteps = 1000000);
	void processLegacyInGameLoopStep();

protected:
	void setup() override;
	GUI::Debugger *getConsole() override;
	void onGameStateEnter(GameState prev, GameState next) override;
	bool tick() override;

public:

	//GameArchive *_gameArchive = nullptr;
	//Data::Saved _saved;
	//Gfx::PicsDecoder _pics;

public:
	//static Data::Character _party[MAX_CHARACTERS];

	PoolradEngine(OSystem *syst, const GoldboxGameDescription *gameDesc);
	~PoolradEngine() override;
	void initializePath(const Common::FSNode &gamePath) override;

	/**
	 * Get the global icon manager.
	 * @return Pointer to the icon manager
	 */
	Gfx::IconManager *getIconManager() const { return _iconManager; }
	EffectHandler &effectsRuntime() { return _effectsRuntime; }
	ECL::AddressSpace *getEclMemory();
	const ECL::AddressSpace *getEclMemory() const;
	bool captureRuntimeMapSnapshot(::Goldbox::RuntimeMapSnapshot &snapshot) const;
	Data::DaxBlockGeo *getGeoBlockById(uint8 mapId);
	Data::DaxBlockGeo *getActiveGeoBlock();
	bool getActiveMapPosition(uint16 &x, uint16 &y, uint8 &dir) const;
	bool saveGameSlotX86(char slotLetter, Common::String &errorMessage);
	bool getDebugWallSetState(int slot, DebugWallSetState &state) const;
	bool queueInGameCommand(Views::InGameView::InGameCommand cmd);

	/**
	 * Execute ECL bytecode from an absolute script VM address.
	 * Returns VM_YIELD when an async syscall suspends the VM.
	 */
	VmResult executeEclAtScriptAddress(uint16 scriptPc,
			uint32 maxSteps = 1000000);
	RuntimeExchange *getRuntimeExchange() override;
	const RuntimeExchange *getRuntimeExchange() const override;
	const LegacySharedRuntimeState &getLegacySharedRuntimeState() const {
		return _legacySharedState;
	}
};

extern PoolradEngine *g_engine;

} // namespace Poolrad
} // namespace Goldbox

#endif
