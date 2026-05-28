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
#include "goldbox/poolrad/data/poolrad_vm_layout.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/poolrad/poolrad_runtime_exchange.h"
//#include "goldbox/poolrad/gfx/cursors.h"

#include "goldbox/poolrad/console.h"

#include <stdio.h>

namespace Goldbox {
namespace Poolrad {

namespace {

static bool isWildernessMapId(uint8 mapId) {
	return mapId == 0x19 || mapId == 0x1A || mapId == 0x1B;
}

static char toUpperAscii(char c) {
	if (c >= 'a' && c <= 'z')
		return static_cast<char>(c - ('a' - 'A'));
	return c;
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
    mem.write16LE(layout.vmGlobalField(kVmGlobalFieldDungeonX).vmAddr, 15);
    mem.write16LE(layout.vmGlobalField(kVmGlobalFieldDungeonY).vmAddr, 1);
    mem.write16LE(layout.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr, 2); // WEST

    // Map type defaults
    mem.write8(layout.vmGlobalField(kVmGlobalFieldMapWallType).vmAddr, 1);

    // Party count = 0
    mem.write8(layout.vmGlobalField(kVmGlobalFieldPartyCount).vmAddr, 0);

    // Picture head ID sentinel (0xFF = no picture loaded)
    mem.write8(layout.vmGlobalField(kVmGlobalFieldPictureHeadId).vmAddr, 0xFF);

    // Game speed default = 1
    mem.write16LE(layout.vmField(kVmFieldGameSpeed).vmAddr, 1);

    // Indoor mode = 1 (dungeon), map type = 1
    mem.write8(layout.vmField(kVmFieldIndoorModeFlag).vmAddr, 1);

    // Runtime game state = GS_START_MENU (0) while at main menu
    mem.write8(layout.runtimeField(ECL::kEclRuntimeGameState),
        static_cast<uint8>(GS_START_MENU));

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


	return true;
}


bool PoolradEngine::initializeRuntimeSystems() {
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
		// VM runtime bootstrap is orchestrator-owned and happens in tick().
		_mapRuntimeNeedsInit = true;
		debug(2, "PoolradEngine::onGameStateEnter map runtime init requested for state=%d",
			(int)next);
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
		debug(2, "PoolradEngine::onGameStateEnter -> replaceView(Title, true) [END_GAME]");
		// Placeholder: go back to title; layout irrelevant.
		replaceView("Title", true);
		view = dynamic_cast<Views::View *>(findView("Title"));
		if (view) {
			view->onEnter(next);
		}
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

Data::DaxBlockGeo *PoolradEngine::getGeoBlockById(uint8 mapId) {
	return dynamic_cast<Data::DaxBlockGeo *>(
		getDaxGeo().getBlockById(mapId));
}

Data::DaxBlockGeo *PoolradEngine::getActiveGeoBlock() {
	return getGeoBlockById(_legacySharedState.byteMapId);
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

	x = snapshot.dungeonX;
	y = snapshot.dungeonY;
	dir = snapshot.dungeonDir;
	return true;
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

	auto writeVmBlock = [&](uint16 startAddr, uint32 size) {
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

	writeVmBlock(0x4900, 0x0800); // VMBANK0_WORLD_STATE
	writeVmBlock(0x6B00, 0x0800); // VMBANK1_PARTY_STATE
	writeVmBlock(0x9700, 0x0400); // VMBANK2_COMBAT_STATE
	writeVmBlock(0x9900, 0x1E00); // VMBANK3_ECL_SCRIPT

	::Goldbox::RuntimeMapSnapshot snapshot;
	if (!captureRuntimeMapSnapshot(snapshot)) {
		errorMessage = "Failed to capture runtime snapshot";
		out.close();
		return false;
	}

	const uint16 posX = snapshot.dungeonX;
	const uint16 posY = snapshot.dungeonY;
	const uint8 posDir = static_cast<uint8>(snapshot.dungeonDir & 0x03);
	const uint8 vmMapType = snapshot.mapType;

	uint8 characterCount = 0;
	byte characterTable[0x148];
	memset(characterTable, 0, sizeof(characterTable));
	Common::Array<Common::String> usedBases;

	for (uint i = 0; i < _party.size(); ++i) {
		if (characterCount >= 8)
			break;

		Data::PoolradCharacter *pc =
			dynamic_cast<Data::PoolradCharacter *>(_party[i]);
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
	// +0x00 u16 posX, +0x02 u16 posY, +0x04 u8 dir, +0x05 u8 vmMapType,
	// +0x06 u8 gameState, +0x07 u8 charCount, +0x08..+0x14f char table.
	byte tail[0x150];
	memset(tail, 0, sizeof(tail));
	tail[0x00] = static_cast<byte>(posX & 0xFF);
	tail[0x01] = static_cast<byte>((posX >> 8) & 0xFF);
	tail[0x02] = static_cast<byte>(posY & 0xFF);
	tail[0x03] = static_cast<byte>((posY >> 8) & 0xFF);
	tail[0x04] = posDir;
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
	auto readVmBlock = [&](uint16 startAddr, uint32 size) {
		byte *buf = new byte[size];
		in->read(buf, size);
		mem->loadBytes(startAddr, Common::Span<const uint8>(buf, size));
		delete[] buf;
	};

	readVmBlock(0x4900, 0x0800); // VMBANK0_WORLD_STATE (GEO bank)
	readVmBlock(0x6B00, 0x0800); // VMBANK1_PARTY_STATE (DAT bank)
	readVmBlock(0x9700, 0x0400); // VMBANK2_COMBAT_STATE (HEAP bank)
	readVmBlock(0x9900, 0x1E00); // VMBANK3_ECL_SCRIPT  (ECL bank)

	// Read STRUCT_POSITION: uint16 x + uint16 y + uint8 dir (5 bytes total).
	// The system bank (not dumped above) holds the live position; restore below.
	const uint16 posX        = in->readUint16LE();
	const uint16 posY        = in->readUint16LE();
	const uint8 posDir       = in->readByte();

	// BYTE_VM_MAP_TYPE: dungeon/town (< 2) vs wilderness/combat (>= 2).
	const uint8 vmMapType = in->readByte();

	// BYTE_GAME_STATE: the saved GameState enum value.
	const uint8 byteGameState = in->readByte();

	// Character table: count byte + 8 Ã— 0x29-byte Pascal-style base filenames
	// (length byte + up to 0x28 chars).
	const uint8 characterCount = in->readByte();
	byte characterTable[0x148];
	in->read(characterTable, sizeof(characterTable));

	debug(2, "PoolradEngine::loadGameSlotX86 tail posX=%u posY=%u dir=%u mapType=%u gameState=%u charCount=%u",
		(unsigned)posX, (unsigned)posY, (unsigned)posDir,
		(unsigned)vmMapType, (unsigned)byteGameState,
		(unsigned)characterCount);
	debug(2, "PoolradEngine::loadGameSlotX86 decoded gameState=%u (%s)",
		(unsigned)byteGameState,
		(byteGameState == GS_START_MENU) ? "GS_START_MENU" : "runtime/ingame");

	delete in;
	in = nullptr;

	// -------------------------------------------------------------------------
	// Post-load: restore system bank position (not included in VM bank dumps).
	// -------------------------------------------------------------------------
	ECL::EclLayoutAccess layout = _eclConfig.getLayoutAccess();

	mem->write16LE(layout.vmGlobalField(kVmGlobalFieldDungeonX).vmAddr,   posX);
	mem->write16LE(layout.vmGlobalField(kVmGlobalFieldDungeonY).vmAddr,   posY);
	mem->write16LE(layout.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr,
		static_cast<uint16>(posDir));
	mem->write8(layout.vmGlobalField(kVmGlobalFieldMapWallType).vmAddr,
		vmMapType);

	// Reset party count in VM memory before rebuilding the party list.
	mem->write8(layout.vmGlobalField(kVmGlobalFieldPartyCount).vmAddr, 0);

	// -------------------------------------------------------------------------
	// Clear existing party and reload characters from individual save files.
	// -------------------------------------------------------------------------
	for (uint i = 0; i < _party.size(); ++i)
		delete _party[i];
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
	// Reload world graphics based on map type.
	// vmMapType < 2 -> dungeon / town (geo block + wall sets need reload).
	// vmMapType >= 2 -> wilderness / combat (icon block reload only).
	// -------------------------------------------------------------------------
	const bool loadIntoRuntime =
		(static_cast<GameState>(byteGameState) != GS_START_MENU);
	if (loadIntoRuntime) {
		if (vmMapType < 2 && _eclHost) {
			const uint8 geoBlockId =
				mem->read8(layout.vmField(kVmFieldGeoBlockId).vmAddr);
			_eclHost->loadGeoBlock(geoBlockId);

			// Restore saved wall set block IDs and slot IDs from VMBANK0.
			// G_SavedWallBlockIds (field474_0x3f2): vmAddr base = GEO_BASE + 0x3f2/2 = 0x4AF9
			//   [slot] -> vmAddr 0x4AF9 + slot  (slots 1-3: 0x4AFA, 0x4AFB, 0x4AFC)
			// G_SavedWallSlotIds  (field477_0x3f8): vmAddr base = GEO_BASE + 0x3f8/2 = 0x4AFC
			//   [slot] -> vmAddr 0x4AFC + slot  (slots 1-3: 0x4AFD, 0x4AFE, 0x4AFF)
			// Original x86 check is signed (JL): negative int16 means sentinel/invalid.
			static const uint16 kGeoSavedWallBlockBase = 0x4AF9;
			static const uint16 kGeoSavedWallSlotBase  = 0x4AFC;
			for (uint8 wallSlot = 1; wallSlot <= 3; ++wallSlot) {
				const int16 blockId = static_cast<int16>(mem->read16LE(
					static_cast<uint16>(kGeoSavedWallBlockBase + wallSlot)));
				const uint8 setSlot = static_cast<uint8>(mem->read16LE(
					static_cast<uint16>(kGeoSavedWallSlotBase + wallSlot)) & 0xFF);
				if (blockId >= 0)
					_eclHost->loadWallSet(static_cast<uint8>(blockId & 0xFF), setSlot);
			}
		} else if (_eclHost) {
			_eclHost->loadIconBlock();
		}
	} else {
		debug(2, "PoolradEngine::loadGameSlotX86 skipping geo/icon preload for GS_START_MENU transfer mode");
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
	_legacySharedState.byteGameState =
		static_cast<GameState>(byteGameState);
	_legacySharedState.byteMapId =
		mem->read8(layout.vmField(kVmFieldGeoBlockId).vmAddr);
	const bool loadedIntoRuntime = loadIntoRuntime;
	_legacySharedState.boolStateLoaded = loadedIntoRuntime;
	if (_eclVm)
		_eclVm->stateLoaded = loadedIntoRuntime;

	debug(2, "PoolradEngine::loadGameSlotX86 applying setGameState(%u)",
		(unsigned)byteGameState);
	setGameState(static_cast<GameState>(byteGameState));
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
	debug(3, "PoolradEngine::initializeMapRuntimeForState state=%d", (int)state);
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
	const uint16 rtGameStateAddr = layout.runtimeField(ECL::kEclRuntimeGameState);

	::Goldbox::RuntimeMapSnapshot snapshot;
	if (!captureRuntimeMapSnapshot(snapshot))
		return;

	const uint8 mapId = snapshot.mapId;
	_legacySharedState.byteMapId = mapId;
	if (isWildernessMapId(mapId) && !snapshot.indoorMode)
		_legacySharedState.byteGameState = GS_WILDERNESS_MAP;
	else
		_legacySharedState.byteGameState = state;

	if (ECL::EclRuntimeLayout::isValidVmAddr(rtGameStateAddr)) {
		mem.write8(rtGameStateAddr,
			static_cast<uint8>(_legacySharedState.byteGameState));
	}

	// G_SaveMapId writeback (original writes BYTE_MAP_ID to world state
	// before ECL_ONINIT and at the top of the main loop).
	const uint16 saveMapIdAddr = layout.vmField(kVmFieldSavedMapId).vmAddr;
	if (VmLayout::isValid(layout.vmField(kVmFieldSavedMapId)))
		mem.write8(saveMapIdAddr, mapId);

	// Dispatch ON_INIT at runtime bootstrap point (ENGINE_Execute(ECL_ONINIT)).
	debug(3, "PoolradEngine::initializeMapRuntimeForState dispatching ECL ON_INIT mapId=%u",
		(unsigned)mapId);
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
	debug(3, "PoolradEngine::runEclEntryPoint field=%d entryAddr=0x%04X entryPc=0x%04X",
		(int)entryField, entryAddr, entryPc);
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

	// PTR_SELECTED_CHAR = PTR_PARTY_ARRAY (reset to first party member)
	if (!_party.empty())
		setSelectedCharacter(_party[0]);

	// G_SaveMapId writeback (original does this at loop top)
	const uint16 saveMapIdAddr = layout.vmField(kVmFieldSavedMapId).vmAddr;
	if (VmLayout::isValid(layout.vmField(kVmFieldSavedMapId)))
		mem.write8(saveMapIdAddr, _legacySharedState.byteMapId);

	if (cmd == Views::InGameView::kCmdEncamp) {
		const VmResult r = runEclEntryPoint(ECL::kEclRuntimeOnRestEntry);
		if (r == VM_YIELD)
			_eclFlags.suspended = true;
		return;
	}

	if (cmd == Views::InGameView::kCmdSearch) {
		// 'S' toggle: XOR bit 0 of D_SearchFlags in VM memory.
		// No script runs - this is the persistent search-while-walking mode.
		const uint16 searchAddr =
			layout.vmGlobalField(kVmGlobalFieldSearchFlags).vmAddr;
		const uint8 flags = mem.read8(searchAddr);
		mem.write8(searchAddr, static_cast<uint8>(flags ^ 1));
		return;
	}

	if (cmd == Views::InGameView::kCmdLook) {
		// 'L' (Look): one-shot search.
		// D_SearchFlags |= 2, advance time, run ECL_ONSEARCH once, restore flags.
		const uint16 searchAddr =
			layout.vmGlobalField(kVmGlobalFieldSearchFlags).vmAddr;
		const uint8 savedSearchFlag = static_cast<uint8>(mem.read8(searchAddr) & 1);
		mem.write8(searchAddr, static_cast<uint8>(savedSearchFlag | 2));

		// TIME_AddUnits(1, 2) - advance clock by 2 minutes.
		// TODO: Wire TIME_AddUnits once clock system is implemented.

		// Now enter the search-loop path: set flags=1, run ONSEARCH, restore.
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
		// Legacy post-ONMOVE branch: save position, try door, check bump.
		RuntimeMapSnapshot snap;
		captureRuntimeMapSnapshot(snap);
		const uint16 savedX = snap.dungeonX;
		const uint16 savedY = snap.dungeonY;

		// DIALOG_OpenDoor: attempt to open door in facing direction.
		if (_eclHost)
			_eclHost->tryOpenDoor();

		// Re-read position after door logic (position may have changed).
		captureRuntimeMapSnapshot(snap);
		if (snap.dungeonX != savedX || snap.dungeonY != savedY) {
			// PlaySound(SOUND_ID_BLOCKED) - sound index 0x0B
			if (_eclHost)
				_eclHost->playSound(0x0B);
		}

		_legacySharedState.bool3dRedraw = false;
		_legacySharedState.boolPictureReady = true;
		const VmResult onSearch = runEclEntryPoint(ECL::kEclRuntimeOnSearchEntry);
		if (onSearch == VM_YIELD)
			_eclFlags.suspended = true;
		else if (_eclVm->eclReady) {
			const VmResult resume = executeEclAtScriptAddress(_eclVm->getPC());
			if (resume == VM_YIELD)
				_eclFlags.suspended = true;
		}
		return;
	}

	const VmResult resume = executeEclAtScriptAddress(_eclVm->getPC());
	if (resume == VM_YIELD)
		_eclFlags.suspended = true;
}

RuntimeExchange *PoolradEngine::getRuntimeExchange() {
	return _runtimeExchange.get();
}

const RuntimeExchange *PoolradEngine::getRuntimeExchange() const {
	return _runtimeExchange.get();
}

} // namespace Poolrad
} // namespace Goldbox
