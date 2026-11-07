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

#include "common/engine_data.h"
#include "common/fs.h"
#include "common/file.h"
#include "common/str.h"
#include "goldbox/gfx/surface.h"
#include "goldbox/gfx/dax_font.h"
#include "goldbox/gfx/dax_tile.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/data/pascal_string_buffer.h"
#include "goldbox/data/strings_data.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/data/legacy_save_utils.h"
#include "goldbox/poolrad/data/poolrad_rules_provider.h"
#include "goldbox/poolrad/data/poolrad_vm_layout.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/poolrad/native_save.h"
#include "goldbox/poolrad/poolrad_runtime_exchange.h"
#include "goldbox/game_factory.h"
#include "goldbox/spells/spell_casting.h"
#include "goldbox/runtime/runtime_time.h"
#include "goldbox/poolrad/rest_operation.h"
#include "goldbox/poolrad/wilderness_travel.h"
#include "goldbox/poolrad/wilderness_map.h"
#include "goldbox/poolrad/wilderness_atlas.h"
#include "goldbox/poolrad/spell_preparation.h"
#include "goldbox/poolrad/program_services.h"
#include "goldbox/core/direction.h"
#include "goldbox/core/coords.h"
//#include "goldbox/poolrad/gfx/cursors.h"

#include "goldbox/poolrad/console.h"

#include <stdio.h>

namespace Goldbox {
namespace Poolrad {

namespace {

static char toUpperAscii(char c) {
	if (c >= 'a' && c <= 'z')
		return static_cast<char>(c - ('a' - 'A'));
	return c;
}

// GB13-024: host binding for the YarashComplete one-time world-state op. The
// persistent pyramid-left/river-open flag is the engine-side mirror
// (LegacySharedRuntimeState::boolYarashPyramidLeft) passed by reference so the
// once-only guard and the river gate share ONE source. The mission byte comes
// from VM memory.
//
// OPEN (trace-gated): the mission VM address and the pyramid-left VM bit are
// unknown and NOT guessed. mission() reports the kMissionYarashComplete
// sentinel from the persistent mirror until the mission field is traced, so
// the confirmable once-only STRUCTURE is exercised end to end without an
// invented address; bind mission()/the flag to the real VM world-state once
// confirmed.
class HostYarashWorldState : public WildernessMap::WorldState {
public:
	HostYarashWorldState(ECL::AddressSpace &mem, ECL::EclLayoutAccess layout,
			bool &persistentPyramidLeft)
		: _mem(mem), _layout(layout),
		  _pyramidLeft(persistentPyramidLeft) {
		(void)_mem;
		(void)_layout;
	}

	uint8 mission() override {
		// OPEN: no confirmed mission VM field. Report the central-region
		// YarashComplete sentinel so the gate STRUCTURE runs; replace with the
		// confirmed mission read once traced.
		return WildernessMap::kMissionYarashComplete;
	}

	bool yarashPyramidLeft() override {
		return _pyramidLeft;
	}

