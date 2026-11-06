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

#include "goldbox/gfx/first_person_renderer.h"
#include "goldbox/gfx/viewport_background.h"
#include "goldbox/gfx/area_map_cache.h"
#include "goldbox/gfx/encounter_sprite_cache.h"
#include "goldbox/gfx/picture_display_cache.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/vm_interface.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/poolrad/views/dialogs/in_game_main_screen_dialog.h"
#include "goldbox/poolrad/views/in_game_view.h"
#include "goldbox/poolrad/wilderness_map.h"
#include "goldbox/runtime/runtime_exchange.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

// Wilderness map-type X offsets used by MAP_3DColorUpdate / MAP_DrawAreaMapBlock.
// Verified from wilderness runtime paths in legacy code:
//   type 2 -> +0, type 3 -> +13, type 4 -> +26.
static const int kWildernessMapXOffsets[8] = {
	0, 0, 0, 13, 26, 0, 0, 0
};

InGameMainScreenDialog::InGameMainScreenDialog(const Common::String &name)
	: Dialog(name) {
	setBounds(Window(0, 0, 39, 24));
}

void InGameMainScreenDialog::setDrawMode(DrawMode mode) {
	if (_mode == mode)
		return;

	_mode = mode;
	redraw();
}

uint8 InGameMainScreenDialog::effectiveMapTypeFromRuntime() const {
	if (!::Goldbox::Poolrad::g_engine)
		return (_mode == kModeWilderness) ? 2 : 1;

	const RuntimeExchange *exchange =
		::Goldbox::Poolrad::g_engine->getRuntimeExchange();
	if (!exchange)
		return (_mode == kModeWilderness) ? 2 : 1;

	::Goldbox::RuntimeMapSnapshot snapshot;
	if (!exchange->captureMapSnapshot(snapshot) || !snapshot.valid)
		return (_mode == kModeWilderness) ? 2 : 1;

	uint8 mapType = snapshot.mapType;
	if (snapshot.indoorMode)
		mapType = 1;

	return mapType;
}

// GB13-024: host quest predicate for the nine-case site resolver. The exact
// per-site quest flags have no confirmed VM field (OPEN), so every condition
// reports NOT met: no override substitutes and every matched site stays
// baseline until the flags are traced. The resolver is read-only regardless.
namespace {
class HostSiteQuestPredicate : public WildernessMap::QuestPredicate {
public:
	bool questConditionMet(int siteIndex) override {
		(void)siteIndex;
		return false;
	}
};
} // namespace

void InGameMainScreenDialog::ensureWildernessTilesLoaded() {
	if (_wildernessTiles.isLoaded())
		return;
	if (!::Goldbox::Poolrad::g_engine)
		return;
	// WILDCOM 0-33 -> slots 0..33, RANDCOM 0-5 -> slots 34..39, mirroring
	// CombatTileCache::loadWilderness used by the combat viewport.
	_wildernessTiles.loadWilderness(
		::Goldbox::Poolrad::g_engine->getDaxWildcom(),
		::Goldbox::Poolrad::g_engine->getDaxRandcom());
}

void InGameMainScreenDialog::drawWildernessAreaBlock(Surface &s,
		int originAtlasX, int originAtlasY) {
	// GB13-024 (Req 39.1): the real MAP_DrawAreaMapBlock geometry. A 5x5
	// canonical window of 24x24 terrain tiles, each spaced by three 8-pixel
	// character cells, from the (3,3) char-cell origin. Cell terrain ids come
	// from the shared atlas grid seam; the subtract-one-once id->tile-slot rule
	// lives in WildernessMap::idToTileSlot (we do NOT subtract again here). The
	// window origin is clamped to x 0..39 / y 0..31 inside buildWindow.
	//
	// NOTE: the 44x36 terrain grid DATA is OPEN (WildernessAtlas returns 0 for
	// every cell until it lands), so today every cell resolves to id 0 ==
	// baseline/empty and draws the empty-cell fill. The geometry, the atlas
	// read seam and the subtract-one path are all live; only the data is
	// pending.
	ensureWildernessTilesLoaded();

	WildernessMap::AtlasGridReader grid;
	HostSiteQuestPredicate quest;
	const WildernessMap::Window window =
		WildernessMap::buildWindow(originAtlasX, originAtlasY, grid, quest);

	const int originPxX = WildernessMap::kCharCellsPerTile
		* WildernessMap::kCharCell; // 3 char cells -> 24 px
	const int originPxY = WildernessMap::kCharCellsPerTile
		* WildernessMap::kCharCell;
	const int step = WildernessMap::kCellPixels; // 24 px per cell

	for (int row = 0; row < WildernessMap::kWindowCells; ++row) {
		for (int col = 0; col < WildernessMap::kWindowCells; ++col) {
			const WildernessMap::WindowCell &cell = window.at(row, col);
			const int px = originPxX + col * step;
			const int py = originPxY + row * step;

			const ::Goldbox::Gfx::Pic *tile = (cell.tileSlot >= 0)
				? _wildernessTiles.getTile(static_cast<uint8>(cell.tileSlot))
				: nullptr;

			if (tile) {
				// tileSlot is ALREADY the subtract-one result; draw it as-is.
				tile->draw(&s, px, py);
			} else {
				// Baseline/empty cell (id 0, out of range, or tile missing).
				s.fillRect(Common::Rect(px, py, px + step, py + step), 0);
			}
		}
	}
}

