/* ScummVM - Graphic Adventure Engine
 *
 * FM Towns intro playback: ordered presentation commands, without BIOS calls.
 * The owner creates/preloads the four slide resources and initializes box 0.
 */

#ifndef ULTIMA_SHARED_VIEWS_INTRO_VIEW_H
#define ULTIMA_SHARED_VIEWS_INTRO_VIEW_H

#include "common/scummsys.h"
#include "common/events.h"

namespace Ultima {
namespace Shared {
namespace Views {

class IntroPort {
public:
	virtual ~IntroPort() {}

	virtual bool drawSlide(uint8 slideIndex) = 0;
	virtual void applyTextBoxFont(uint8 boxIndex) = 0;
	virtual void setDisplayPage(int32 legacyMode) = 0;
	virtual void prepareTextBox(uint8 boxIndex) = 0;
	virtual void printTextBoxString(uint8 boxIndex, const char *bytes) = 0;
	virtual void advanceTextBoxLine(uint8 boxIndex) = 0;
	virtual void resetBoxTo(uint8 boxIndex, int32 arg1, int32 arg2) = 0;
	virtual void present() = 0;
	virtual void clearPendingKeyboard() = 0;
};

struct IntroAssets {
	enum { kTextCount = 21 };
	const char *text[kTextCount];
};

class IntroView {
public:
	enum State {
		kNotStarted,
		kRunning,
		kCompleted,
		kInterrupted,
		kFailed
	};

	IntroView(IntroPort &port, const IntroAssets &assets, uint32 millisPerDelayUnit);

	void start(uint32 nowMillis);
	void update(uint32 nowMillis);
	bool handleEvent(const Common::Event &event);
	void stop();

	State state() const { return _state; }
	bool isWaiting() const { return _waiting; }
	uint16 commandIndex() const { return _nextCommand; }

private:
	void executeUntilWait(uint32 nowMillis);

	IntroPort &_port;
	IntroAssets _assets;
	uint32 _millisPerDelayUnit;
	uint32 _waitStartMillis;
	uint32 _waitDurationMillis;
	uint16 _nextCommand;
	State _state;
	bool _waiting;
};

} // End of namespace Views
} // End of namespace Shared
} // End of namespace Ultima

#endif
