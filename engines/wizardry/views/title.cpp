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

#include "wizardry/views/title.h"
#include "wizardry/resources/title_image.h"
#include "wizardry/ui/town_screens.h"
#include "wizardry/wizardry.h"

namespace Wizardry {

enum {
	kMsgStart = 2021,    // "S)TART GAME": its first letter is the key
	kMsgMakeDisk = 2022,

	// The creatures shown are the pictures from 4 to 29: the first three
	// are the treasure, the chest and the grave
	kFirstCreature = 4,
	kCreatureCount = 26,

	// The original waits 30 times for 4 ticks of the PC's timer, which
	// runs at 18.2 a second: about 6.6 seconds
	kShowFrames = FRAME_RATE * 30 * 4 * 10 / 182,

	// Each row of the slide is one repaint, as fast as the PC could draw
	// it. One frame is a guess at that.
	kSlideFrames = 1,

	// Rows of the screen with the two choices
	kStartRow = 22,
	kMakeDiskRow = 23
};

bool Title::isOriginal() const {
	const Profile *profile = g_engine->getProfile();
	return profile && profile->platform == Common::kPlatformDOS && profile->game <= kWizardry3;
}

bool Title::msgFocus(const FocusMessage &msg) {
	_logo = TileImage();
	loadTitleImage(*g_engine->getProfile(), g_engine->getGfxSet(), _logo);
	_key = 0;

	if (isOriginal()) {
		newPictures();
	} else {
		_state = kStatic;
		cancelDelay();
	}

	return TileView::msgFocus(msg);
}

void Title::newPictures() {
	// One to four creatures, each picked at random; the same one may come twice
	const int count = g_engine->getRandomNumber(3) + 1;

	_pictures.clear();
	for (int i = 0; i < count; ++i) {
		TileImage image;
		// The two-colour set of Wizardry II and III has no pictures: then
		// there is nothing to show, but the cycle runs all the same
		_G(pictures).load(kFirstCreature + g_engine->getRandomNumber(kCreatureCount - 1), image);
		_pictures.push_back(image);
	}

	_slide = 0;
	_state = kShowing;
	delayFrames(kShowFrames);
	redraw();
}

void Title::startSlide() {
	_state = kSliding;
	_slide = 1;
	delayFrames(kSlideFrames);
	redraw();
}

void Title::timeout() {
	switch (_state) {
	case kShowing:
		startSlide();
		break;

	case kSliding:
		if (_slide < TownScreens::kTitlePictureRows) {
			++_slide;
			delayFrames(kSlideFrames);
			redraw();
		} else {
			const uint16 key = _key;
			_key = 0;
			keyAfterSlide(key);
		}
		break;

	default:
		TileView::timeout();
		break;
	}
}

void Title::keyAfterSlide(uint16 key) {
	const MessageFile &messages = _G(messages);

	if (key && key == (byte)messages.get(kMsgStart)[0]) {
		leave();
		return;
	}

	// M)AKE SCENARIO DISK made a copy of the scenario disk to play on.
	// There is nothing to make here, so the key opens the gallery of the
	// game's pictures, character set and messages instead: a feature for
	// debugging, not mentioned on screen. Escape there comes back here.
	if (key && key == (byte)messages.get(kMsgMakeDisk)[0]) {
		addView("Gallery");
		return;
	}

	newPictures();
}

void Title::drawTiles(TextWindow &window) {
	if (!isOriginal()) {
		_G(tiles).clear();

		// The Japanese titles are 40 columns wide. Their row is assumed;
		// it has not been read from those drivers.
		if (!_logo.empty())
			window.showImage(_logo, (window.width() - _logo.cols) / 2, 2);
		return;
	}

	TownScreens screens(_G(tiles), _G(messages), _G(party), _G(items));
	screens.drawTitle(_logo.empty() ? nullptr : &_logo, _pictures, _state == kSliding ? _slide : 0);
}

void Title::leave() {
	cancelDelay();
	if (g_engine->hasTown())
		replaceView("Town.Castle");
	else
		replaceView("Gallery");
}

bool Title::msgKeypress(const KeypressMessage &msg) {
	if (_state == kStatic) {
		leave();
		return true;
	}

	uint16 key = msg.ascii;
	if (key >= 'a' && key <= 'z')
		key -= 0x20;
	if (!key)
		return true;

	// The original reads the key once the creatures are gone: a key cuts
	// the wait short, and one pressed during the slide waits for its end
	if (!_key)
		_key = key;
	if (_state == kShowing) {
		cancelDelay();
		startSlide();
	}

	return true;
}

bool Title::msgMouseDown(const MouseDownMessage &msg) {
	if (_state == kStatic) {
		leave();
		return true;
	}

	// A click on one of the two choices is its key
	const int row = msg._pos.y / _G(renderer).tileHeight();
	const MessageFile &messages = _G(messages);
	if (row == kStartRow || row == kMakeDiskRow) {
		KeypressMessage key;
		key.ascii = (byte)messages.get(row == kStartRow ? kMsgStart : kMsgMakeDisk)[0];
		msgKeypress(key);
	}

	return true;
}

} // End of namespace Wizardry