void InGameMainScreenDialog::drawWildernessPositionMarker(Surface &s,
		int prospectiveOriginX, int prospectiveOriginY, int partyAtlasX,
		int partyAtlasY) {
	// GB13-024 (Req 39.4): the marker cell is computed INDEPENDENTLY of the
	// viewport-origin clamp (WildernessMap::markerCell clamps the origin but
	// not the party cell), so it drifts toward the window edge at map borders
	// instead of staying pinned to the centre. Render only; no movement/quest/
	// RNG mutation. The 6-frame pulse is PRESENTATION state.
	const WildernessMap::MarkerCell marker = WildernessMap::markerCell(
		prospectiveOriginX, prospectiveOriginY, partyAtlasX, partyAtlasY);

	// Advance the pulse frame regardless so the animation keeps cycling even
	// when the marker is momentarily off-window.
	const uint32 markerColor = (_wildernessAnimFrame < 3) ? 15 : 14;
	_wildernessAnimFrame = static_cast<uint8>((_wildernessAnimFrame + 1) % 6);

	if (!marker.visible)
		return;

	const int originPxX = WildernessMap::kCharCellsPerTile
		* WildernessMap::kCharCell;
	const int originPxY = WildernessMap::kCharCellsPerTile
		* WildernessMap::kCharCell;
	const int step = WildernessMap::kCellPixels;
	const int px = originPxX + marker.col * step;
	const int py = originPxY + marker.row * step;

	// Outline the marker cell (24x24) so it reads over the terrain tile.
	s.fillRect(Common::Rect(px, py, px + step, py + 2), markerColor);
	s.fillRect(Common::Rect(px, py + step - 2, px + step, py + step),
		markerColor);
	s.fillRect(Common::Rect(px, py, px + 2, py + step), markerColor);
	s.fillRect(Common::Rect(px + step - 2, py, px + step, py + step),
		markerColor);
}

