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

#include "goldbox/poolrad/views/amiga/amiga_intro_view.h"
#include "common/system.h"
#include "graphics/palette.h"
#include "graphics/paletteman.h"
#include "goldbox/engine.h"
#include "goldbox/gfx/backend/gfx_backend.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

namespace {

// The Amiga intro runs at the PAL frame rate.
const uint32 kFrameMs = 20;

// The original waits count passes of a loop that polls VHPOSR for line 128
// (FUN_00204830), not frames: a pass is about 120 cycles (17 us on a
// 7.09 MHz 68000) and a line 64 us, so 3-4 passes fall on each frame. The
// waits before each step (81 passes), the hold (1001 passes, or the left
// mouse button) and each of the 16 fade levels (9 shorter passes) in frames:
const uint32 kWaitFrames = 22;
const uint32 kHoldFrames = 270;
const uint32 kFadeFrames = 2;
const int kFadeLevels = 16;

/** One step of the intro script (FUN_00204000). */
struct IntroStep {
	bool painting;  ///< The painting instead of a band of the sheet
	int srcRow;     ///< First row of the band in the sheet
	int dstRow;     ///< Screen row it is copied to
	int rows;
	int palette;
	bool fadeOut;
};

const IntroStep kSteps[] = {
	{ false, 93,  30, 48,  0, true },   // A Forgotten Realms...
	{ false, 455, 60, 16,  0, true },   // UBI Soft
	{ false, 398, 30, 56,  0, true },   // UBI Soft Entertainment Software
	{ false, 474, 60, 19,  0, true },   // Amiga version
	{ false, 141, 20, 146, 0, true },   // SSI, AD&D, copyright
	{ false, 0,   20, 92,  1, true },   // Pool of Radiance logo
	{ false, 288, 20, 110, 0, true },   // credits
	{ false, 499, 20, 100, 0, true },   // Amiga credits
	{ true,  0,   0,  200, 0, false }   // the painting
};

const int kStepCount = ARRAYSIZE(kSteps);

} // namespace

AmigaIntroView::AmigaIntroView() : View("AmigaIntro") {
}

AmigaIntroView::~AmigaIntroView() {
}

void AmigaIntroView::startMusic() {
	g_engine->playMusic(1);
}

void AmigaIntroView::stopMusic() {
	g_engine->playMusic(0);
}

bool AmigaIntroView::loadIntro() {
	if (!_loaded && !_tried) {
		_tried = true;
		_loaded = _intro.load(Common::Path("intro"));
		if (!_loaded)
			warning("AmigaIntroView: the intro program could not be loaded");
	}
	return _loaded;
}

bool AmigaIntroView::msgFocus(const FocusMessage &msg) {
	_phase = kWait;
	_fadeLevel = -1;
	if (!loadIntro()) {
		finish();
		return true;
	}
	_step = 0;
	applyFade(kFadeLevels);
	startPhase(kWait);
	startMusic();
	return true;
}

bool AmigaIntroView::msgUnfocus(const UnfocusMessage &msg) {
	stopMusic();
	// Back to the game palette.
	g_engine->getGfx().setupPalette();
	return true;
}

bool AmigaIntroView::msgKeypress(const KeypressMessage &msg) {
	finish();
	return true;
}

bool AmigaIntroView::msgMouseDown(const MouseDownMessage &msg) {
	finish();
	return true;
}

void AmigaIntroView::startPhase(Phase phase) {
	_phase = phase;
	_phaseStart = g_system->getMillis();
	redraw();
}

void AmigaIntroView::applyFade(int divisor) {
	// Each 4-bit component divided by the fade level: 16 is black, 1 full.
	if (divisor == _fadeLevel)
		return;
	_fadeLevel = divisor;
	const uint16 *colors = _intro.palette(kSteps[MIN(_step, kStepCount - 1)].palette);
	Graphics::Palette pal(Gfx::AmigaIntro::kColors);
	for (int i = 0; i < Gfx::AmigaIntro::kColors; ++i) {
		const uint16 c = colors[i];
		pal.set(i, (((c >> 8) & 0xF) / divisor) * 17,
			(((c >> 4) & 0xF) / divisor) * 17, ((c & 0xF) / divisor) * 17);
	}
	g_system->getPaletteManager()->setPalette(pal, 0);
}

bool AmigaIntroView::tick() {
	if (_phase == kDone)
		return View::tick();

	const uint32 frames = (g_system->getMillis() - _phaseStart) / kFrameMs;
	switch (_phase) {
	case kWait:
		if (frames >= kWaitFrames)
			startPhase(kFadeIn);
		break;
	case kFadeIn: {
		const int level = (int)(frames / kFadeFrames);
		if (level >= kFadeLevels) {
			applyFade(1);
			startPhase(kHold);
		} else {
			applyFade(kFadeLevels - level);
		}
		break;
	}
	case kHold:
		if (frames >= kHoldFrames) {
			if (kSteps[_step].fadeOut)
				startPhase(kFadeOut);
			else
				finish();
		}
		break;
	case kFadeOut: {
		const int level = (int)(frames / kFadeFrames);
		if (level >= kFadeLevels) {
			applyFade(kFadeLevels);
			if (++_step >= kStepCount)
				finish();
			else
				startPhase(kWait);
		} else {
			applyFade(level + 1);
		}
		break;
	}
	default:
		break;
	}
	return View::tick();
}

void AmigaIntroView::draw() {
	Surface s = getSurface();
	s.clear(0);
	if (!_loaded || _phase == kWait || _phase == kDone || _step >= kStepCount)
		return;

	// The band is blitted while the palette is black, then faded in.
	const IntroStep &step = kSteps[_step];
	const Graphics::ManagedSurface &src = step.painting ? _intro.painting()
		: _intro.sheet();
	s.blitFrom(src, Common::Rect(0, step.srcRow, Gfx::AmigaIntro::kWidth,
		step.srcRow + step.rows), Common::Point(0, step.dstRow));
}

void AmigaIntroView::finish() {
	if (_phase == kDone)
		return;
	_phase = kDone;
	Surface s = getSurface();
	s.clear(0);
	replaceView("Mainmenu");
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
