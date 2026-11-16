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

#ifndef GOLDBOX_POOLRAD_VIEWS_AMIGA_AMIGA_INTRO_VIEW_H
#define GOLDBOX_POOLRAD_VIEWS_AMIGA_AMIGA_INTRO_VIEW_H

#include "goldbox/poolrad/views/view.h"
#include "goldbox/gfx/backend/amiga_intro.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

/**
 * The Amiga intro program, replayed: each step shows a band of the credits
 * sheet (or the painting), fades it in, holds it and fades it out. A key
 * or mouse button ends the intro, as the left mouse button did.
 */
class AmigaIntroView : public View {
public:
	AmigaIntroView();
	~AmigaIntroView() override;

	bool msgFocus(const FocusMessage &msg) override;
	bool msgUnfocus(const UnfocusMessage &msg) override;
	bool msgKeypress(const KeypressMessage &msg) override;
	bool msgMouseDown(const MouseDownMessage &msg) override;
	bool tick() override;
	void draw() override;

	/** Load the intro program; false when this install has none. */
	bool loadIntro();

private:
	enum Phase { kWait, kFadeIn, kHold, kFadeOut, kDone };

	void startPhase(Phase phase);
	void applyFade(int divisor);
	void finish();
	/** PlayMusic(1) before the intro program, PlayMusic(0) after it. */
	void startMusic();
	void stopMusic();

	Gfx::AmigaIntro _intro;
	bool _loaded = false;
	bool _tried = false;
	int _step = 0;
	Phase _phase = kWait;
	uint32 _phaseStart = 0;
	int _fadeLevel = -1;
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
