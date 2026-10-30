/* ScummVM - Graphic Adventure Engine
 *
 * Direct, ordered transcription of the original FM Towns intro main_loop.
 * Text pointers supplied by the game view refer to the existing demo strings.
 */

#include "ultima/shared/views/intro_view.h"

namespace Ultima {
namespace Shared {
namespace Views {

namespace {

enum IntroOpcode : uint8 {
	kSlide,
	kFont,
	kDisplay,
	kPrepare,
	kPrint,
	kNewLine,
	kWait,
	kResetBox
};

struct IntroCommand {
	IntroOpcode op;
	uint16 value;
	uint8 offset;
};

static const IntroCommand kIntroScript[] = {
	{kSlide, 0, 0}, {kFont, 0, 0}, {kDisplay, 2, 0}, {kPrepare, 0, 0},
	{kPrint, 0, 0}, {kNewLine, 0, 0}, {kWait, 20, 0}, {kPrint, 1, 0},
	{kNewLine, 0, 0}, {kPrint, 2, 0}, {kNewLine, 0, 0}, {kWait, 50, 0},
	{kPrint, 3, 0}, {kNewLine, 0, 0}, {kWait, 70, 0}, {kPrint, 4, 0},
	{kWait, 15, 0}, {kPrint, 5, 0}, {kWait, 40, 0},

	{kSlide, 1, 0}, {kPrepare, 0, 0}, {kResetBox, 0, 0}, {kDisplay, 2, 0},
	{kPrint, 6, 0}, {kNewLine, 0, 0}, {kWait, 30, 0}, {kPrint, 7, 0},
	{kNewLine, 0, 0}, {kWait, 30, 0}, {kPrint, 8, 0}, {kWait, 30, 0},
	{kPrint, 9, 0}, {kNewLine, 0, 0}, {kWait, 30, 0}, {kPrint, 10, 0},
	{kNewLine, 0, 0}, {kWait, 40, 0}, {kPrepare, 0, 0}, {kResetBox, 0, 0},
	{kPrint, 11, 0}, {kWait, 8, 0}, {kPrint, 12, 0}, {kNewLine, 0, 0},
	{kNewLine, 0, 0}, {kWait, 30, 0}, {kPrint, 13, 0}, {kNewLine, 0, 0},
	{kWait, 25, 0},

	{kSlide, 2, 0}, {kDisplay, 2, 0}, {kPrepare, 0, 0}, {kResetBox, 0, 0},
	{kPrint, 14, 0}, {kNewLine, 0, 0}, {kWait, 40, 0}, {kPrint, 15, 0},
	{kNewLine, 0, 0}, {kWait, 40, 0}, {kPrint, 16, 0}, {kNewLine, 0, 0},
	{kWait, 40, 0}, {kPrint, 17, 1}, {kNewLine, 0, 0}, {kPrint, 18, 1},
	{kWait, 40, 0},

	{kSlide, 3, 0}, {kDisplay, 2, 0}, {kPrepare, 0, 0}, {kResetBox, 0, 0},
	{kPrint, 19, 1}, {kPrint, 20, 0}, {kNewLine, 0, 0}, {kWait, 200, 0}
};

const uint16 kCommandCount = sizeof(kIntroScript) / sizeof(kIntroScript[0]);

} // End of anonymous namespace

IntroView::IntroView(IntroPort &port, const IntroAssets &assets,
		uint32 millisPerDelayUnit) :
		_port(port), _assets(assets), _millisPerDelayUnit(millisPerDelayUnit),
		_waitStartMillis(0), _waitDurationMillis(0), _nextCommand(0),
		_state(kNotStarted), _waiting(false) {
}

void IntroView::start(uint32 nowMillis) {
	_nextCommand = 0;
	_waiting = false;
	_waitStartMillis = 0;
	_waitDurationMillis = 0;
	_state = _millisPerDelayUnit ? kRunning : kFailed;
	if (_state == kRunning)
		executeUntilWait(nowMillis);
}

void IntroView::update(uint32 nowMillis) {
	if (_state != kRunning)
		return;

	if (_waiting) {
		if (nowMillis - _waitStartMillis < _waitDurationMillis)
			return;
		_waiting = false;
	}

	executeUntilWait(nowMillis);
}

bool IntroView::handleEvent(const Common::Event &event) {
	if (_state != kRunning)
		return false;

	if (event.type == Common::EVENT_KEYDOWN) {
		if (_waiting) {
			_waiting = false;
			_state = kInterrupted;
		}
		return true;
	}
	return false;
}

void IntroView::stop() {
	if (_state == kRunning) {
		_waiting = false;
		_state = kInterrupted;
	}
}

void IntroView::executeUntilWait(uint32 nowMillis) {
	bool pixelsChanged = false;

	while (_nextCommand < kCommandCount && _state == kRunning) {
		const IntroCommand &command = kIntroScript[_nextCommand++];

		switch (command.op) {
		case kSlide:
			if (!_port.drawSlide(static_cast<uint8>(command.value))) {
				_state = kFailed;
				break;
			}
			pixelsChanged = true;
			break;
		case kFont:
			_port.applyTextBoxFont(static_cast<uint8>(command.value));
			break;
		case kDisplay:
			_port.setDisplayPage(command.value);
			pixelsChanged = true;
			break;
		case kPrepare:
			_port.prepareTextBox(static_cast<uint8>(command.value));
			pixelsChanged = true;
			break;
		case kPrint: {
			if (command.value >= IntroAssets::kTextCount ||
					!_assets.text[command.value]) {
				_state = kFailed;
				break;
			}
			const char *bytes = _assets.text[command.value];
			if (command.offset && !bytes[0]) {
				_state = kFailed;
				break;
			}
			_port.printTextBoxString(0, bytes + command.offset);
			pixelsChanged = true;
			break;
		}
		case kNewLine:
			_port.advanceTextBoxLine(static_cast<uint8>(command.value));
			pixelsChanged = true;
			break;
		case kResetBox:
			_port.resetBoxTo(static_cast<uint8>(command.value), 0, 0);
			break;
		case kWait:
			if (pixelsChanged) {
				_port.present();
				pixelsChanged = false;
			}
			_port.clearPendingKeyboard();
			if (command.value > 0xffffffffu / _millisPerDelayUnit) {
				_state = kFailed;
				break;
			}
			_waitDurationMillis = command.value * _millisPerDelayUnit;
			_waitStartMillis = nowMillis;
			_waiting = true;
			return;
		}
	}

	if (pixelsChanged)
		_port.present();
	if (_nextCommand >= kCommandCount && _state == kRunning)
		_state = kCompleted;
}

} // End of namespace Views
} // End of namespace Shared
} // End of namespace Ultima
