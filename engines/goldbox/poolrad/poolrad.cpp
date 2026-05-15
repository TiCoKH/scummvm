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

#include "common/config-manager.h"
#include "common/engine_data.h"
#include "common/fs.h"
#include "goldbox/gfx/surface.h"
#include "goldbox/gfx/dax_font.h"
#include "goldbox/gfx/dax_tile.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/data/strings_data.h"
#include "goldbox/poolrad/data/poolrad_vm_layout.h"
#include "goldbox/poolrad/poolrad.h"
//#include "goldbox/poolrad/gfx/cursors.h"

#include "goldbox/poolrad/console.h"

namespace Goldbox {
namespace Poolrad {

namespace {

static bool isWildernessMapId(uint8 mapId) {
	return mapId == 0x19 || mapId == 0x1A || mapId == 0x1B;
}

} // namespace

PoolradEngine *g_engine;

//Data::Character PoolradEngine::_party[MAX_CHARACTERS];

PoolradEngine::PoolradEngine(OSystem *syst, const GoldboxGameDescription *gameDesc) :
		Goldbox::Engine(syst, gameDesc) {
	g_engine = this;
}

PoolradEngine::~PoolradEngine() {
	g_engine = nullptr;
	delete _views;
	delete _iconManager;
}

void PoolradEngine::initializePath(const Common::FSNode &gamePath) {
	Engine::initializePath(gamePath);
	//SearchMan.addDirectory("data", gamePath.getChild("rom").getChild("data"), 0, 3);
}

void PoolradEngine::setup() {

// Initialise engine data for the game
//	Common::U32String errMsg;
//	if (!Common::load_engine_data("poolrad.dat", "poolrad", 1, 0, errMsg)) {
//		Common::String msg(errMsg);
//		error("%s", msg.c_str());
//	}
	Common::File items;
	if (!items.open("ITEMS")) {
		warning("Cannot open ITEMS file");
		return;
	}
	Common::SeekableReadStream &in = items;
	Engine::gItemProps.load(in);
//	Engine::gItemProps.debugStorage(); //TODO: Remove this line later
	items.close();

	Surface::setupPalette();

	// Load all 8x8d files via DaxFileManager (auto-sorts to correct container)
	getDaxManager().loadFile(Common::Path("8x8d1.dax"));
	getDaxManager().loadFile(Common::Path("8x8d2.dax"));
	getDaxManager().loadFile(Common::Path("8x8d3.dax"));
	getDaxManager().loadFile(Common::Path("8x8d4.dax"));
	getDaxManager().loadFile(Common::Path("8x8d5.dax"));
	getDaxManager().loadFile(Common::Path("8x8d6.dax"));
	getDaxManager().loadFile(Common::Path("8x8d7.dax"));
	getDaxManager().loadFile(Common::Path("8x8d8.dax"));

	// Load tile-based DAX files
	getDaxManager().loadFile(Common::Path("bacpac.dax"));
	getDaxManager().loadFile(Common::Path("dungcom.dax"));
	getDaxManager().loadFile(Common::Path("randcom.dax"));
	getDaxManager().loadFile(Common::Path("sqrpaci.dax"));
	getDaxManager().loadFile(Common::Path("wildcom.dax"));

	// Load geo and monster resource DAX files
	getDaxManager().loadFile(Common::Path("geo1.dax"));
	getDaxManager().loadFile(Common::Path("geo2.dax"));
	getDaxManager().loadFile(Common::Path("geo3.dax"));
	getDaxManager().loadFile(Common::Path("geo4.dax"));
	getDaxManager().loadFile(Common::Path("geo5.dax"));
	getDaxManager().loadFile(Common::Path("geo6.dax"));
	getDaxManager().loadFile(Common::Path("geo7.dax"));
	getDaxManager().loadFile(Common::Path("geo8.dax"));
	getDaxManager().loadFile(Common::Path("cpic1.dax"));
	getDaxManager().loadFile(Common::Path("cpic2.dax"));
	getDaxManager().loadFile(Common::Path("cpic3.dax"));
	getDaxManager().loadFile(Common::Path("cpic4.dax"));
	getDaxManager().loadFile(Common::Path("cpic5.dax"));
	getDaxManager().loadFile(Common::Path("cpic6.dax"));
	getDaxManager().loadFile(Common::Path("cpic7.dax"));
	getDaxManager().loadFile(Common::Path("cpic8.dax"));
	getDaxManager().loadFile(Common::Path("mon1cha.dax"));
	getDaxManager().loadFile(Common::Path("mon2cha.dax"));
	getDaxManager().loadFile(Common::Path("mon3cha.dax"));
	getDaxManager().loadFile(Common::Path("mon4cha.dax"));
	getDaxManager().loadFile(Common::Path("mon5cha.dax"));
	getDaxManager().loadFile(Common::Path("mon6cha.dax"));
	getDaxManager().loadFile(Common::Path("mon7cha.dax"));
	getDaxManager().loadFile(Common::Path("mon8cha.dax"));
	getDaxManager().loadFile(Common::Path("mon1itm.dax"));
	getDaxManager().loadFile(Common::Path("mon2itm.dax"));
	getDaxManager().loadFile(Common::Path("mon3itm.dax"));
	getDaxManager().loadFile(Common::Path("mon4itm.dax"));
	getDaxManager().loadFile(Common::Path("mon5itm.dax"));
	getDaxManager().loadFile(Common::Path("mon6itm.dax"));
	getDaxManager().loadFile(Common::Path("mon7itm.dax"));
	getDaxManager().loadFile(Common::Path("mon8itm.dax"));
	getDaxManager().loadFile(Common::Path("mon2spc.dax"));
	getDaxManager().loadFile(Common::Path("mon4spc.dax"));
	getDaxManager().loadFile(Common::Path("mon5spc.dax"));
	getDaxManager().loadFile(Common::Path("mon6spc.dax"));
	getDaxManager().loadFile(Common::Path("mon7spc.dax"));
	getDaxManager().loadFile(Common::Path("mon8spc.dax"));
	getDaxManager().loadFile(Common::Path("item1.dax"));
	getDaxManager().loadFile(Common::Path("item2.dax"));
	getDaxManager().loadFile(Common::Path("item3.dax"));
	getDaxManager().loadFile(Common::Path("item4.dax"));
	getDaxManager().loadFile(Common::Path("item5.dax"));
	getDaxManager().loadFile(Common::Path("item6.dax"));
	getDaxManager().loadFile(Common::Path("item7.dax"));
	getDaxManager().loadFile(Common::Path("item8.dax"));
	getDaxManager().loadFile(Common::Path("sprit1.dax"));
	getDaxManager().loadFile(Common::Path("sprit2.dax"));
	getDaxManager().loadFile(Common::Path("sprit3.dax"));
	getDaxManager().loadFile(Common::Path("sprit4.dax"));
	getDaxManager().loadFile(Common::Path("sprit5.dax"));
	getDaxManager().loadFile(Common::Path("sprit6.dax"));
	getDaxManager().loadFile(Common::Path("sprit7.dax"));
	getDaxManager().loadFile(Common::Path("sprit8.dax"));

	getDaxManager().loadFile(Common::Path("walldef1.dax"));
	getDaxManager().loadFile(Common::Path("walldef2.dax"));
	getDaxManager().loadFile(Common::Path("walldef3.dax"));
	getDaxManager().loadFile(Common::Path("walldef4.dax"));
	getDaxManager().loadFile(Common::Path("walldef5.dax"));
	getDaxManager().loadFile(Common::Path("walldef6.dax"));
	getDaxManager().loadFile(Common::Path("walldef7.dax"));
	getDaxManager().loadFile(Common::Path("walldef8.dax"));

	// Load DAX Pic files
	getDaxManager().loadFile(Common::Path("body1.dax"));
	getDaxManager().loadFile(Common::Path("body2.dax"));
	getDaxManager().loadFile(Common::Path("body3.dax"));
	getDaxManager().loadFile(Common::Path("body4.dax"));
	getDaxManager().loadFile(Common::Path("body5.dax"));
	getDaxManager().loadFile(Common::Path("body6.dax"));
	getDaxManager().loadFile(Common::Path("body7.dax"));
	getDaxManager().loadFile(Common::Path("body8.dax"));

	getDaxManager().loadFile(Common::Path("head1.dax"));
	getDaxManager().loadFile(Common::Path("head2.dax"));
	getDaxManager().loadFile(Common::Path("head3.dax"));
	getDaxManager().loadFile(Common::Path("head4.dax"));
	getDaxManager().loadFile(Common::Path("head5.dax"));
	getDaxManager().loadFile(Common::Path("head6.dax"));
	getDaxManager().loadFile(Common::Path("head7.dax"));
	getDaxManager().loadFile(Common::Path("head8.dax"));

	getDaxManager().loadFile(Common::Path("title.dax"));

	getDaxManager().loadFile(Common::Path("cbody.dax"));
	getDaxManager().loadFile(Common::Path("chead.dax"));
	getDaxManager().loadFile(Common::Path("comspr.dax"));


	// Initialize icon manager
	_iconManager = new Gfx::IconManager();

	// Populate daxFont from container
	Goldbox::Data::DaxBlock *pc_font = getDax8x8d().getBlockById(201);
	if (!pc_font) {
		error("Failed to load font block 201 from 8x8d container");
	}
	auto daxFont = new Goldbox::Gfx::DaxFont(dynamic_cast<Goldbox::Data::DaxBlock8x8D*>(pc_font));
	_font = daxFont;

	// Engine-wide fixed tile cache preload (original Goldbox behavior):
	// slot 4 <- symbols block 202, slot 0 <- block 203.
	initFixedTileCacheSlots(202, 203);


	if (!_strings.load("global_strings.yml")){
		error("Failed to open global_strings.yml");
	}



/*
	// Load save data
	int saveSlot = ConfMan.getInt("save_slot");
	_saved.load();
	if (saveSlot != -1)
		(void)loadGameState(saveSlot);
*/
	//_pics.load("allpics1");

	// Setup game views
	_views = new Views::Views();
	addView("Title");

	// Phase 1: initialise ECL VM runtime
	// 1. Construct VM (memory is owned by VM; no syscall handler yet).
	_eclVm.reset(new ECL::EclVM(&_eclConfig));
	// 2. Construct game host (needs a pointer into VM memory).
	_eclHost.reset(new PoolradEngineHostImpl(this, &_eclVm->getMemory()));
	// 3. Wire host as the VM's syscall dispatch target.
	_eclVm->setSyscallHandler(_eclHost.get());
}

void PoolradEngine::onGameStateEnter(GameState prev, GameState next) {
	Views::View *view = nullptr;

	if (isMapRuntimeState(next) && (!isMapRuntimeState(prev) || prev != next)) {
		// VM runtime bootstrap is orchestrator-owned and happens in tick().
		_mapRuntimeNeedsInit = true;
	}

	switch (next) {
	case GS_START_MENU:
		replaceView("Title", true);
		view = dynamic_cast<Views::View *>(findView("Title"));
		if (view) {
			view->onEnter(next);
		}
		break;
	case GS_SHOP:
		// InGameView kModeShop: drawMainScreenWindows(true) + NPC portrait at (3,3).
		replaceView("InGame");
		view = dynamic_cast<Views::View *>(findView("InGame"));
		if (view)
			view->onEnter(next);
		break;
	case GS_CAMPING:
		// InGameView kModeCamping: drawMainScreenWindows(true) + camp state area.
		replaceView("InGame");
		view = dynamic_cast<Views::View *>(findView("InGame"));
		if (view)
			view->onEnter(next);
		break;
	case GS_DUNGEON_MAP:
		// InGameView kModeDungeon: drawMainScreenWindows(true) + 3D view + party panel.
		replaceView("InGame");
		view = dynamic_cast<Views::View *>(findView("InGame"));
		if (view)
			view->onEnter(next);
		break;
	case GS_WILDERNESS_MAP:
		// InGameView kModeWilderness: drawMainScreenWindows(false) + area-map block.
		replaceView("InGame");
		view = dynamic_cast<Views::View *>(findView("InGame"));
		if (view)
			view->onEnter(next);
		break;
	case GS_AFTER_COMBAT:
		// InGameView kModeAfterCombat: drawMainScreenWindows(true) + loot panel.
		replaceView("InGame");
		view = dynamic_cast<Views::View *>(findView("InGame"));
		if (view)
			view->onEnter(next);
		break;
	case GS_COMBAT:
		// InGameView kModeCombat: layout to be defined.
		replaceView("InGame");
		view = dynamic_cast<Views::View *>(findView("InGame"));
		if (view)
			view->onEnter(next);
		break;
	case GS_END_GAME:
		// Placeholder: go back to title; layout irrelevant.
		replaceView("Title", true);
		view = dynamic_cast<Views::View *>(findView("Title"));
		if (view) {
			view->onEnter(next);
		}
		break;
	}
}

GUI::Debugger *PoolradEngine::getConsole() {
	return new Console();
}

ECL::AddressSpace *PoolradEngine::getEclMemory() {
	if (!_eclVm)
		return nullptr;
	return &_eclVm->getMemory();
}

const ECL::AddressSpace *PoolradEngine::getEclMemory() const {
	if (!_eclVm)
		return nullptr;
	return &_eclVm->getMemory();
}

Data::DaxBlockGeo *PoolradEngine::getGeoBlockById(uint8 mapId) {
	return dynamic_cast<Data::DaxBlockGeo *>(
		getDaxGeo().getBlockById(mapId));
}

Data::DaxBlockGeo *PoolradEngine::getActiveGeoBlock() {
	return getGeoBlockById(_legacySharedState.byteMapId);
}

bool PoolradEngine::getActiveMapPosition(uint16 &x, uint16 &y,
		uint8 &dir) const {
	if (!_eclVm)
		return false;

	const ECL::AddressSpace &mem = _eclVm->getMemory();
	const VmGlobalLayout &layout = Data::getPoolradGlobalVmLayout();
	const uint16 xAddr = layout.field(kVmGlobalFieldDungeonX).vmAddr;
	const uint16 yAddr = layout.field(kVmGlobalFieldDungeonY).vmAddr;
	const uint16 dirAddr = layout.field(kVmGlobalFieldDungeonDir).vmAddr;

	x = mem.read16LE(xAddr);
	y = mem.read16LE(yAddr);
	dir = static_cast<uint8>((mem.read16LE(dirAddr) & 0x03) * 2);
	return true;
}

bool PoolradEngine::getDebugWallSetState(int slot,
		DebugWallSetState &state) const {
	if (!_eclHost || slot < 1 || slot > 3)
		return false;

	const PoolradEngineHostImpl::WallSetRuntimeState &hostState =
		_eclHost->wallSetState(slot);
	state.loaded = hostState.loaded;
	state.walldefBlockId = hostState.walldefBlockId;
	state.tileBlockId = hostState.tileBlockId;
	state.chunkIndex = hostState.chunkIndex;
	return true;
}

bool PoolradEngine::tick() {
	bool handled = Goldbox::Events::tick();

	if (_mapRuntimeNeedsInit && !_eclFlags.suspended &&
			isMapRuntimeState(getGameState())) {
		initializeMapRuntimeForState(getGameState());
	}

	if (_eclFlags.suspended && _eclHost && _eclVm) {
		if (!_eclHost->hasPendingAsync()) {
			_eclFlags.suspended = false;
		} else if (_eclHost->isPendingAsyncReady()) {
			const VmResult finalizeResult = _eclHost->finalizePendingAsync();
			if (finalizeResult == VM_OK) {
				_eclFlags.suspended = false;
				const VmResult resume =
					executeEclAtScriptAddress(_eclVm->getPC());
				if (resume == VM_YIELD)
					_eclFlags.suspended = true;
			} else if (finalizeResult == VM_ERROR || finalizeResult == VM_HALTED) {
				_eclFlags.suspended = false;
			}
		}
	}

	if (isMapRuntimeState(getGameState())) {
		processLegacyInGameLoopStep();
		refreshLegacySharedRuntimeState();
	}

	return handled;
}

VmResult PoolradEngine::executeEclAtScriptAddress(uint16 scriptPc,
		uint32 maxSteps) {
	if (!_eclVm)
		return VM_ERROR;

	const VmResult r = _eclVm->runAtScriptAddress(scriptPc, maxSteps);
	_eclFlags.suspended = (r == VM_YIELD);
	return r;
}

bool PoolradEngine::isMapRuntimeState(GameState state) const {
	return state == GS_DUNGEON_MAP || state == GS_WILDERNESS_MAP
		|| state == GS_SHOP || state == GS_CAMPING
		|| state == GS_AFTER_COMBAT || state == GS_COMBAT;
}

Views::InGameView *PoolradEngine::getInGameView() {
	UIElement *view = findView("InGame");
	if (!view)
		return nullptr;
	return dynamic_cast<Views::InGameView *>(view);
}

void PoolradEngine::initializeMapRuntimeForState(GameState state) {
	_mapRuntimeNeedsInit = false;

	_eclFlags.wallsetReady = false;
	_eclFlags.geoReady = false;
	_eclFlags.mapDataReady = false;
	_eclFlags.screenRefresh = true;
	_eclFlags.eclReady = false;
	_eclFlags.suspended = false;

	if (!_eclVm)
		return;

	_eclVm->wallsetReady = false;
	_eclVm->geoReady = false;
	_eclVm->mapdataInload = false;
	_eclVm->characterInload = false;
	_eclVm->eclReady = false;
	_eclVm->screenRefresh = true;

	ECL::EclLayoutAccess layout = _eclConfig.getLayoutAccess();
	ECL::AddressSpace &mem = _eclVm->getMemory();
	const uint16 mapAddr = layout.vmField(kVmFieldSavedMapId).vmAddr;
	const uint16 indoorAddr = layout.vmField(kVmFieldIndoorModeFlag).vmAddr;
 const uint16 rtGameStateAddr = layout.runtimeField(ECL::kEclRuntimeGameState);

	const uint8 mapId = mem.read8(mapAddr);
	_legacySharedState.byteMapId = mapId;
	if (isWildernessMapId(mapId) && mem.read8(indoorAddr) == 0)
		_legacySharedState.byteGameState = GS_WILDERNESS_MAP;
	else
		_legacySharedState.byteGameState = state;

	if (ECL::EclRuntimeLayout::isValidVmAddr(rtGameStateAddr)) {
		mem.write8(rtGameStateAddr,
			static_cast<uint8>(_legacySharedState.byteGameState));
	}

	// Dispatch ON_INIT at runtime bootstrap point (ENGINE_Execute(ECL_ONINIT)).
	(void)runEclEntryPoint(ECL::kEclRuntimeOnInitEntry);
}

void PoolradEngine::refreshLegacySharedRuntimeState() {
	_legacySharedState.byteGameState = getGameState();
	_legacySharedState.ptrCharacter = getSelectedCharacter();
	_legacySharedState.boolSuspendFlag = _eclFlags.suspended;

	if (!_eclVm)
		return;

	ECL::EclLayoutAccess layout = _eclConfig.getLayoutAccess();
	ECL::AddressSpace &mem = _eclVm->getMemory();

	_legacySharedState.boolStateLoaded = _eclVm->stateLoaded;

	const uint16 mapAddr = layout.vmField(kVmFieldSavedMapId).vmAddr;
	_legacySharedState.byteMapId = mem.read8(mapAddr);

	const uint16 skyboxRedrawAddr =
		layout.runtimeField(ECL::kEclRuntimeSkyboxRedrawFlag);
	if (ECL::EclRuntimeLayout::isValidVmAddr(skyboxRedrawAddr)) {
		_legacySharedState.bool3dRedraw =
			(mem.read8(skyboxRedrawAddr) != 0);
	}

	const uint16 picHeadAddr =
		layout.vmGlobalField(kVmGlobalFieldPictureHeadId).vmAddr;
	_legacySharedState.boolPictureReady = (mem.read8(picHeadAddr) != 0xFF);

	_eclFlags.eclReady = _eclVm->eclReady;
	_eclFlags.geoReady = _eclVm->geoReady;
	_eclFlags.wallsetReady = _eclVm->wallsetReady;
	_eclFlags.screenRefresh = _eclVm->screenRefresh;
	_eclFlags.mapDataReady = _eclVm->geoReady && _eclVm->wallsetReady;
}

VmResult PoolradEngine::runEclEntryPoint(ECL::EclRuntimeFieldId entryField,
		uint32 maxSteps) {
	if (!_eclVm)
		return VM_ERROR;

	ECL::EclLayoutAccess layout = _eclConfig.getLayoutAccess();
	ECL::AddressSpace &mem = _eclVm->getMemory();
	const uint16 entryAddr = layout.runtimeField(entryField);
	if (!ECL::EclRuntimeLayout::isValidVmAddr(entryAddr))
		return VM_ERROR;

	const uint16 entryPc = mem.read16LE(entryAddr);
	if (entryPc == 0)
		return VM_OK;

	const VmResult result = executeEclAtScriptAddress(entryPc, maxSteps);
	_eclFlags.eclReady = _eclVm->eclReady;
	return result;
}

void PoolradEngine::processLegacyInGameLoopStep() {
	if (!_eclVm || _eclFlags.suspended)
		return;

	Views::InGameView *inGameView = getInGameView();
	if (!inGameView || !inGameView->hasPendingCommand())
		return;

	const Views::InGameView::InGameCommand cmd =
		inGameView->consumePendingCommand();
	if (cmd == Views::InGameView::kCmdNone)
		return;

	ECL::EclLayoutAccess layout = _eclConfig.getLayoutAccess();
	ECL::AddressSpace &mem = _eclVm->getMemory();

	if (cmd == Views::InGameView::kCmdEncamp) {
		const VmResult r = runEclEntryPoint(ECL::kEclRuntimeOnRestEntry);
		if (r == VM_YIELD)
			_eclFlags.suspended = true;
		return;
	}

	if (cmd == Views::InGameView::kCmdSearch) {
		const uint16 searchAddr =
			layout.vmGlobalField(kVmGlobalFieldSearchFlags).vmAddr;
		const uint8 savedSearchFlag = static_cast<uint8>(mem.read8(searchAddr) & 1);
		mem.write8(searchAddr, 1);

		const VmResult onSearch = runEclEntryPoint(ECL::kEclRuntimeOnSearchEntry);
		mem.write8(searchAddr, savedSearchFlag);
		if (onSearch == VM_YIELD) {
			_eclFlags.suspended = true;
			return;
		}

		if (_eclVm->eclReady) {
			const VmResult resume = executeEclAtScriptAddress(_eclVm->getPC());
			if (resume == VM_YIELD)
				_eclFlags.suspended = true;
		}
		return;
	}

	if (cmd != Views::InGameView::kCmdMove)
		return;

	const VmResult onMove = runEclEntryPoint(ECL::kEclRuntimeOnMoveEntry);
	if (onMove == VM_YIELD) {
		_eclFlags.suspended = true;
		return;
	}

	if (!_eclVm->eclReady) {
		// Legacy post-ONMOVE branch when ECL has not entered ready state yet:
		// force redraw bookkeeping and run ON_SEARCH once.
		_legacySharedState.bool3dRedraw = false;
		_legacySharedState.boolPictureReady = true;
		const VmResult onSearch = runEclEntryPoint(ECL::kEclRuntimeOnSearchEntry);
		if (onSearch == VM_YIELD)
			_eclFlags.suspended = true;
		return;
	}

	const VmResult resume = executeEclAtScriptAddress(_eclVm->getPC());
	if (resume == VM_YIELD)
		_eclFlags.suspended = true;
}

} // namespace Poolrad
} // namespace Goldbox