	void setYarashPyramidLeft(bool open) override {
		_pyramidLeft = open;
	}

private:
	ECL::AddressSpace &_mem;
	ECL::EclLayoutAccess _layout;
	bool &_pyramidLeft;
};

static const Goldbox::TimeFieldAddresses kPoolradClockAddrs = {{
	0x49C5, // units
	0x49C7, // minuteOnes
	0x49C8, // minuteTens
	0x49C9, // hour
	0x49CA, // day
	0x49CB, // month
	0x49CC  // yearLo
}};

static const uint8 kCampfirePicId = 29;

} // namespace

PoolradEngine *g_engine;

int kForceLinkPoolrad = 0;

namespace {
GameFactory::Registrar<PoolradEngine> sPoolradRegistrar(GAMETYPE_POOLRAD);
} // namespace

//Data::Character PoolradEngine::_party[MAX_CHARACTERS];

PoolradEngine::PoolradEngine(OSystem *syst, const GoldboxGameDescription *gameDesc) :
		Goldbox::Engine(syst, gameDesc) {
	g_engine = this;
	Goldbox::Data::Rules::setRulesProvider(&PoolradRulesProvider::instance());
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
	// Keep the framework entrypoint explicit in Poolrad while reusing
	// the generic phased setup pipeline from Goldbox::Engine.
	Engine::setup();
}

void PoolradEngine::initGameDefaults() {
    if (!_eclVm)
        return;

    ECL::AddressSpace &mem = _eclVm->getMemory();
    ECL::EclLayoutAccess layout = _eclConfig.getLayoutAccess();

    // Mirror GAME_Init: zero all 4 VM banks.
    // VM constructor already zeroes flat memory, but be explicit for clarity.
    // Bank ranges: GEO 0x4900-0x4CFF, DAT 0x6B00-0x6EFF,
    //              HEAP 0x9700-0x98FF, ECL 0x9900-0xB6FF.

    // Default position: x=15, y=1, dir=WEST(2)
    mem.write8(layout.vmGlobalField(kVmGlobalFieldDungeonX).vmAddr, 15);
    mem.write8(layout.vmGlobalField(kVmGlobalFieldDungeonY).vmAddr, 1);
    mem.write8(layout.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr, 2); // WEST

    // Map type defaults
    mem.write8(layout.vmGlobalField(kVmGlobalFieldMapWallType).vmAddr, 1);

    // Party count = 0
    mem.write8(layout.vmGlobalField(kVmGlobalFieldPartyCount).vmAddr, 0);

    // Picture head ID sentinel (0xFF = no picture loaded)
    mem.write8(layout.vmGlobalField(kVmGlobalFieldPictureHeadId).vmAddr, 0xFF);

    // Game speed default = 1
    mem.write8(layout.vmField(kVmFieldGameSpeed).vmAddr, 1);

    // Indoor mode = 1 (dungeon), map type = 1
    mem.write8(layout.vmField(kVmFieldIndoorModeFlag).vmAddr, 1);

    // Runtime game state = GS_START_MENU (0) while at main menu
    mem.write8(layout.runtimeField(ECL::kEclRuntimeGameState),
        static_cast<uint8>(GS_START_MENU));
	if (g_events) {
		g_events->postEclStateMessage(EclVmMessage::ST_GAME_STATE,
			static_cast<uint16>(GS_START_MENU), EclVmMessage::VT_UINT8);
	}

    // GAME_Init color register defaults (pre-scenario startup values).
    mem.write8(layout.vmGlobalField(kVmGlobalFieldColorFlagFloor).vmAddr, 219);
    mem.write8(layout.vmGlobalField(kVmGlobalFieldColorFlagHorizon).vmAddr, 232);

    // WORD_ECL_PC = 0x9900 (script start, matches original)
    mem.write16LE(layout.runtimeField(ECL::kEclRuntimePc), 0x9900);

    // Legacy shared state mirrors
    _legacySharedState.byteGameState = GS_START_MENU;
    _legacySharedState.boolStateLoaded = false;
    _legacySharedState.byteMapId = 0;
    _legacySharedState.byteMenuStatus = 0;
    _legacySharedState.ptrCharacter = nullptr;
    _legacySharedState.boolSuspendFlag = false;
    _legacySharedState.bool3dRedraw = false;
    _legacySharedState.boolPictureReady = false;
}

bool PoolradEngine::initializeGameData() {
	// Initialise engine data for the game
//	Common::U32String errMsg;
//	if (!Common::load_engine_data("poolrad.dat", "poolrad", 1, 0, errMsg)) {
//		Common::String msg(errMsg);
//		error("%s", msg.c_str());
//	}
	Common::File items;
	if (!items.open("ITEMS")) {
		warning("Cannot open ITEMS file");
		return false;
	}
	Common::SeekableReadStream &in = items;
	Engine::gItemProps.load(in);
//	Engine::gItemProps.debugStorage(); //TODO: Remove this line later
	items.close();

	if (!_strings.load("global_strings.yml")) {
		error("Failed to open global_strings.yml");
	}

	return true;
}

bool PoolradEngine::loadGameAssets() {
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
	getDaxManager().loadFile(Common::Path("pic1.dax"));
	getDaxManager().loadFile(Common::Path("pic2.dax"));
	getDaxManager().loadFile(Common::Path("pic3.dax"));
	getDaxManager().loadFile(Common::Path("pic4.dax"));
	getDaxManager().loadFile(Common::Path("pic5.dax"));
	getDaxManager().loadFile(Common::Path("pic6.dax"));
	getDaxManager().loadFile(Common::Path("pic7.dax"));
	getDaxManager().loadFile(Common::Path("pic8.dax"));
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

	// Load ECL script DAX files
	getDaxManager().loadFile(Common::Path("ecl1.dax"));
	getDaxManager().loadFile(Common::Path("ecl2.dax"));
	getDaxManager().loadFile(Common::Path("ecl3.dax"));
	getDaxManager().loadFile(Common::Path("ecl4.dax"));
	getDaxManager().loadFile(Common::Path("ecl5.dax"));
	getDaxManager().loadFile(Common::Path("ecl6.dax"));
	getDaxManager().loadFile(Common::Path("ecl7.dax"));
	getDaxManager().loadFile(Common::Path("ecl8.dax"));

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

	return true;
}

bool PoolradEngine::initializeRuntimeSystems() {
	// Wire subsystem pointers on base Engine
	EffectHandler *eh = new EffectHandler();
	eh->setupHandlers();
	_effectHandler = eh;

	_spellCasting = new Spells::SpellCastingService();

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
	// 4. Runtime exchange bridge (generic contract + Poolrad mapping).
	_runtimeExchange.reset(new PoolradRuntimeExchange(this));

	// Phase 2: write GAME_Init defaults so VM memory is valid for
	// save/load even from the main menu (matches original behavior).
	initGameDefaults();

	return true;
}

void PoolradEngine::onGameStateEnter(GameState prev, GameState next) {
	Views::View *view = nullptr;
	debug(2, "PoolradEngine::onGameStateEnter prev=%d next=%d", (int)prev,
		(int)next);

	if (isMapRuntimeState(next) && (!isMapRuntimeState(prev) || prev != next)) {
		// Camp keeps the map context: entering or leaving it only redraws
		// the map view (EclVm.EnterCamp), it never re-runs ONINIT.
		const bool campFromMap = (next == GS_CAMPING && isMapRuntimeState(prev));
		const bool campToMap = (prev == GS_CAMPING
			&& (next == GS_DUNGEON_MAP || next == GS_WILDERNESS_MAP));
		if (!campFromMap && !campToMap) {
			_mapRuntimeNeedsInit = true;
			debug(2, "PoolradEngine::onGameStateEnter map runtime init requested for state=%d",
				(int)next);
		}
	}

	switch (next) {
	case GS_START_MENU:
		debug(2, "PoolradEngine::onGameStateEnter -> replaceView(Title, true)");
		replaceView("Title", true);
		view = dynamic_cast<Views::View *>(findView("Title"));
		if (view) {
			view->onEnter(next);
		}
		break;
	case GS_SHOP:
		debug(2, "PoolradEngine::onGameStateEnter -> replaceView(InGame) [SHOP]");
		// InGameView kModeShop: drawMainScreenWindows(true) + NPC portrait at (3,3).
		replaceView("InGame");
		view = dynamic_cast<Views::View *>(findView("InGame"));
		if (view)
			view->onEnter(next);
		break;
	case GS_CAMPING:
		debug(2, "PoolradEngine::onGameStateEnter -> replaceView(InGame) [CAMPING]");
		// Host-layer resource preparation for camp mode: preload campfire
		// picture so the viewport draw path can consume PictureDisplayCache.
		getPictureDisplayCache().load(0xFF, kCampfirePicId);
		// InGameView kModeCamping: drawMainScreenWindows(true) + camp state area.
		replaceView("InGame");
		view = dynamic_cast<Views::View *>(findView("InGame"));
		if (view)
			view->onEnter(next);
		break;
	case GS_DUNGEON_MAP:
		debug(2, "PoolradEngine::onGameStateEnter -> replaceView(InGame) [DUNGEON]");
		// InGameView kModeDungeon: drawMainScreenWindows(true) + 3D view + party panel.
		replaceView("InGame");
		view = dynamic_cast<Views::View *>(findView("InGame"));
		if (view)
			view->onEnter(next);
		break;
	case GS_WILDERNESS_MAP:
		debug(2, "PoolradEngine::onGameStateEnter -> replaceView(InGame) [WILDERNESS]");
		// InGameView kModeWilderness: drawMainScreenWindows(false) + area-map block.
		replaceView("InGame");
		view = dynamic_cast<Views::View *>(findView("InGame"));
		if (view)
			view->onEnter(next);
		break;
	case GS_AFTER_COMBAT:
		debug(2, "PoolradEngine::onGameStateEnter -> replaceView(InGame) [AFTER_COMBAT]");
		// InGameView kModeAfterCombat: drawMainScreenWindows(true) + loot panel.
		replaceView("InGame");
		view = dynamic_cast<Views::View *>(findView("InGame"));
		if (view)
			view->onEnter(next);
		break;
	case GS_COMBAT:
		debug(2, "PoolradEngine::onGameStateEnter -> replaceView(InGame) [COMBAT]");
		// InGameView kModeCombat: layout to be defined.
		replaceView("InGame");
		view = dynamic_cast<Views::View *>(findView("InGame"));
		if (view)
			view->onEnter(next);
		break;
	case GS_END_GAME:
		debug(2, "PoolradEngine::onGameStateEnter -> GameOver [END_GAME]");
		getGameText().clearMessageArea();
		replaceView(findView("GameOver"), true);
		break;
	default:
		debug(2, "PoolradEngine::onGameStateEnter no explicit view mapping for state=%d",
			(int)next);
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

bool PoolradEngine::isSaveBoundaryStable() const {
	// Feed the live engine state into the pure, headless-testable predicate
	// (GB13-033 24.2, Requirement 30.3). hasPendingTransaction is reserved for
	// 24.3's transaction tracking; there is no half-applied-transaction signal
	// to query yet, so it is left false (conservative: the other gates already
	// exclude mid-operation states).
	NativeSave::SaveBoundaryState st;
	st.gameState = getGameState();
	st.vmSuspended = _eclFlags.suspended;
	st.hasPendingHostRequest =
		(_eclHost && _eclHost->hasPendingAsync());
	st.hasPendingTransaction = false;
	return NativeSave::isSaveBoundaryStable(st);
}

Common::Error PoolradEngine::syncGameState(Common::Serializer &s) {
	// GB13-033: SAVE drives the native versioned schema here. LOAD is handled
	// transactionally in loadGameStream() over the raw stream (24.3), so this
	// seam is save-only in practice; NativeSave::sync() guards against a stray
	// Serializer-only load.
	return NativeSave::sync(s, *this);
}

Common::Error PoolradEngine::loadGameStream(Common::SeekableReadStream *stream) {
	// GB13-033 24.3: transactional native load. Parse into a temporary,
	// validate against this engine, then swap in. A malformed/truncated/
	// wrong-profile/missing-resource load returns the first error with the
	// previous game intact.
	if (!stream) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: no stream to load from");
	}
	return NativeSave::loadNative(*stream, *this);
}

bool PoolradEngine::canSaveGameStateCurrently(Common::U32String *msg) {
	// GB13-033 24.2 (Requirement 30.3): AND the engine-family sentinel with the
	// live save-boundary predicate. While kNativeSaveImplemented is false this
	// is always false (native save is not advertised); once 24.3 flips it on,
	// saving is still DISABLED in combat/rest/modal/suspended contexts.
	if (!nativeSaveAllowed(isSaveBoundaryStable())) {
		if (msg) {
			if (!kNativeSaveImplemented)
				*msg = _("Saving is not yet supported in this engine.");
			else
				*msg = _("Saving is not available right now.");
		}
		return false;
	}
	return true;
}

Data::DaxBlockGeo *PoolradEngine::getGeoBlockById(uint8 mapId) {
	return dynamic_cast<Data::DaxBlockGeo *>(
		getDaxGeo().getBlockById(mapId));
}

Data::DaxBlockGeo *PoolradEngine::getActiveGeoBlock() {
	// The renderer needs the GEO block that was actually loaded into
	// RuntimeGeoBlock (kVmFieldGeoBlockId), NOT the ECL script ID
	// (byteMapId / kVmFieldSavedEclId). These can differ when a map
	// uses a different ECL script than its GEO block number.
	const RuntimeGeoBlock &rtGeo = getRuntimeGeo();
	if (!rtGeo.isLoaded())
		return nullptr;
	return getGeoBlockById(rtGeo.blockId());
}

bool PoolradEngine::captureRuntimeMapSnapshot(
		::Goldbox::RuntimeMapSnapshot &snapshot) const {
	const RuntimeExchange *exchange = getRuntimeExchange();
	if (!exchange)
		return false;

	return exchange->captureMapSnapshot(snapshot) && snapshot.valid;
}

bool PoolradEngine::getActiveMapPosition(uint16 &x, uint16 &y,
		uint8 &dir) const {
	::Goldbox::RuntimeMapSnapshot snapshot;
	if (!captureRuntimeMapSnapshot(snapshot))
		return false;

	x = snapshot.dungeonPos.x;
	y = snapshot.dungeonPos.y;
	dir = snapshot.dungeonDir;
	return true;
}

uint8 PoolradEngine::getGameSpeed() const {
	const ECL::AddressSpace *mem = getEclMemory();
	if (!mem)
		return 1;

	const ECL::EclLayoutAccess layout = _eclConfig.getLayoutAccess();
	const uint8 speed = mem->read8(layout.vmField(kVmFieldGameSpeed).vmAddr);
	return (speed == 0) ? 1 : speed;
}

void PoolradEngine::setLegacyMenuStatus(uint8 status) {
	_legacySharedState.byteMenuStatus = status;

	if (!_eclVm)
		return;

	ECL::EclLayoutAccess layout = _eclConfig.getLayoutAccess();
	ECL::AddressSpace &mem = _eclVm->getMemory();
	const uint16 menuStatusAddr = layout.runtimeField(ECL::kEclRuntimeMenuStatus);
	if (ECL::EclRuntimeLayout::isValidVmAddr(menuStatusAddr))
		mem.write8(menuStatusAddr, status);
}

bool PoolradEngine::saveGameSlotX86(char slotLetter,
		Common::String &errorMessage) {
	errorMessage.clear();

	const char slot = toUpperAscii(slotLetter);
	if (slot < 'A' || slot > 'J') {
		errorMessage = "Invalid save slot";
		return false;
	}

	ECL::AddressSpace *mem = getEclMemory();
	if (!mem) {
		errorMessage = "ECL memory is not ready";
		return false;
	}

	Common::Path savePath = resolveSavePath();

	Common::FSNode saveNode(savePath);
	if (!saveNode.isDirectory()) {
		if (!saveNode.createDirectory()) {
			errorMessage = "Failed to create save directory";
			return false;
		}
	}

	const Common::Path gameSavePath =
		savePath / Common::String::format("SAVGAM%c.DAT", slot);

	Common::DumpFile out;
	if (!out.open(gameSavePath)) {
		errorMessage = Common::String::format("Failed to create %s",
			gameSavePath.toString().c_str());
		return false;
	}

	auto writeVmBlockWord = [&](uint16 firstVmAddr, uint16 wordCount) {
		for (uint16 i = 0; i < wordCount; ++i) {
			const uint16 addr = static_cast<uint16>(firstVmAddr + i);
			out.writeByte(mem->read8(addr)); // lo byte (the value)
			out.writeByte(0);                // hi byte (always 0 for byte-sized fields)
		}
	};

	auto writeVmBlockByte = [&](uint16 startAddr, uint32 size) {
		for (uint32 i = 0; i < size; ++i)
			out.writeByte(mem->read8(static_cast<uint16>(startAddr + i)));
	};

	// BYTE_FAVAIL / leading legacy byte:
	// - x86 loads it back into BYTE_FAVAIL from DAT bank state.
	// - m68k/Amiga code appears to don't save it at all,
	//   when creating a legacy save file.
	// ScummVM does not use it for runtime behavior; we keep the byte for
	// binary compatibility with the original save format.
	out.writeByte(3);

	writeVmBlockWord(0x4900, 0x0400); // VMBANK0_WORLD_STATE (0x0400 words = 0x0800 bytes)
	writeVmBlockWord(0x6B00, 0x0400); // VMBANK1_PARTY_STATE (0x0400 words = 0x0800 bytes)
	writeVmBlockWord(0x9700, 0x0200); // VMBANK2_COMBAT_STATE (0x0200 words = 0x0400 bytes)
	writeVmBlockByte(0x9900, 0x1E00); // VMBANK3_ECL_SCRIPT  (byte-addressed)

	::Goldbox::RuntimeMapSnapshot snapshot;
	if (!captureRuntimeMapSnapshot(snapshot)) {
		errorMessage = "Failed to capture runtime snapshot";
		out.close();
		return false;
	}

	const uint16 posX = snapshot.dungeonPos.x;
	const uint16 posY = snapshot.dungeonPos.y;
	const uint8 posDir = static_cast<uint8>(snapshot.dungeonDir & 0x03);
	const uint8 vmMapType = snapshot.mapType;

	uint8 characterCount = 0;
	byte characterTable[0x148];
	memset(characterTable, 0, sizeof(characterTable));
	Common::Array<Common::String> usedBases;

	for (auto it = _party.begin(); it != _party.end(); ++it) {
		if (characterCount >= 8)
			break;

		Data::PoolradCharacter *pc =
			dynamic_cast<Data::PoolradCharacter *>(*it);
		if (!pc)
			continue;

		++characterCount;
		const Common::String base = Data::makeLegacyCharacterBaseName(pc,
			slot, characterCount, usedBases);
		usedBases.push_back(base);

		const uint32 tableOffset = (characterCount - 1) * 0x29;
		Goldbox::Data::PascalStringBuffer<0x28>::writeToBuffer(
			&characterTable[tableOffset], base, 0x29);

		const Common::Path savPath = savePath / (base + ".SAV");
		Common::DumpFile charOut;
		if (!charOut.open(savPath)) {
			errorMessage = Common::String::format("Failed to save %s",
				savPath.toString().c_str());
			out.close();
			return false;
		}
		pc->save(charOut);
		charOut.close();

		const Common::Path itmPath = savePath / (base + ".ITM");
		const Common::Path spcPath = savePath / (base + ".SPC");

		if (!pc->inventory.items().empty()) {
			if (!pc->inventory.save(itmPath.toString())) {
				errorMessage = Common::String::format("Failed to save %s",
					itmPath.toString().c_str());
				out.close();
				return false;
			}
		} else {
			(void)::remove(itmPath.toString().c_str());
		}

		if (!pc->effects.effects().empty()) {
			if (!pc->effects.save(spcPath.toString())) {
				errorMessage = Common::String::format("Failed to save %s",
					spcPath.toString().c_str());
				out.close();
				return false;
			}
		} else {
			(void)::remove(spcPath.toString().c_str());
		}
	}

	// Legacy save tail layout (0x150 bytes):
	// +0x00 u8 dungeonX, +0x01 u8 dungeonY, +0x02 u8 dir (8-way),
	// +0x03 u8 posNibble (walldef in facing dir), +0x04 u8 posEventId,
	// +0x05 u8 vmMapType, +0x06 u8 gameState, +0x07 u8 charCount,
	// +0x08..+0x14f char table.
	byte tail[0x150];
	memset(tail, 0, sizeof(tail));
	tail[0x00] = static_cast<byte>(posX & 0xFF);
	tail[0x01] = static_cast<byte>(posY & 0xFF);
	tail[0x02] = static_cast<byte>(posDir * 2);  // 4-way to 8-way
	tail[0x03] = snapshot.wallNibble;   // position nibble (walldef type)
	tail[0x04] = snapshot.eventId;      // position event ID
	tail[0x05] = vmMapType;
	tail[0x06] = static_cast<byte>(getGameState());
	tail[0x07] = characterCount;
	memcpy(&tail[0x08], characterTable, sizeof(characterTable));
	out.write(tail, sizeof(tail));
	out.flush();
	out.close();

	return true;
}

bool PoolradEngine::loadGameSlotX86(char slotLetter,
		Common::String &errorMessage) {
	errorMessage.clear();
	debug(2, "PoolradEngine::loadGameSlotX86 requested slot=%c", slotLetter);

	const char slot = toUpperAscii(slotLetter);
	if (slot < 'A' || slot > 'J') {
		errorMessage = "Invalid save slot";
		return false;
	}

	ECL::AddressSpace *mem = getEclMemory();
	if (!mem) {
		errorMessage = "ECL memory is not ready";
		return false;
	}

	Common::Path savePath = resolveSavePath();

	const Common::Path gameSavePath =
		savePath / Common::String::format("SAVGAM%c.DAT", slot);
	debug(2, "PoolradEngine::loadGameSlotX86 path=%s",
		gameSavePath.toString().c_str());

	Common::FSNode saveNode(gameSavePath);
	Common::SeekableReadStream *in = saveNode.createReadStream();
	if (!in) {
		errorMessage = Common::String::format("Failed to open %s",
			gameSavePath.toString().c_str());
		debug(2, "PoolradEngine::loadGameSlotX86 open failed: %s",
			errorMessage.c_str());
		return false;
	}

	// Skip leading BYTE_FAVAIL / legacy placeholder byte.
	in->readByte();

	// Load the 4 VM banks directly into ECL flat memory.
	// VMBANK0-2 are word-addressed in the original: each logical VM address
	// occupies 2 bytes (LE word) in the save file. We unpack them so that
	// vm.read8(addr) returns the low byte of the stored word, matching
	// original VM_ReadVar / VM_ReadMemory behavior.
	auto readVmBlockWord = [&](uint16 firstVmAddr, uint16 wordCount) {
		for (uint16 i = 0; i < wordCount; ++i) {
			const uint8 lo = in->readByte();
			in->readByte(); // hi byte (unused in flat byte-addressed memory)
			const uint16 addr = static_cast<uint16>(firstVmAddr + i);
			mem->write8(addr, lo);
		}
	};

	// VMBANK3 (ECL script) is byte-addressed: 1 byte per address.
	auto readVmBlockByte = [&](uint16 startAddr, uint32 size) {
		byte *buf = new byte[size];
		in->read(buf, size);
		mem->loadBytes(startAddr, Common::Span<const uint8>(buf, size));
		delete[] buf;
	};

	readVmBlockWord(0x4900, 0x0400); // VMBANK0_WORLD_STATE (0x0400 words = 0x0800 bytes)
	readVmBlockWord(0x6B00, 0x0400); // VMBANK1_PARTY_STATE (0x0400 words = 0x0800 bytes)
	readVmBlockWord(0x9700, 0x0200); // VMBANK2_COMBAT_STATE (0x0200 words = 0x0400 bytes)
	readVmBlockByte(0x9900, 0x1E00); // VMBANK3_ECL_SCRIPT  (byte-addressed)

	// Legacy save tail struct (8 bytes):
	// +0x00 u8 dungeonX, +0x01 u8 dungeonY, +0x02 u8 dir (8-way),
	// +0x03 u8 posNibble (walldef in facing dir), +0x04 u8 posEventId,
	// +0x05 u8 vmMapType, +0x06 u8 gameState, +0x07 u8 charCount.
	const uint8 dungeonX       = in->readByte();
	const uint8 dungeonY       = in->readByte();
	const uint8 dir8way        = in->readByte();
	const uint8 wallNibble     = in->readByte();
	const uint8 eventId        = in->readByte();
	const uint8 vmMapType      = in->readByte();
	const uint8 byteGameState  = in->readByte();

	// Character table: 8 x 0x29-byte Pascal-style base filenames.
	const uint8 characterCount = in->readByte();
	byte characterTable[0x148];
	in->read(characterTable, sizeof(characterTable));

	// Convert 8-way direction to 4-way for VM memory storage.
	const uint8 posDir = static_cast<uint8>((dir8way / 2) & 0x03);

	debug(2, "PoolradEngine::loadGameSlotX86 tail x=%u y=%u dir8=%u dir4=%u nibble=%u event=%u mapType=%u gameState=%u charCount=%u",
		(unsigned)dungeonX, (unsigned)dungeonY, (unsigned)dir8way,
		(unsigned)posDir, (unsigned)wallNibble, (unsigned)eventId,
		(unsigned)vmMapType, (unsigned)byteGameState,
		(unsigned)characterCount);

	delete in;
	in = nullptr;

	// -------------------------------------------------------------------------
	// Post-load: restore system bank position (not included in VM bank dumps).
	// -------------------------------------------------------------------------
	ECL::EclLayoutAccess layout = _eclConfig.getLayoutAccess();

	mem->write8(layout.vmGlobalField(kVmGlobalFieldDungeonX).vmAddr, dungeonX);
	mem->write8(layout.vmGlobalField(kVmGlobalFieldDungeonY).vmAddr, dungeonY);
	mem->write8(layout.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr, posDir);
	mem->write8(layout.vmGlobalField(kVmGlobalFieldMapWallType).vmAddr, wallNibble);
	mem->write8(layout.vmGlobalField(kVmGlobalFieldMapSquareInfo).vmAddr, eventId);

	// Reset party count in VM memory before rebuilding the party list.
	mem->write8(layout.vmGlobalField(kVmGlobalFieldPartyCount).vmAddr, 0);

	// -------------------------------------------------------------------------
	// Clear existing party and reload characters from individual save files.
	// -------------------------------------------------------------------------
	for (auto it = _party.begin(); it != _party.end(); ++it)
		delete *it;
	_party.clear();

	const uint8 count = MIN<uint8>(characterCount, 8);
	for (uint8 i = 0; i < count; ++i) {
		const byte *entry = &characterTable[i * 0x29];
		const uint8 entryLen = entry[0];
		Common::String base;
		if (entryLen <= 0x28) {
			base = Goldbox::Data::PascalStringBuffer<0x28>::readFromBuffer(
				entry, 0x29);
		} else {
			// Compatibility fallback: older ScummVM builds wrote
			// null-terminated entries without a Pascal length prefix.
			for (uint j = 0; j < 0x29 && entry[j] != '\0'; ++j)
				base += static_cast<char>(entry[j]);
		}
		if (base.empty())
			continue;

		debug(2, "PoolradEngine::loadGameSlotX86 char[%u] base=%s",
			(unsigned)(i + 1), base.c_str());

		const Common::Path charSavPath = savePath / (base + ".SAV");
		Common::FSNode charNode(charSavPath);
		Common::SeekableReadStream *charStream = charNode.createReadStream();
		if (!charStream) {
			debug(2, "PoolradEngine::loadGameSlotX86 missing .SAV for base=%s",
				base.c_str());
			continue;
		}

		Data::PoolradCharacter *pc = new Data::PoolradCharacter();
		pc->load(*charStream);
		delete charStream;

		Common::Path itmResolvedPath;
		Common::SeekableReadStream *itmStream = nullptr;
		const bool itmLoaded = Data::openLegacyCompanionStream(savePath, base, slot,
			".ITM", ".itm", itmResolvedPath, itmStream);
		if (itmLoaded && itmStream) {
			pc->inventory.loadFromStream(*itmStream);
			delete itmStream;
			itmStream = nullptr;
			pc->resolveEquippedItems();
		}

		Common::Path spcResolvedPath;
		Common::SeekableReadStream *spcStream = nullptr;
		const bool spcLoaded = Data::openLegacyCompanionStream(savePath, base, slot,
			".SPC", ".spc", spcResolvedPath, spcStream);
		if (spcLoaded && spcStream) {
			pc->effects.loadFromStream(*spcStream);
			delete spcStream;
			spcStream = nullptr;
		}

		debug(2, "PoolradEngine::loadGameSlotX86 companion files for %s: ITM=%s (%u items) SPC=%s (%u effects)",
			base.c_str(),
			itmLoaded ? "loaded" : "missing/failed",
			(unsigned)pc->inventory.items().size(),
			spcLoaded ? "loaded" : "missing/failed",
			(unsigned)pc->effects.effects().size());
		if (itmLoaded)
			debug(2, "PoolradEngine::loadGameSlotX86 ITM path=%s",
				itmResolvedPath.toString().c_str());
		if (spcLoaded)
			debug(2, "PoolradEngine::loadGameSlotX86 SPC path=%s",
				spcResolvedPath.toString().c_str());
		debug(2, "PoolradEngine::loadGameSlotX86 loaded base=%s (.SAV required, .ITM/.SPC optional)",
			base.c_str());

		_party.push_back(pc);
	}

	// Update VM party count to match how many characters were successfully loaded.
	mem->write8(layout.vmGlobalField(kVmGlobalFieldPartyCount).vmAddr,
		static_cast<uint8>(_party.size()));
	debug(2, "PoolradEngine::loadGameSlotX86 rebuilt party size=%u",
		(unsigned)_party.size());

	// -------------------------------------------------------------------------
	// Reload world graphics / ECL script based on saved game state.
	// -------------------------------------------------------------------------
	const uint8 savedMapId =
		mem->read8(layout.vmField(kVmFieldSavedEclId).vmAddr);
	const uint8 geoBlockId =
		mem->read8(layout.vmField(kVmFieldGeoBlockId).vmAddr);
	const uint8 wallPrimary =
		mem->read8(layout.vmField(kVmFieldWallSetPrimary).vmAddr);
	const uint8 wallSecondary =
		mem->read8(layout.vmField(kVmFieldWallSetSecondary).vmAddr);
	debug(2, "PoolradEngine::loadGameSlotX86 loaded state: savedMapId=%u geoBlockId=%u wallPrimary=%u wallSecondary=%u vmMapType=%u gameState=%u",
		(unsigned)savedMapId, (unsigned)geoBlockId,
		(unsigned)wallPrimary, (unsigned)wallSecondary,
		(unsigned)vmMapType, (unsigned)byteGameState);

	const bool loadIntoRuntime =
		(static_cast<GameState>(byteGameState) != GS_START_MENU);

	// Debug: dump key addresses after VM bank restore to verify save data integrity.
	{
		const uint8 val4AC5 = mem->read8(0x4AC5);
		const uint16 val4AC5_w = mem->read16LE(0x4AC5);
		const uint8 val4A17 = mem->read8(0x4A17);
		const uint16 val4A17_w = mem->read16LE(0x4A17);
		debug(1, "PoolradEngine::loadGameSlotX86 POST-RESTORE: "
			"mem[0x4AC5]=0x%02X (word=0x%04X) mem[0x4A17]=0x%02X (word=0x%04X)",
			(unsigned)val4AC5, (unsigned)val4AC5_w,
			(unsigned)val4A17, (unsigned)val4A17_w);
		// Dump a few bytes around 0x4AC5 for context.
		debug(1, "PoolradEngine::loadGameSlotX86 DUMP 0x4AC0-0x4ACF: "
			"%02X %02X %02X %02X %02X %02X %02X %02X "
			"%02X %02X %02X %02X %02X %02X %02X %02X",
			(unsigned)mem->read8(0x4AC0), (unsigned)mem->read8(0x4AC1),
			(unsigned)mem->read8(0x4AC2), (unsigned)mem->read8(0x4AC3),
			(unsigned)mem->read8(0x4AC4), (unsigned)mem->read8(0x4AC5),
			(unsigned)mem->read8(0x4AC6), (unsigned)mem->read8(0x4AC7),
			(unsigned)mem->read8(0x4AC8), (unsigned)mem->read8(0x4AC9),
			(unsigned)mem->read8(0x4ACA), (unsigned)mem->read8(0x4ACB),
			(unsigned)mem->read8(0x4ACC), (unsigned)mem->read8(0x4ACD),
			(unsigned)mem->read8(0x4ACE), (unsigned)mem->read8(0x4ACF));
	}

	if (loadIntoRuntime) {
		// Non-GS_START_MENU: BOOL_GAME_LOADED=true path.
		// Original GB_EngineMain skips ECL_LoadBlock (bytes already in VMBANK3
		// from save) but still calls ECL_LoadHeader to parse entry points.
		if (_eclVm) {
			_eclVm->loadProgramFromMemory(savedMapId);
		}

		// Preload geo/walldefs from saved state.
		if (vmMapType < 2 && _eclHost) {
			_eclHost->loadGeoBlock(geoBlockId);

			static const uint16 kGeoSavedWallBlockBase = 0x4AF9;
			static const uint16 kGeoSavedWallSlotBase  = 0x4AFC;
			for (uint8 wallSlot = 1; wallSlot <= 3; ++wallSlot) {
				const int16 blockId = static_cast<int16>(static_cast<int8>(mem->read8(
					static_cast<uint16>(kGeoSavedWallBlockBase + wallSlot))));
				const uint8 setSlot = mem->read8(
					static_cast<uint16>(kGeoSavedWallSlotBase + wallSlot));
				if (blockId >= 0)
					_eclHost->loadWallSet(static_cast<uint8>(blockId & 0xFF), setSlot);
			}
		} else if (_eclHost) {
			_eclHost->loadIconBlock();
		}
	} else {
		// GS_START_MENU: BOOL_GAME_LOADED=false path.
		// Original stays at title screen; player presses 'B' (Begin) which
		// triggers GB_EngineMain -> ECL_LoadBlock(BYTE_MAP_ID) -> ON_INIT.
		// Pre-load the ECL block from DAX now so it's ready for the 'B'
		// transition. The actual state change happens in the title view.
		const uint8 mapId = savedMapId;
		debug(2, "PoolradEngine::loadGameSlotX86 GS_START_MENU: preloading ECL block %u from DAX",
			(unsigned)mapId);

		Goldbox::Data::DaxBlock *eclBlock =
			getDaxManager().getEcl().getBlockById(mapId);
		if (eclBlock && eclBlock->_data.size() > 2 && _eclVm) {
			// Original zeroes VMBANK3 (0x1E00 bytes) before loading.
			for (uint16 i = 0; i < 0x1E00; ++i)
				mem->write8(static_cast<uint16>(0x9900 + i), 0);
			// ECL_LoadBlock skips first 2 bytes (size header) then copies
			// into VMBANK3. loadProgram handles the full sequence.
			Common::Span<const uint8> eclData(
				eclBlock->_data.data() + 2,
				eclBlock->_data.size() - 2);
			_eclVm->loadProgram(eclData, mapId);
			debug(2, "PoolradEngine::loadGameSlotX86 ECL block %u loaded (%u bytes)",
				(unsigned)mapId,
				(unsigned)(eclBlock->_data.size() - 2));
		} else {
			warning("PoolradEngine::loadGameSlotX86 ECL block %u not found or too small",
				(unsigned)mapId);
		}

		_legacySharedState.boolStateLoaded = false;
	}

	// -------------------------------------------------------------------------
	// If 3D terrain mode was active, signal that 3D rendering needs to restart.
	// G_TerrainFlags (field387_0x344 = kVmFieldScriptFlagAA2): non-zero â†’ 3D dungeon.
	// The _mapRuntimeNeedsInit flag (set by setGameState below) already covers
	// this via initializeMapRuntimeForState; note it here for Amiga diff tracing.
	// m68k-only: would call GFX_3DRender(false) explicitly here if != 0.
	// -------------------------------------------------------------------------

	// -------------------------------------------------------------------------
	// Update legacy shared state and transition to the saved game state.
	// -------------------------------------------------------------------------
	_legacySharedState.byteMapId =
		mem->read8(layout.vmField(kVmFieldGeoBlockId).vmAddr);
	const bool loadedIntoRuntime = loadIntoRuntime;
	_legacySharedState.boolStateLoaded = loadedIntoRuntime;
	if (_eclVm)
		_eclVm->stateLoaded = loadedIntoRuntime;

	// Mirror original: GFX_SetDefaultColors(6,7,0,0xb) before GB_EngineMain.
	// Initialize full default skybox, then signal redraw so the renderer
	// picks up sky/ceiling colors from the restored World bank state.
	if (loadIntoRuntime) {
		if (_eclHost)
			_eclHost->setDefaultSkyboxColors();

		const uint16 skyboxFlag = layout.runtimeField(
			ECL::kEclRuntimeSkyboxRedrawFlag);
		if (ECL::EclRuntimeLayout::isValidVmAddr(skyboxFlag))
			mem->write8(skyboxFlag, 1);
		if (g_events) {
			g_events->postEclStateMessage(EclVmMessage::ST_SKYBOX_DIRTY, 1,
				EclVmMessage::VT_UINT8);
		}
	}

	debug(2, "PoolradEngine::loadGameSlotX86 applying setGameState(%u)",
		(unsigned)byteGameState);
	// Original GB_EngineMain only enters with GS_DUNGEON_MAP or GS_WILDERNESS_MAP.
	// Other states (CAMPING, SHOP, COMBAT) are transient sub-loops that always
	// resume into a map state. Resolve to the correct map state on load.
	// GB13-021: reconstruct the authoritative location TYPE from the persisted
	// active script id + indoor/3D-view flag (the location type and coords,
	// not re-derived from the indoor flag alone). A transient sub-loop state
	// (CAMPING/SHOP/COMBAT) always resumes into the map state its location
	// implies.
	const uint8 loadedMapId = mem->read8(layout.vmField(kVmFieldSavedEclId).vmAddr);
	const uint8 loadedIndoorFlag =
		mem->read8(layout.vmField(kVmFieldIndoorModeFlag).vmAddr);
	const uint8 loadedView3dFlag = (loadedIndoorFlag != 0) ? 1 : 0;
	_location.kind = LocationContext::classify(loadedMapId, loadedView3dFlag);
	_location.syncRegion();
	_location.activeEclId = loadedMapId;
	_location.activeGeoId = loadedMapId;
	_location.dungeonPos = MapPos(
		static_cast<int8>(mem->read8(
			layout.vmGlobalField(kVmGlobalFieldDungeonX).vmAddr)),
		static_cast<int8>(mem->read8(
			layout.vmGlobalField(kVmGlobalFieldDungeonY).vmAddr)));
	_location.dungeonDir =
		mem->read8(layout.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr) & 0x03;
	_location.wildernessPos = TilePos(
		mem->read8(layout.vmField(kVmFieldWildernessX).vmAddr),
		mem->read8(layout.vmField(kVmFieldWildernessY).vmAddr));
	_location.indoorRender = (loadedIndoorFlag != 0);
	_hasSavedCallerLocation = false;

	GameState resolvedState = static_cast<GameState>(byteGameState);
	if (resolvedState != GS_DUNGEON_MAP && resolvedState != GS_WILDERNESS_MAP
			&& resolvedState != GS_START_MENU) {
		resolvedState = mapStateForLocation(_location);
		debug(2, "PoolradEngine::loadGameSlotX86 resolved transient state %u -> %u",
			(unsigned)byteGameState, (unsigned)resolvedState);
	}
	_legacySharedState.byteGameState = resolvedState;

	// GB13-009 (8.2): a new-game load replaces the whole VM/party context.
	// Bump the pending-operation request generation on the LOAD branch (not in
	// Engine::syncGame, which also runs mid-session on save) so any completion
	// minted before the load is discarded as out-of-generation instead of
	// mutating the freshly loaded state. _pendingOps is host-private, so we go
	// through the engine-reachable host hook.
	if (_eclHost)
		_eclHost->bumpPendingGeneration();

	setGameState(resolvedState);
	debug(2, "PoolradEngine::loadGameSlotX86 complete: engine gameState now=%d",
		(int)getGameState());

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

bool PoolradEngine::queueInGameCommand(Views::InGameView::InGameCommand cmd) {
	Views::InGameView *inGameView = getInGameView();
	if (!inGameView)
		return false;

	inGameView->queueCommand(cmd);
	return true;
}

bool PoolradEngine::tick() {
	bool handled = Goldbox::Events::tick();

	if (_mapRuntimeNeedsInit && !_eclFlags.suspended &&
			isMapRuntimeState(getGameState())) {
		initializeMapRuntimeForState(getGameState());
	}

	if (_eclFlags.suspended && _eclHost && _eclVm) {
		_eclHost->tickAsyncMenuPicture();
		bool readyToResume = false;
		const bool hasAsync = _eclHost->hasPendingAsync();
		const bool asyncReady = hasAsync ? _eclHost->isPendingAsyncReady() : false;
		if (!hasAsync) {
			readyToResume = true;
		} else if (asyncReady) {
			const VmResult finalizeResult = _eclHost->finalizePendingAsync();
			if (finalizeResult == VM_OK) {
				readyToResume = true;
			} else if (finalizeResult == VM_ERROR || finalizeResult == VM_HALTED) {
				_eclFlags.suspended = false;
				if (_campEntryPending) {
					_campEntryPending = false;
					if (finalizeResult == VM_HALTED)
						enterCampAfterOnRest();
				}
			}
		}
		if (readyToResume) {
			_eclFlags.suspended = false;
			const VmResult resume =
				executeEclAtScriptAddress(_eclVm->getPC());
			if (resume == VM_YIELD) {
				_eclFlags.suspended = true;
			} else if (_campEntryPending) {
				_campEntryPending = false;
				enterCampAfterOnRest();
			} else if (_mapRuntimeReady
					&& (_eclVm->eclReady || _newEclPhase != kNewEclIdle))
				runNewEclDone();
			else if (!_mapRuntimeReady && isMapRuntimeState(getGameState())) {
				// ONINIT yielded and has now completed. Do post-init refresh.
				if (getGameState() != GS_WILDERNESS_MAP) {
					if (_eclVm->screenRefresh) {
						Views::InGameView *igv = getInGameView();
						if (igv)
							igv->applyScreenByState(getGameState());
						_eclVm->screenRefresh = false;
						_eclFlags.screenRefresh = false;
					}
					if (_eclHost)
						_eclHost->refreshViewport();
				}
				// Sync VM position → InGameView.
				Views::InGameView *syncIgv = getInGameView();
				if (syncIgv && _eclVm) {
					ECL::AddressSpace &m = _eclVm->getMemory();
					ECL::EclLayoutAccess l = _eclConfig.getLayoutAccess();
					const uint16 x = m.read8(l.vmGlobalField(kVmGlobalFieldDungeonX).vmAddr);
					const uint16 y = m.read8(l.vmGlobalField(kVmGlobalFieldDungeonY).vmAddr);
					const uint8 d = static_cast<uint8>(m.read8(l.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr) & 0x03);
					syncIgv->setMapPosition(x, y, static_cast<uint8>(d * 2));
				}
				_mapRuntimeReady = true;
				if (g_events) {
					g_events->postEclStateMessage(
						EclVmMessage::ST_INGAME_MENU_VISIBLE,
						1, EclVmMessage::VT_UINT8);
				}
			}
		}
	}

	if (isMapRuntimeState(getGameState())) {
		dispatchPlayerCommand();
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

GameState PoolradEngine::mapStateForLocation(const LocationContext &ctx) {
	// GB13-021: the map state a location implies, used for service return
	// instead of unconditionally choosing GS_DUNGEON_MAP. Wilderness kinds
	// (west/central/east) resume GS_WILDERNESS_MAP; dungeon resumes
	// GS_DUNGEON_MAP.
	return ctx.isWilderness() ? GS_WILDERNESS_MAP : GS_DUNGEON_MAP;
}

void PoolradEngine::saveCallerLocation() {
	// Capture the caller location before a camp/shop/temple/battle sub-loop so
	// its EXIT can resume the map state it came from (GB13-021 AC 3).
	_savedCallerLocation = _location;
	_hasSavedCallerLocation = true;
}

bool PoolradEngine::restoreCallerLocation() {
	// Restore the saved caller location on service EXIT. A NEW ECL transition
	// does not call this; it replaces _location via initializeMapRuntimeForState
	// instead.
	if (!_hasSavedCallerLocation)
		return false;
	_location = _savedCallerLocation;
	_hasSavedCallerLocation = false;
	return true;
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

void PoolradEngine::applyLocationContext(
		const ::Goldbox::RuntimeMapSnapshot &snapshot, GameState dungeonState) {
	ECL::EclLayoutAccess layout = _eclConfig.getLayoutAccess();
	ECL::AddressSpace &mem = _eclVm->getMemory();
	const uint16 rtGameStateAddr = layout.runtimeField(ECL::kEclRuntimeGameState);

	// GB13-021 NewEclDone: start every on-load entry as Dungeon, then assign a
	// wilderness kind ONLY when the 3D-view flag is zero (indoor flag clear)
	// AND the active script id is one of the three wilderness scripts
	// (25/26/27 = 0x19/0x1A/0x1B). classify() is the single boundary; it
	// distinguishes west/central/east (region 0/13/26) rather than collapsing
	// them. The authoritative location context is the TYPE owner; the game
	// state and mapType are derived from it.
	const uint8 mapId = snapshot.mapId;
	const uint8 view3dFlag = snapshot.indoorMode ? 1 : 0;
	_location.kind = LocationContext::classify(mapId, view3dFlag);
	_location.syncRegion();
	_location.activeEclId = mapId;
	_location.activeGeoId = snapshot.mapId;
	_location.dungeonPos = snapshot.dungeonPos;
	_location.dungeonDir = snapshot.dungeonDir & 0x03;
	_location.wildernessPos = snapshot.wildernessPos;
	_location.indoorRender = snapshot.indoorMode;

	// GB13-024 (Req 39.4): YarashComplete one-time world change at the
	// NewEclDone CENTRAL-REGION gate. Runs ONCE when (mission == 254 AND
	// pyramid-left == 0), opening the river by flipping the persistent
	// pyramid-left flag. Guarded by that same persistent flag so it is
	// idempotent across redraw and survives service/battle return and
	// save/load. This does NOT change the GB13-021 location classification
	// above - it only consults the already-classified kind and the persistent
	// world-state, then mutates the one-time flag. The exact mission=254 field
	// and river-open bit are OPEN; the once-only STRUCTURE is driven through
	// the HostYarashWorldState predicates, never a guessed address.
	if (_location.kind == LocationKind::WildernessCentral) {
		HostYarashWorldState yarashWorld(mem, layout,
			_legacySharedState.boolYarashPyramidLeft);
		WildernessMap::runYarashComplete(yarashWorld);
	}

	if (_location.isWilderness())
		_legacySharedState.byteGameState = GS_WILDERNESS_MAP;
	else
		_legacySharedState.byteGameState = dungeonState;

	if (ECL::EclRuntimeLayout::isValidVmAddr(rtGameStateAddr)) {
		mem.write8(rtGameStateAddr,
			static_cast<uint8>(_legacySharedState.byteGameState));
		if (g_events) {
			g_events->postEclStateMessage(EclVmMessage::ST_GAME_STATE,
				static_cast<uint16>(_legacySharedState.byteGameState),
				EclVmMessage::VT_UINT8);
		}
	}
}

void PoolradEngine::initializeMapRuntimeForState(GameState state) {
	debug(3, "PoolradEngine::initializeMapRuntimeForState state=%d", (int)state);
	_mapRuntimeNeedsInit = false;
	_mapRuntimeReady = false;
	_newEclPhase = kNewEclIdle;
	_campEntryPending = false;

	// Clear menu row 24 immediately (visible before message dispatch).
	Graphics::Screen *screen = getScreen();
	if (screen) {
		screen->fillRect(Common::Rect(0, 24 * 8, 320, 25 * 8), 0);
	}

	// Hide the in-game menu during ONINIT.
	if (g_events) {
		g_events->postEclStateMessage(EclVmMessage::ST_INGAME_MENU_VISIBLE,
			0, EclVmMessage::VT_UINT8);
	}

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
	::Goldbox::RuntimeMapSnapshot snapshot;
	if (!captureRuntimeMapSnapshot(snapshot))
		return;

	const uint8 mapId = snapshot.mapId;
	_legacySharedState.byteMapId = mapId;

	applyLocationContext(snapshot, state);

	// G_SaveMapId writeback (original writes BYTE_MAP_ID to world state
	// before ECL_ONINIT and at the top of the main loop).
	const uint16 saveMapIdAddr = layout.vmField(kVmFieldSavedEclId).vmAddr;
	if (VmLayout::isValid(layout.vmField(kVmFieldSavedEclId)))
		mem.write8(saveMapIdAddr, mapId);

	// Dispatch ON_INIT at runtime bootstrap point (ENGINE_Execute(ECL_ONINIT)).
	debug(3, "PoolradEngine::initializeMapRuntimeForState dispatching ECL ON_INIT mapId=%u",
		(unsigned)mapId);
	const bool wasStateLoaded = _eclVm->stateLoaded;
	const VmResult initResult = runEclEntryPoint(ECL::kEclRuntimeOnInitEntry);

	// After ON_INIT, re-apply saved position. ON_INIT resets position to
	// map defaults (via LOAD_AREA_GEO), but the original restores
	// STRUCT_POSITION from the save tail after ON_INIT completes.
	if (wasStateLoaded) {
		_eclVm->stateLoaded = false;
		const uint16 xAddr = layout.vmGlobalField(kVmGlobalFieldDungeonX).vmAddr;
		const uint16 yAddr = layout.vmGlobalField(kVmGlobalFieldDungeonY).vmAddr;
		const uint16 dirAddr = layout.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr;
		mem.write8(xAddr, snapshot.dungeonPos.x & 0xFF);
		mem.write8(yAddr, snapshot.dungeonPos.y & 0xFF);
		mem.write8(dirAddr, snapshot.dungeonDir & 0x03);
		debug(3, "PoolradEngine::initializeMapRuntimeForState POST-ONINIT restored pos x=%u y=%u dir=%u",
			(unsigned)snapshot.dungeonPos.x, (unsigned)snapshot.dungeonPos.y,
			(unsigned)snapshot.dungeonDir);
	}

	// Post-ONINIT viewport refresh (mirrors GB_EngineMain after ENGINE_Execute):
	//   if (BYTE_GAME_STATE != GS_WILDERNESS_MAP) {
	//       if (BOOL_SCREEN_REFRESH) { GAME_ScreenByState(); BOOL_SCREEN_REFRESH=false; }
	//       GFX_ViewPortUpdate();
	//   }
	if (initResult != VM_YIELD && state != GS_WILDERNESS_MAP) {
		if (_eclVm->screenRefresh) {
			Views::InGameView *igv = getInGameView();
			if (igv)
				igv->applyScreenByState(state);
			_eclVm->screenRefresh = false;
			_eclFlags.screenRefresh = false;
		}
		if (_eclHost)
			_eclHost->refreshViewport();

		// Sync VM position/direction → InGameView after ONINIT.
		Views::InGameView *syncIgv = getInGameView();
		if (syncIgv) {
			ECL::AddressSpace &syncMem = _eclVm->getMemory();
			ECL::EclLayoutAccess syncLayout = _eclConfig.getLayoutAccess();
			const uint16 x = syncMem.read8(
				syncLayout.vmGlobalField(kVmGlobalFieldDungeonX).vmAddr);
			const uint16 y = syncMem.read8(
				syncLayout.vmGlobalField(kVmGlobalFieldDungeonY).vmAddr);
			const uint8 dir = static_cast<uint8>(
				syncMem.read8(syncLayout.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr) & 0x03);
			syncIgv->setMapPosition(x, y, static_cast<uint8>(dir * 2));
		}
	}

	if (initResult == VM_YIELD) {
		_eclFlags.suspended = true;
		return;
	}

	_mapRuntimeReady = true;

	// Show the in-game menu now that ON_INIT and viewport refresh are done
	// (equivalent to reaching DIALOG_InGame in the original).
	if (g_events) {
		g_events->postEclStateMessage(EclVmMessage::ST_INGAME_MENU_VISIBLE,
			1, EclVmMessage::VT_UINT8);
	}
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

	::Goldbox::RuntimeMapSnapshot snapshot;
	if (!captureRuntimeMapSnapshot(snapshot))
		return;

	_legacySharedState.byteMapId = snapshot.mapId;
	const uint16 menuStatusAddr = layout.runtimeField(ECL::kEclRuntimeMenuStatus);
	if (ECL::EclRuntimeLayout::isValidVmAddr(menuStatusAddr))
		_legacySharedState.byteMenuStatus = mem.read8(menuStatusAddr);
	_legacySharedState.bool3dRedraw = snapshot.skyboxRedraw;
	_legacySharedState.boolPictureReady = (snapshot.pictureHeadId != 0xFF);

	_eclFlags.eclReady = _eclVm->eclReady;
	_eclFlags.geoReady = _eclVm->geoReady;
	_eclFlags.wallsetReady = _eclVm->wallsetReady;
	_eclFlags.screenRefresh = _eclVm->screenRefresh;

	const bool newMapDataReady = _eclVm->geoReady && _eclVm->wallsetReady;
	if (newMapDataReady && _eclVm->screenRefresh && !_eclFlags.mapDataReady) {
		// Original: BOOL_SCREEN_REFRESH = false after ScreenByState fires.
		_eclVm->screenRefresh = false;
		_eclFlags.screenRefresh = false;
	}
	_eclFlags.mapDataReady = newMapDataReady;
}

VmResult PoolradEngine::runEclEntryPoint(ECL::EclRuntimeFieldId entryField,
		uint32 maxSteps) {
	if (!_eclVm)
		return VM_ERROR;

	// Map runtime field ID to entry point index (0-4).
	uint8 entryIndex = 0xFF;
	switch (entryField) {
	case ECL::kEclRuntimeOnMoveEntry:          entryIndex = 0; break;
	case ECL::kEclRuntimeOnSearchEntry:        entryIndex = 1; break;
	case ECL::kEclRuntimeOnRestEntry:          entryIndex = 2; break;
	case ECL::kEclRuntimeOnRestInterruptEntry: entryIndex = 3; break;
	case ECL::kEclRuntimeOnInitEntry:          entryIndex = 4; break;
	default: return VM_ERROR;
	}

	// Use immutable parsed entry points (not runtime memory which scripts can overwrite).
	const uint16 entryPc = _eclVm->getEntryPointPc(entryIndex);
	debug(3, "PoolradEngine::runEclEntryPoint field=%d index=%u entryPc=0x%04X",
		(int)entryField, (unsigned)entryIndex, entryPc);
	if (entryPc == 0) {
		debug(3, "PoolradEngine::runEclEntryPoint entryPc=0, skipping");
		return VM_OK;
	}

	const VmResult result = executeEclAtScriptAddress(entryPc, maxSteps);
	debug(3, "PoolradEngine::runEclEntryPoint result=%d eclReady=%d",
		(int)result, (int)_eclVm->eclReady);
	_eclFlags.eclReady = _eclVm->eclReady;
	return result;
}

VmResult PoolradEngine::runProgramService(uint8 programID) {
	if (!_eclVm)
		return VM_ERROR;

	const ProgramService service = resolveProgramService(programID);
	if (service == kServiceUnknown) {
		warning("PoolradEngine::runProgramService: unknown PROGRAM id %u",
			(unsigned)programID);
		return VM_ERROR;
	}

	switch (service) {
	case kServiceCamp: {
		// PROGRAM 9 = EnterCamp. Mirror the kCmdEncamp flow: ONREST, then the
		// camp sub-loop (GS_CAMPING), then the optional camp interruption
		// (ONRESTINTR), then restore the saved instruction position and EXIT.
		//
		// A NEW ECL during the camp flow replaces the script; the obsolete
		// saved PC is NOT resumed in that case (decideServiceWait suppresses
		// the resume on eclReplaced). Quit unwinds cleanly from every wait.
		//
		// OPEN (trace-gated): the PRECISE saved-PC/EXIT tail and the exact
		// nested-ONRESTINTR ordering vs. the original Inst_PROGRAM/EnterCamp
		// are not byte-confirmed from the implementation package; the
		// confirmable structure (ONREST -> camp -> optional ONRESTINTR ->
		// saved-PC/EXIT with quit/NEW-ECL guards) is implemented here.
		saveCallerLocation();

		_eclVm->eclReady = false;
		const VmResult onRest = runEclEntryPoint(ECL::kEclRuntimeOnRestEntry);
		if (onRest == VM_YIELD) {
			_eclFlags.suspended = true;
			return VM_YIELD;
		}

		ServiceWaitSignals sig;
		sig.quitting = shouldQuit();
		// A NEW ECL inside ONREST replaces the script: eclReady set means the
		// script requested a context replacement, so the saved PC is obsolete.
		sig.eclReplaced = _eclVm->eclReady;
		ServiceWaitDecision dec = decideServiceWait(sig);
		if (dec.exitCleanly)
			return VM_OK;
		if (!dec.resumeSavedPc) {
			// NEW ECL replaced the script during ONREST: do not resume the
			// obsolete PC; the replacement script drives from here.
			return VM_OK;
		}

		// ONREST allowed the rest (EXIT): drive the shared camp/rest operation
		// and decide, from its completion reason, whether to run the camp
		// interruption entry (ONRESTINTR).
		const bool interrupted = runRest();

		if (interrupted) {
			_eclVm->eclReady = false;
			const VmResult onIntr =
				runEclEntryPoint(ECL::kEclRuntimeOnRestInterruptEntry);
			if (onIntr == VM_YIELD) {
				_eclFlags.suspended = true;
				return VM_YIELD;
			}
			// A NEW ECL inside ONRESTINTR likewise replaces the script.
			ServiceWaitSignals intrSig;
			intrSig.quitting = shouldQuit();
			intrSig.eclReplaced = _eclVm->eclReady;
			ServiceWaitDecision intrDec = decideServiceWait(intrSig);
			if (intrDec.exitCleanly || !intrDec.resumeSavedPc)
				return VM_OK;
		}

		// Normal completion: restore the caller location/interface and EXIT so
		// the PROGRAM opcode resumes at the saved instruction position.
		restoreCallerLocation();
		return VM_OK;
	}

	case kServiceWinGame: {
		// PROGRAM 8 = Ending.WinGame. The original composes the FINAL ending
		// assets with timed waits and then restores the saved interface state;
		// it is NOT VM_HALTED and NOT the generic GS_END_GAME game-over view.
		//
		// OPEN (trace-gated): the exact FINAL asset ids, the image composition
		// order and the wait timing in Ending.WinGame are not confirmed from
		// the implementation package, and no dedicated WinGame/ending view is
		// registered yet (only "Title"/"GameOver" exist). A new
		// epilogue/credits/achievement flow is explicitly OUT OF SCOPE (source
		// note), so none is invented here.
		//
		// Confirmable structure implemented: guard the ending against quit and
		// a missing ending resource (fail truthfully, do NOT delete the party
		// and do NOT fabricate a view), then present the victory sequence via
		// the real ending path and restore the saved interface afterward.
		// Until the dedicated WinGame view/asset path is wired, this drives the
		// structural flow and returns a non-halting result so the ECL resumes
		// to restore the saved interface (never a bare VM_HALTED).
		ServiceWaitSignals sig;
		sig.quitting = shouldQuit();
		// The ending asset path is not yet confirmed/available; treat it as a
		// missing resource so the flow fails truthfully rather than pretending
		// to have shown the FINAL composition.
		sig.resourceMissing = true;
		ServiceWaitDecision dec = decideServiceWait(sig);
		if (dec.exitCleanly)
			return VM_OK;
		if (dec.failTruthfully) {
			// Missing ending resource: report truthfully. The real party is
			// retained (no GS_END_GAME, no party deletion). This is the
			// documented OPEN seam where the FINAL WinGame assets/composition
			// are wired once confirmed.
			warning("PoolradEngine::runProgramService: PROGRAM 8 (WinGame) "
				"ending FINAL assets/composition are OPEN (trace-gated); "
				"party retained, interface preserved, not halting");
			return VM_OK;
		}
		// When the dedicated ending view is wired, present it here with waits
		// and restore the saved interface on completion. Never VM_HALTED.
		return VM_OK;
	}

	case kServiceTraining: {
		// PROGRAM 0 = roster/training workflow. Preserve the caller location so
		// the service EXIT returns to the caller's interface instead of
		// unconditionally choosing a map state; the TrainingRules operation and
		// the main-menu roster actions (GB13-020) own the actual workflow.
		//
		// OPEN (trace-gated): the exact main-menu/training entry that
		// PROGRAM 0 should present (and the confirmation / mage-spell dialog
		// wiring) is documented OPEN in training_rules.cpp / mainmenu_view.cpp.
		// The selection is NOT reset to the party front here (GB13-022).
		saveCallerLocation();

		ServiceWaitSignals sig;
		sig.quitting = shouldQuit();
		ServiceWaitDecision dec = decideServiceWait(sig);
		if (dec.exitCleanly)
			return VM_OK;

		// Return to the caller's interface (the roster/training view path is
		// the OPEN seam; the caller location/interface is restored here).
		restoreCallerLocation();
		return VM_OK;
	}

	default:
		return VM_ERROR;
	}
}

void PoolradEngine::syncViewDirection(uint8 cardinal) {
	if (!_eclVm)
		return;
	ECL::EclLayoutAccess layout = _eclConfig.getLayoutAccess();
	ECL::AddressSpace &mem = _eclVm->getMemory();
	const uint16 dirAddr = layout.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr;
	mem.write8(dirAddr, cardinal & 0x03);
	if (g_events) {
		g_events->postEclVmMessage(dirAddr, static_cast<uint8>(cardinal & 0x03));
		g_events->postEclStateMessage(EclVmMessage::ST_POSITION_DIRTY, 1,
			EclVmMessage::VT_UINT8);
	}
	if (_eclHost)
		_eclHost->refreshViewport();
}

void PoolradEngine::restoreSelectionIfPresent(
		::Goldbox::Data::PlayerCharacter *savedSelection) {
	// GB13-022 (Req 28.4): re-select the saved character only if it is still a
	// live party member. Party membership is the stable identity here: a
	// character removed mid-command (GB13-006 removeLoadedCharacter) is no
	// longer found, so no stale pointer is restored.
	if (!savedSelection)
		return;
	for (Common::List<Goldbox::Data::PlayerCharacter *>::const_iterator it =
			_party.begin(); it != _party.end(); ++it) {
		if (*it == savedSelection) {
			setSelectedCharacter(savedSelection);
			return;
		}
	}
	// Saved selection is gone: leave the current selection as-is (no stale
	// restore). The dispatcher does not reset to the party front.
}

namespace {

// GB13-023: host implementation of the wilderness-travel side effects. The
// pure rules (clamp 0..15/0..35, region 0/13/26 atlas offset, block/river id
// sets, 12h-on-permitted-including-clamped-noop, no-time-on-blocked) live in
// WildernessTravel; this seam forwards them onto the engine. The two formerly
// stubbed seams now read the SHARED sources GB13-024 introduced:
//   * readAtlasTerrain: reads WildernessAtlas::terrainIdAt - the SAME 44x36
//     grid seam the wilderness DRAWING module (WildernessMap) reads, so both
//     share one source. The grid DATA itself is still OPEN (the seam returns 0
//     for every cell until the confirmed 44x36 grid lands), which keeps the
//     bounds-validated travel path passable end to end as before.
//   * yarashPyramidLeft: reads the SAME persistent pyramid-left/river-open
//     world-state flag the YarashComplete op sets
//     (LegacySharedRuntimeState::boolYarashPyramidLeft), so completing Yarash
//     opens the river for this travel gate too. The exact VM address remains
//     OPEN; this is the engine-side mirror, not a guessed address.
//   * runScriptTransitions: the exact wilderness ONMOVE/clock/ONSEARCH order
//     is documented OPEN; the permitted-move commit + 12h advance happen
//     first, then the transition hook runs.
class HostWildernessEnvironment : public WildernessTravel::Environment {
public:
	HostWildernessEnvironment(ECL::AddressSpace &mem,
			ECL::EclLayoutAccess layout,
			const Goldbox::TimeFieldAddresses &clockAddrs,
			Common::List<Goldbox::Data::PlayerCharacter *> &party,
			Goldbox::Data::Effects::EffectHandlerBase *handler,
			Goldbox::GameState gameState,
			LocationContext &location,
			bool yarashPyramidLeft)
		: _mem(mem), _layout(layout), _clockAddrs(clockAddrs), _party(party),
		  _handler(handler), _gameState(gameState), _location(location),
		  _yarashPyramidLeft(yarashPyramidLeft) {}

	void currentLocal(int &localX, int &localY) override {
		localX = (int)_mem.read8(_layout.vmField(kVmFieldWildernessX).vmAddr);
		localY = (int)_mem.read8(_layout.vmField(kVmFieldWildernessY).vmAddr);
	}

	uint8 regionOffset() override {
		// Reuse the region offset already computed by LocationContext
		// (0/13/26). Keep it in sync with the kind before reading.
		_location.syncRegion();
		return _location.region;
	}

	bool yarashPyramidLeft() override {
		// Reads the SAME persistent pyramid-left/river-open flag the
		// YarashComplete op sets (passed in from
		// LegacySharedRuntimeState::boolYarashPyramidLeft). OPEN: the confirmed
		// VM address of the bit is unknown; this is the engine-side mirror, not
		// a guessed address. Completing Yarash opens the river for this gate.
		return _yarashPyramidLeft;
	}

	uint8 readAtlasTerrain(int atlasX, int atlasY) override {
		// Reads the SHARED 44x36 grid seam (WildernessAtlas) the drawing module
		// reads too. OPEN: the grid DATA is not yet populated, so this returns
		// 0 (baseline/empty => passable) for every in-range cell until the
		// confirmed grid lands; the bounds-validated (atlasX,atlasY) is honored.
		return WildernessAtlas::terrainIdAt(atlasX, atlasY);
	}

	void commitLocal(int localX, int localY) override {
		const uint8 x = (uint8)(localX & 0xFF);
		const uint8 y = (uint8)(localY & 0xFF);
		_mem.write8(_layout.vmField(kVmFieldWildernessX).vmAddr, x);
		_mem.write8(_layout.vmField(kVmFieldWildernessY).vmAddr, y);
		_location.wildernessPos = TilePos(x, y);
		if (g_events) {
			g_events->postEclVmMessage(
				_layout.vmField(kVmFieldWildernessX).vmAddr, x);
			g_events->postEclVmMessage(
				_layout.vmField(kVmFieldWildernessY).vmAddr, y);
		}
	}

	void advanceTwelveHours() override {
		// A permitted overworld step costs 12 HOURS (field = 3 (hour),
		// amount = 12) - DOS-reference timing - charged once per permitted
		// move, including a clamped no-op move.
		Goldbox::timeAddUnits(_mem, _clockAddrs, _party, _handler,
			_gameState, 3, 12);
	}

	void runScriptTransitions() override {
		// OPEN (trace-gated): the exact wilderness ONMOVE/clock/ONSEARCH order
		// is not byte-confirmed. The commit + 12h advance already ran; the
		// confirmable transition hook is left as a no-op seam until the
		// original overworld sequence is traced.
	}

private:
	ECL::AddressSpace &_mem;
	ECL::EclLayoutAccess _layout;
	const Goldbox::TimeFieldAddresses &_clockAddrs;
	Common::List<Goldbox::Data::PlayerCharacter *> &_party;
	Goldbox::Data::Effects::EffectHandlerBase *_handler;
	Goldbox::GameState _gameState;
	LocationContext &_location;
	bool _yarashPyramidLeft;
};

} // namespace

void PoolradEngine::dispatchPlayerCommand() {
	if (!_eclVm || _eclFlags.suspended)
		return;

	// Don't accept player commands until ONINIT has fully completed.
	if (!_mapRuntimeReady)
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

	// GB13-022 (Req 28.4): PRESERVE the selected character across the command
	// instead of resetting to the party front. The original reset
	// PTR_SELECTED_CHAR to PTR_PARTY_ARRAY (party.front()) on every command;
	// that reset is removed. We save the real selection at the loop boundary
	// here and restore it at each non-yield return boundary below. On a yield
	// the selection is left untouched so it survives the suspend/resume (and a
	// NEW ECL transition); a character removed mid-yield is handled because the
	// restore below only re-selects a member still present in the party.
	::Goldbox::Data::PlayerCharacter *const savedSelection =
		getSelectedCharacter();

	// G_SaveMapId writeback: kVmFieldSavedEclId is maintained by the
	// NEWECL handler and initializeMapRuntimeForState. Don't overwrite
	// it here — byteMapId tracks GEO block ID which can differ from ECL ID.

	// GB13-023 (Req 33): wilderness DIRECTIONAL-TRAVEL path, selected by the
	// LocationContext (overworld regions). It is distinct from the dungeon
	// move/turn path below: the view converts the arrow to a compass direction
	// at one boundary and queues kCmdWildernessMove; the pure WildernessTravel
	// rules clamp (0..15 / 0..35), apply the region 0/13/26 atlas offset with
	// signed pre-clamp math + bounds validation, test the blocked-terrain set
	// and the river gate, and decide the 12h-vs-0 time outcome. A permitted
	// move (including a clamped no-op) advances 12 hours; a blocked move does
	// not move and advances no time.
	if (cmd == Views::InGameView::kCmdWildernessMove) {
		if (!_location.isWilderness()) {
			// Defensive: a wilderness command outside a wilderness context is
			// ignored (no dungeon fallthrough), selection preserved.
			restoreSelectionIfPresent(savedSelection);
			return;
		}

		HostWildernessEnvironment wildEnv(mem, layout, kPoolradClockAddrs,
			_party, effectsRuntime(), getGameState(), _location,
			_legacySharedState.boolYarashPyramidLeft);
		WildernessTravel travel;
		const WildernessTravel::Compass dir = inGameView->getTravelDir();
		const uint16 wxAddr = layout.vmField(kVmFieldWildernessX).vmAddr;
		const uint16 wyAddr = layout.vmField(kVmFieldWildernessY).vmAddr;
		const uint8 prevX = mem.read8(wxAddr);
		const uint8 prevY = mem.read8(wyAddr);
		travel.step(wildEnv, dir);

		// Sync the committed wilderness position back to the view.
		const uint16 wx = mem.read8(wxAddr);
		const uint16 wy = mem.read8(wyAddr);
		if (wx != prevX || wy != prevY)
			playSfx(kSfxStep);
		inGameView->setMapPosition(wx, wy, inGameView->getMapDir());

		restoreSelectionIfPresent(savedSelection);
		return;
	}

	if (cmd == Views::InGameView::kCmdEncamp) {
		_eclVm->eclReady = false;
		const VmResult r = runEclEntryPoint(ECL::kEclRuntimeOnRestEntry);
		if (r == VM_YIELD) {
			// The camp menu still follows once ONREST completes.
			_campEntryPending = true;
			_eclFlags.suspended = true;
			return;
		}
		restoreSelectionIfPresent(savedSelection);
		enterCampAfterOnRest();
		return;
	}

	if (cmd == Views::InGameView::kCmdLeaveCamp) {
		restoreSelectionIfPresent(savedSelection);
		leaveCamp(inGameView->wasCampInterrupted());
		return;
	}

	if (cmd == Views::InGameView::kCmdSearch) {
		// 'S' toggle: XOR bit 0 of D_SearchFlags in VM memory.
		// No script runs - this is the persistent search-while-walking mode.
		const uint16 searchAddr =
			layout.vmGlobalField(kVmGlobalFieldSearchFlags).vmAddr;
		const uint8 flags = mem.read8(searchAddr);
		const uint8 newFlags = static_cast<uint8>(flags ^ 1);
		mem.write8(searchAddr, newFlags);
		if (g_events)
			g_events->postEclVmMessage(searchAddr, newFlags);
		restoreSelectionIfPresent(savedSelection);
		return;
	}

	if (cmd == Views::InGameView::kCmdLook) {
		// 'L' (Look): one-shot search.
		// D_SearchFlags |= 2, advance time, run ECL_ONSEARCH once, restore flags.
		const uint16 searchAddr =
			layout.vmGlobalField(kVmGlobalFieldSearchFlags).vmAddr;
		const uint8 savedSearchFlag = static_cast<uint8>(mem.read8(searchAddr) & 1);
		const uint8 lookFlags = static_cast<uint8>(savedSearchFlag | 2);
		mem.write8(searchAddr, lookFlags);
		if (g_events)
			g_events->postEclVmMessage(searchAddr, lookFlags);

		// GB13-022 (Req 28.3, DEFECT B): Look advances ONE TURN, charged once.
		// The original charged two minutes here (field=1, amount=2); that is
		// wrong - Look is a one-shot search, so it uses the same one-turn form
		// (field=2, amount=1) as the search-move path in this same dispatcher.
		if (_eclVm) {
			timeAddUnits(_eclVm->getMemory(), kPoolradClockAddrs,
				_party, effectsRuntime(), getGameState(), 2, 1);
		}

		// Now enter the search-loop path: set flags=1, run ONSEARCH, restore.
		mem.write8(searchAddr, 1);
		if (g_events)
			g_events->postEclVmMessage(searchAddr, static_cast<uint8>(1));
		// Refresh MapSquareInfo so ONSEARCH script can check bit 7.
		if (_eclHost)
			_eclHost->readGeoAtPosition();
		const VmResult onSearch = runEclEntryPoint(ECL::kEclRuntimeOnSearchEntry);
		mem.write8(searchAddr, savedSearchFlag);
		if (g_events)
			g_events->postEclVmMessage(searchAddr, savedSearchFlag);
		if (onSearch == VM_YIELD) {
			// Yield: leave the selection untouched so it survives the resume
			// (GB13-022 Req 28.4). Flags were already restored above.
			_eclFlags.suspended = true;
			return;
		}

		if (!runNewEclDone())
			return;
		restoreSelectionIfPresent(savedSelection);
		return;
	}

	if (cmd != Views::InGameView::kCmdMove)
		return;

	// GB13-022 (Req 28.1/28.2, DEFECT C): reordered exploration phases. The
	// original committed the destination coordinates AND charged time BEFORE
	// running ONMOVE. The corrected order is PREPARE_INTENT -> EXEC_ONMOVE ->
	// RESOLVE: prepare a prospective destination + leave flag WITHOUT writing
	// x/y or charging time, run ONMOVE first, and only then commit position and
	// advance the clock once. The resumable phase semantics are encoded and
	// unit tested in poolrad/exploration_operation.*; this dispatcher is the
	// thin host driver that wires those phases onto the engine calls while
	// preserving the existing yield/resume contract.
	const uint16 moveDirAddr =
		layout.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr;
	const uint16 moveXAddr = layout.vmGlobalField(kVmGlobalFieldDungeonX).vmAddr;
	const uint16 moveYAddr = layout.vmGlobalField(kVmGlobalFieldDungeonY).vmAddr;
	const uint8 moveCardinal =
		static_cast<uint8>((inGameView->getMapDir() / 2) & 0x03);

	// Prospective destination, computed in PREPARE_INTENT (NOT yet committed).
	int pendingDestX = 0;
	int pendingDestY = 0;
	bool pendingValid = false; // false => blocked (wall or closed door): no move

	// ----- PREPARE_INTENT -------------------------------------------------
	{
		// Sync facing direction to VM memory (position stays authoritative in
		// VM memory; only direction is changed by InGameView).
		mem.write8(moveDirAddr, moveCardinal);

		// Save the pre-move position to SavedMapX/SavedMapY. Scripts use
		// SAVE(0x49F0, 0xC04B) to restore the pre-move position.
		mem.write8(layout.vmField(kVmFieldSavedMapX).vmAddr,
			mem.read8(moveXAddr));
		mem.write8(layout.vmField(kVmFieldSavedMapY).vmAddr,
			mem.read8(moveYAddr));

		// Edge/leave flag (TriedToLeaveMap): cleared at the start of the step,
		// set below if the prospective destination clamps at a border. This is
		// part of preparation in the original - NOT a side-effect-free prepare.
		const uint16 leaveAddr =
			layout.vmGlobalField(kVmGlobalFieldTriedToLeaveMap).vmAddr;
		mem.write8(leaveAddr, 0);

		const Direction wireDir = static_cast<Direction>(moveCardinal * 2);

		// Barrier predicate (original MAP_StepForward wall check).
		RuntimeGeoBlock &rtGeo = getRuntimeGeo();
		bool blocked = false;
		bool solidWall = false;
		if (rtGeo.isLoaded()) {
			const int cx = static_cast<int>(mem.read8(moveXAddr));
			const int cy = static_cast<int>(mem.read8(moveYAddr));
			const uint8 wallFlag = rtGeo.getWallFlag(
				MapPos(static_cast<int8>(cx), static_cast<int8>(cy)), wireDir);
			if (wallFlag == 0 || wallFlag >= 2) {
				// wallFlag 0 = solid wall, 2/3 = locked door.
				// Locked door: DoorDialog/bash/pick/Knock handled by
				// tryOpenDoor(), holding pending movement until it finishes.
				if (wallFlag >= 2 && _eclHost)
					_eclHost->tryOpenDoor();
				blocked = true;
				solidWall = (wallFlag == 0);
			}
		}

		if (blocked) {
			// No move, no time commit. Notify direction and restore selection.
			// ResolveMovement bumps on a wall; the door dialog plays its own.
			if (solidWall)
				playSfx(kSfxImpact);
			if (g_events)
				g_events->postEclVmMessage(moveDirAddr, moveCardinal);
			restoreSelectionIfPresent(savedSelection);
			return;
		}

		// Compute the prospective (clamped) destination WITHOUT committing it.
		int x = static_cast<int>(mem.read8(moveXAddr));
		int y = static_cast<int>(mem.read8(moveYAddr));
		x += kDirDeltaX[inGameView->getMapDir()];
		y += kDirDeltaY[inGameView->getMapDir()];

		bool clamped = false;
		if (x > 15) { x = 15; clamped = true; }
		if (x < 0)  { x = 0;  clamped = true; }
		if (y > 15) { y = 15; clamped = true; }
		if (y < 0)  { y = 0;  clamped = true; }
		if (clamped)
			mem.write8(leaveAddr, 1); // edge/leave flag (part of preparation)

		pendingDestX = x;
		pendingDestY = y;
		pendingValid = true;
	}

	// Read geo event data at the current position (MAP_getGEOData equivalent)
	// so ONMOVE can decide which event to trigger. No coordinate/time commit.
	if (_eclHost)
		_eclHost->readGeoAtPosition();

	// ----- EXEC_ONMOVE (runs BEFORE any commit) ---------------------------
	const VmResult onMove = runEclEntryPoint(ECL::kEclRuntimeOnMoveEntry);
	if (onMove == VM_YIELD) {
		// Suspend at the ONMOVE boundary: NO coordinate/time commit happened.
		// Selection is left untouched so it survives the resume.
		_eclFlags.suspended = true;
		return;
	}

	// ----- RESOLVE --------------------------------------------------------
	// If ONMOVE fired NEW ECL (eclReady), the NEW-ECL transition path below
	// replaces the normal move: do NOT commit the prospective coordinates or
	// charge time. Otherwise commit the destination and advance the clock once.
	if (!_eclVm->eclReady && pendingValid) {
		// Commit coordinates now (write DungeonX/DungeonY).
		mem.write8(moveXAddr, static_cast<uint8>(pendingDestX));
		mem.write8(moveYAddr, static_cast<uint8>(pendingDestY));
		// MovePartyForward3D
		playSfx(kSfxStep);
		if (g_events) {
			g_events->postEclVmMessage(moveDirAddr, moveCardinal);
			g_events->postEclVmMessage(moveXAddr,
				static_cast<uint8>(pendingDestX));
			g_events->postEclVmMessage(moveYAddr,
				static_cast<uint8>(pendingDestY));
			g_events->postEclStateMessage(EclVmMessage::ST_POSITION_DIRTY, 1,
				EclVmMessage::VT_UINT8);
		}

		// Advance the clock exactly ONCE based on search mode. One minute for
		// an ordinary dungeon move (field=1, amount=1), one turn for a search
		// move (field=2, amount=1). Charged only on this path (never twice).
		const uint16 searchAddr =
			layout.vmGlobalField(kVmGlobalFieldSearchFlags).vmAddr;
		const uint8 searchFlags = mem.read8(searchAddr);
		if ((searchFlags & 1) == 0)
			timeAddUnits(mem, kPoolradClockAddrs, _party,
				effectsRuntime(), getGameState(), 1, 1);
		else
			timeAddUnits(mem, kPoolradClockAddrs, _party,
				effectsRuntime(), getGameState(), 2, 1);
	}

	// Sync VM position back to InGameView after ONMOVE/commit.
	{
		const uint16 x = mem.read8(moveXAddr);
		const uint16 y = mem.read8(moveYAddr);
		const uint8 d = static_cast<uint8>(mem.read8(moveDirAddr) & 0x03);
		inGameView->setMapPosition(x, y, static_cast<uint8>(d * 2));
	}

	if (!_eclVm->eclReady) {
		// Legacy post-ONMOVE branch: save position, try door, check bump.
		RuntimeMapSnapshot snap;
		captureRuntimeMapSnapshot(snap);
		const uint16 savedX = snap.dungeonPos.x;
		const uint16 savedY = snap.dungeonPos.y;

		// DIALOG_OpenDoor: attempt to open door in facing direction.
		if (_eclHost)
			_eclHost->tryOpenDoor();

		// Re-read position after door logic (position may have changed).
		captureRuntimeMapSnapshot(snap);
		if (snap.dungeonPos.x != savedX || snap.dungeonPos.y != savedY) {
			// Step after a position change (EclVm main loop)
			playSfx(kSfxStep);
		}

		_legacySharedState.bool3dRedraw = false;
		_legacySharedState.boolPictureReady = true;

		// Always run ONSEARCH unconditionally after movement.
		// The script itself handles PRINTCLEAR, PICTURE(255), and checks
		// D_SearchFlags internally to decide what to do.
		if (_eclHost)
			_eclHost->readGeoAtPosition();

		const VmResult onSearch = runEclEntryPoint(ECL::kEclRuntimeOnSearchEntry);

		if (onSearch == VM_YIELD)
			_eclFlags.suspended = true;
		else if (!runNewEclDone())
			return;
		// GB13-022 (Req 28.4): restore the saved selection (stable-id checked)
		// only after the yielded scripts/transitions completed on this path.
		restoreSelectionIfPresent(savedSelection);
		return;
	}

	// eclReady == true: NEWECL fired during ON_MOVE.
	if (!runNewEclDone())
		return;

	// GB13-022 (Req 28.4): NEW-ECL transition drained - restore the saved
	// selection (stable-id checked) at the loop boundary before awaiting input.
	restoreSelectionIfPresent(savedSelection);
}

void PoolradEngine::enterCampAfterOnRest() {
	// A NEW ECL from ONREST replaces the map; the camp is not opened.
	if (_eclVm && _eclVm->eclReady) {
		runNewEclDone();
		return;
	}

	// ONREST may have left the map (combat, shop, party defeated).
	const GameState state = getGameState();
	if (state != GS_DUNGEON_MAP && state != GS_WILDERNESS_MAP)
		return;

	// InGameView::applyScreenByState activates CampMenuDialog.
	setGameState(GS_CAMPING);
}

void PoolradEngine::leaveCamp(bool interrupted) {
	if (getGameState() == GS_CAMPING)
		setGameState(mapStateForLocation(_location));

	// Show the menu before ONRESTINTR so the host's prompt handling
	// hides and restores it like for any other script.
	Views::InGameView *inGameView = getInGameView();
	if (inGameView)
		inGameView->setInGameMenuVisible(true);

	if (!interrupted || !_eclVm)
		return;

	_eclVm->eclReady = false;
	const VmResult r =
		runEclEntryPoint(ECL::kEclRuntimeOnRestInterruptEntry);
	if (r == VM_YIELD) {
		_eclFlags.suspended = true;
		return;
	}
	if (_eclVm->eclReady)
		runNewEclDone();
}

bool PoolradEngine::runNewEclDone() {
	if (!_eclVm)
		return true;

	ECL::EclLayoutAccess layout = _eclConfig.getLayoutAccess();
	ECL::AddressSpace &mem = _eclVm->getMemory();

	// NewEclDone (eclvm.pas): on-load entry, map redraw, on-move entry, then
	// the search entry unless on-move ran NEWECL; repeat while NEWECL fires.
	// _newEclPhase records the step a yielded script belongs to, so tick()
	// can finish the sequence once the script resumes.
	for (;;) {
		switch (_newEclPhase) {
		case kNewEclIdle:
			if (!_eclVm->eclReady)
				return true;
			_eclVm->eclReady = false;
			if (_eclHost)
				_eclHost->readGeoAtPosition();
			mem.write8(layout.vmGlobalField(kVmGlobalFieldTriedToLeaveMap).vmAddr, 0);
			// EclInit selects the 3D view before the on-load entry runs.
			mem.write8(layout.vmField(kVmFieldIndoorModeFlag).vmAddr, 1);
			_newEclPhase = kNewEclOnInit;
			if (runEclEntryPoint(ECL::kEclRuntimeOnInitEntry) == VM_YIELD) {
				_eclFlags.suspended = true;
				return false;
			}
			// fall through
		case kNewEclOnInit: {
			::Goldbox::RuntimeMapSnapshot snapshot;
			if (captureRuntimeMapSnapshot(snapshot)) {
				const LocationKind kind = LocationContext::classify(
					snapshot.mapId, snapshot.indoorMode ? 1 : 0);
				if (kind != _location.kind)
					applyLocationContext(snapshot, GS_DUNGEON_MAP);
				_location.activeEclId = snapshot.mapId;
				// DisplayMapView re-reads the event byte from the GEO the
				// on-load entry just loaded.
				if (snapshot.indoorMode && _eclHost)
					_eclHost->readGeoAtPosition();
			}
			if (_eclHost)
				_eclHost->refreshViewport();

			// A NEWECL inside the on-load entry does not restart the sequence.
			_eclVm->eclReady = false;
			_newEclPhase = kNewEclOnMove;
			if (runEclEntryPoint(ECL::kEclRuntimeOnMoveEntry) == VM_YIELD) {
				_eclFlags.suspended = true;
				return false;
			}
		}
			// fall through
		case kNewEclOnMove:
			_newEclPhase = kNewEclOnSearch;
			if (!_eclVm->eclReady
					&& runEclEntryPoint(ECL::kEclRuntimeOnSearchEntry) == VM_YIELD) {
				_eclFlags.suspended = true;
				return false;
			}
			// fall through
		case kNewEclOnSearch:
		default:
			_newEclPhase = kNewEclIdle;
			break;
		}
	}
}

RuntimeExchange *PoolradEngine::getRuntimeExchange() {
	return _runtimeExchange.get();
}

const RuntimeExchange *PoolradEngine::getRuntimeExchange() const {
	return _runtimeExchange.get();
}

namespace {

// Host implementation of the rest side effects. Forwards the clock advance to
// timeAddUnits (field = minutes, amount = 5), heals via the ordinary HP
// restoration (clamped to max), and rolls d100 through the engine RNG. Kept
// out of the pure RestOperation so the timer/heal/prep logic stays testable.
class HostRestEnvironment : public RestOperation::Environment {
public:
	HostRestEnvironment(ECL::AddressSpace &mem,
			const Goldbox::TimeFieldAddresses &clockAddrs,
			Common::List<Goldbox::Data::PlayerCharacter *> &party,
			Goldbox::Data::Effects::EffectHandlerBase *handler,
			Goldbox::GameState gameState)
		: _mem(mem), _clockAddrs(clockAddrs), _party(party),
		  _handler(handler), _gameState(gameState) {}

	void advanceFiveMinutes() override {
		// Original GameTime.AdvanceTime(5, MINUTES): field = 1 (minutes),
		// amount = 5. Same call shape poolrad.cpp already uses for search.
		Goldbox::timeAddUnits(_mem, _clockAddrs, _party, _handler,
			_gameState, 1, 5);
	}

	bool restoreOneHitPoint(Data::PoolradCharacter *member) override {
		// Ordinary restoration: +1 HP, never above max, never a full heal.
		// Mirrors FxEngine.RestoreHitPoints(cur, 1, False).
		if (!member)
			return false;
		if (member->hitPoints.current >= member->hitPoints.max)
			return false;
		++member->hitPoints.current;
		return true;
	}

	int rollInterruptionPercent() override {
		return VmInterface::rollDice(1, 100);
	}

private:
	ECL::AddressSpace &_mem;
	const Goldbox::TimeFieldAddresses &_clockAddrs;
	Common::List<Goldbox::Data::PlayerCharacter *> &_party;
	Goldbox::Data::Effects::EffectHandlerBase *_handler;
	Goldbox::GameState _gameState;
};

} // namespace

bool PoolradEngine::runRest() {
	if (!_eclVm)
		return false;

	ECL::AddressSpace &mem = _eclVm->getMemory();
	ECL::EclLayoutAccess layout = _eclConfig.getLayoutAccess();

	// Gather the party as PoolradCharacters in list order.
	Common::Array<Data::PoolradCharacter *> party;
	for (Common::List<Goldbox::Data::PlayerCharacter *>::iterator it =
			_party.begin(); it != _party.end(); ++it) {
		party.push_back(static_cast<Data::PoolradCharacter *>(*it));
	}

	// Max regain time (ACTION_Rest): the party-wide maximum spell preparation
	// requirement, in minutes. The original reads CHARACTER_GetSpellRegain;
	// no C++ port of that helper exists yet, so the requested duration is
	// derived from pending work at 15 minutes per spell level (3 ticks/level *
	// 5 minutes). OPEN: swap to the exact CHARACTER_GetSpellRegain minutes if
	// a port lands.
	uint16 maxRegainMinutes = 0;
	for (uint i = 0; i < party.size(); ++i) {
		Data::PoolradCharacter *member = party[i];
		if (!member)
			continue;
		SpellPreparation prep(member);
		// Each pending memorize slot costs its spell level in 15-minute units
		// (3 ticks/level * 5 min). Estimate from the pending count; the tick
		// simulation enforces the exact per-level timing as it runs.
		const uint8 pendingMem = prep.countPendingMemorized();
		if (pendingMem > 0) {
			const uint16 estimate = (uint16)(pendingMem * 15);
			if (estimate > maxRegainMinutes)
				maxRegainMinutes = estimate;
		}
	}

	// Area safe-rest fields (zone1[REST_SAFE_TIME], zone1[REST_INTERRUPT_PCT]).
	const uint8 safeTime = mem.read8(
		layout.vmGlobalField(kVmGlobalFieldRestSafeTime).vmAddr);
	const uint8 interruptPct = mem.read8(
		layout.vmGlobalField(kVmGlobalFieldRestInterruptChance).vmAddr);

	RestOperation rest;
	rest.start(party, maxRegainMinutes, safeTime, interruptPct);

	HostRestEnvironment env(mem, kPoolradClockAddrs, _party,
		effectsRuntime(), getGameState());

	// Drive the simulation to completion. Ticks are bounded per iteration to
	// stay responsive without changing the simulated order or count; a real
	// frame-bounded driver would yield here between batches.
	const int kMaxTicksPerBatch = 64;
	while (rest.isResting()) {
		for (int i = 0; i < kMaxTicksPerBatch && rest.isResting(); ++i)
			rest.advanceTick(env);
	}

	return rest.shouldRunInterruptionEcl();
}

} // namespace Poolrad
} // namespace Goldbox