void InGameMainScreenDialog::drawMap3dIfNeeded(Surface &s) {
	if (_mode != kModeDungeon && _mode != kModeWilderness
			&& _mode != kModeCombat && _mode != kModeCamping
			&& _mode != kModeShop && _mode != kModeAfterCombat)
		return;

	if (!::Goldbox::Poolrad::g_engine)
		return;

	// Camping mode: draw picture from PictureDisplayCache (PIC 29 campfire).
	if (_mode == kModeCamping) {
		const ::Goldbox::Gfx::PictureDisplayCache &picCache =
			::Goldbox::Poolrad::g_engine->getPictureDisplayCache();
		const ::Goldbox::Gfx::Pic *headPic = picCache.currentHeadFrame();
		if (headPic) {
			const int vpX = 3 * 8;
			const int vpY = 3 * 8;
			headPic->draw(&s, vpX, vpY);
			return;
		}
	}

	// Shop/AfterCombat: draw portrait from PictureDisplayCache
	// (loaded by ECL PICTURE opcode before ENCOUNTER, preserved by VM yield).
	if (_mode == kModeShop || _mode == kModeAfterCombat) {
		const ::Goldbox::Gfx::PictureDisplayCache &picCache =
			::Goldbox::Poolrad::g_engine->getPictureDisplayCache();
		if (picCache.isActive()) {
			const int vpX = 3 * 8;
			const int vpY = 3 * 8;
			if (picCache.headPic())
				picCache.headPic()->draw(&s, vpX, vpY);
			if (picCache.bodyPic())
				picCache.bodyPic()->draw(&s, vpX, vpY + 5 * 8);
		}
		return;
	}

	::Goldbox::RuntimeMapSnapshot snapshot;
	const RuntimeExchange *exchange =
		::Goldbox::Poolrad::g_engine->getRuntimeExchange();
	if (!exchange || !exchange->captureMapSnapshot(snapshot) || !snapshot.valid)
		return;

	uint16 mapX = snapshot.dungeonPos.x;
	uint16 mapY = snapshot.dungeonPos.y;
	uint8 mapDir = snapshot.dungeonDir;
	const uint8 effectiveType = effectiveMapTypeFromRuntime();

	if (effectiveType == 1) {
		// Check if area map mode is active.
		InGameView *igv = dynamic_cast<InGameView *>(
			::Goldbox::Poolrad::g_engine->findView("InGame"));
		if (igv && igv->isAreaMapMode()) {
			const Gfx::AreaMapCache &areaMap =
				::Goldbox::Poolrad::g_engine->getAreaMapCache();
			if (areaMap.isBuilt()) {
				areaMap.drawViewport(s, (int)mapX, (int)mapY, mapDir,
					::Goldbox::Poolrad::g_engine->getTileCache());
				return;
			}
		}

		Data::DaxBlockGeo *geo = ::Goldbox::Poolrad::g_engine->getActiveGeoBlock();
		if (!geo)
			return;

		// Blit the cached sky/floor background before drawing walls.
		const Gfx::ViewportBackground &vpBg =
			::Goldbox::Poolrad::g_engine->getViewportBackground();
		s.blitFrom(vpBg.surface(),
			Common::Rect(
				Gfx::ViewportBackground::kViewportX,
				Gfx::ViewportBackground::kViewportY,
				Gfx::ViewportBackground::kViewportX + Gfx::ViewportBackground::kViewportSize,
				Gfx::ViewportBackground::kViewportY + Gfx::ViewportBackground::kViewportSize),
			Common::Point(
				Gfx::ViewportBackground::kViewportX,
				Gfx::ViewportBackground::kViewportY));

		// Rendering priority:
		// 1. EncounterSpriteCache (SPRITE_START/ADVANCE) -> 3D + sprite overlay
		// 2. PictureDisplayCache (PICTURE opcode) -> portrait replaces 3D
		// 3. Neither -> normal 3D world
		const ::Goldbox::Gfx::EncounterSpriteCache &spriteCache =
			::Goldbox::Poolrad::g_engine->getEncounterSpriteCache();
		const ::Goldbox::Gfx::PictureDisplayCache &picCache =
			::Goldbox::Poolrad::g_engine->getPictureDisplayCache();

		if (!spriteCache.isSpriteLoaded() && picCache.isActive()) {
			// PICTURE opcode portrait/scene — replaces 3D view.
			const int vpX = ::Goldbox::Gfx::FirstPersonRenderer::k3dViewOffsetX * 8;
			const int vpY = ::Goldbox::Gfx::FirstPersonRenderer::k3dViewOffsetY * 8;
			if (picCache.headPic())
				picCache.headPic()->draw(&s, vpX, vpY);
			if (picCache.bodyPic())
				picCache.bodyPic()->draw(&s, vpX, vpY + 5 * 8);
			return;
		}

		// Renderer expects wire format: cardinal * 2 (0=N,2=E,4=S,6=W).
		const uint8 wireDir = static_cast<uint8>((mapDir & 0x03) * 2);
		Gfx::FirstPersonRenderer::draw3dWorld(&s, wireDir,
			(int)mapX, (int)mapY, *geo,
			::Goldbox::Poolrad::g_engine->getWalldefSlotCache());

		// Draw encounter sprite overlay if active.
		if (spriteCache.isSpriteLoaded() && spriteCache.spritePic()) {
			const ::Goldbox::Gfx::Pic *sprite = spriteCache.spritePic();
			const int vpX = ::Goldbox::Gfx::FirstPersonRenderer::k3dViewOffsetX * 8;
			const int vpY = ::Goldbox::Gfx::FirstPersonRenderer::k3dViewOffsetY * 8;
			// Sprite xPos/yPos from header are in char units; multiply by 8.
			::Goldbox::Data::DaxBlock *rawSp =
				::Goldbox::VmInterface::getDaxSprit().getBlockById(
					spriteCache.spriteBlockId());
			::Goldbox::Data::DaxBlockSprit *spBlock = rawSp
				? dynamic_cast< ::Goldbox::Data::DaxBlockSprit *>(rawSp) : nullptr;
			int sprX = vpX, sprY = vpY;
			if (spBlock) {
				const ::Goldbox::Data::DaxBlockSprit::FrameInfo *fi =
					spBlock->frameInfo(static_cast<int>(spriteCache.distance()));
				if (fi) {
					sprX += static_cast<int>(fi->xPos) * 8;
					sprY += static_cast<int>(fi->yPos) * 8;
				}
			}
			sprite->trDraw(&s, sprX, sprY, sprite->getTransparentIndex());
		}
		// Draw head/portrait on top when at distance 0 (adjacent).
		if (spriteCache.isSpriteLoaded() && spriteCache.distance() == 0) {
			const int vpX = ::Goldbox::Gfx::FirstPersonRenderer::k3dViewOffsetX * 8;
			const int vpY = ::Goldbox::Gfx::FirstPersonRenderer::k3dViewOffsetY * 8;
			const uint8 headPicId = spriteCache.lastHeadPicId();
			const uint8 bodyPicId = spriteCache.bodyPicId();

			if (headPicId == 0xFF) {
				// Scene picture from PIC DAX (no body/head composite).
				if (spriteCache.headPic())
					spriteCache.headPic()->draw(&s, vpX, vpY);
			} else {
				// Portrait composite: head at top, body 5 char rows below.
				if (spriteCache.headPic())
					spriteCache.headPic()->draw(&s, vpX, vpY);
				::Goldbox::Data::DaxBlock *bodyRaw =
					::Goldbox::VmInterface::getDaxBody().getBlockById(bodyPicId);
				::Goldbox::Data::DaxBlockPic *bodyBlock = bodyRaw
					? dynamic_cast< ::Goldbox::Data::DaxBlockPic *>(bodyRaw) : nullptr;
				if (bodyBlock) {
					Common::SharedPtr< ::Goldbox::Gfx::Pic> bodyPic(
						::Goldbox::Gfx::Pic::read(bodyBlock));
					if (bodyPic)
						bodyPic->draw(&s, vpX, vpY + 5 * 8);
				}
			}
		}
		return;
	}

	if (effectiveType > 1 && effectiveType < 5) {
		const int wildX = static_cast<int>(snapshot.wildernessPos.col);
		const int wildY = static_cast<int>(snapshot.wildernessPos.row);
		const int mapType = (int)effectiveType;
		const int xOffset = (mapType >= 0 && mapType < 8)
			? kWildernessMapXOffsets[mapType]
			: 0;

		// Party atlas cell = local wilderness pos + region x-offset (0/13/26).
		const int partyAtlasX = wildX + xOffset;
		const int partyAtlasY = wildY;

		// Prospective 5x5 window origin centres the party (col/row 2). The
		// origin is clamped to x 0..39 / y 0..31 inside buildWindow /
		// markerCell; the two are computed INDEPENDENTLY (GB13-024 Req 39.1 /
		// 39.4) so the clamp shifts the terrain window without pinning the
		// marker to the centre at map borders.
		const int prospectiveOriginX = partyAtlasX - 2;
		const int prospectiveOriginY = partyAtlasY - 2;

		drawWildernessAreaBlock(s, prospectiveOriginX, prospectiveOriginY);
		drawWildernessPositionMarker(s, prospectiveOriginX, prospectiveOriginY,
			partyAtlasX, partyAtlasY);
	}
}

void InGameMainScreenDialog::draw() {
	if (!_isVisible)
		return;

	if (_mode == kModeNone) {
		drawWindow(1, 1, 38, 22);
		return;
	}

	const bool showMiniWindow = (_mode != kModeWilderness);
	drawMainScreenWindows(showMiniWindow);

	Surface s = getSurface();
	drawMap3dIfNeeded(s);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
