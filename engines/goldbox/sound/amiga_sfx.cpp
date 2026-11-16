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

#include "goldbox/sound/amiga_sfx.h"
#include "common/endian.h"
#include "common/file.h"
#include "common/textconsole.h"

namespace Goldbox {

namespace {

// Pitch limits of the sweeps (PAULA_Play).
const uint16 kSweepUpLimit = 0x71;
const uint16 kSweepDownLimit = 0xD60;

// Effect modes.
const uint8 kModeSweepUp = 1;
const uint8 kModeSweepDown = 2;
const uint8 kModeTrill = 4;

// The effects play on channel 0, panned to the middle.
const int kEffectChannel = 0;

const int8 kSilence[2] = { 0, 0 };

} // namespace

AmigaSfx::AmigaSfx() : Paula(true, 44100, 44100 / 50) {
	for (int ch = 0; ch < 4; ++ch)
		setChannelPanning(ch, 128);
}

bool AmigaSfx::load(const Common::Path &datPath) {
	Common::File file;
	if (!file.open(datPath)) {
		warning("AmigaSfx: %s not found", datPath.toString().c_str());
		return false;
	}
	// Skip the music section (see AmigaMusic::load).
	const uint32 magic = file.readUint32BE();
	const uint16 version = file.readUint16LE();
	file.readUint16LE();
	file.readUint32LE();
	const uint32 musicSize = file.readUint32LE();
	if (magic != MKTAG('G', 'B', 'A', 'M') || version < 2) {
		warning("AmigaSfx: %s has no sound effects", datPath.toString().c_str());
		return false;
	}
	file.skip(musicSize);

	const uint16 periodCount = file.readUint16LE();
	const uint16 effectCount = file.readUint16LE();
	const uint16 sampleCount = file.readUint16LE();
	_periods.resize(periodCount);
	for (uint i = 0; i < periodCount; ++i)
		_periods[i] = file.readUint16LE();
	_effects.resize(effectCount);
	for (uint i = 0; i < effectCount; ++i) {
		Effect &e = _effects[i];
		e.sample = file.readByte();
		e.pitch = file.readByte();
		e.param = file.readByte();
		e.volume = file.readByte();
		e.mode = file.readByte();
		e.duration = file.readByte();
		e.decay = file.readByte();
		e.repeats = file.readByte();
	}
	_samples.resize(sampleCount);
	for (uint i = 0; i < sampleCount; ++i) {
		Sample &s = _samples[i];
		s.offset = file.readUint32LE();
		s.length = file.readUint16LE();
		s.repeatOffset = file.readUint16LE();
		s.repeatLength = file.readUint16LE();
		s.loop = file.readByte() != 0;
		file.readByte();
	}
	const uint32 dataSize = file.readUint32LE();
	_sampleData.resize(dataSize);
	if (file.read(_sampleData.data(), dataSize) != dataSize || file.err()) {
		warning("AmigaSfx: %s is truncated", datPath.toString().c_str());
		return false;
	}
	for (uint i = 0; i < sampleCount; ++i) {
		if (_samples[i].offset + _samples[i].length > dataSize) {
			warning("AmigaSfx: sample %u is outside the sample data", i);
			return false;
		}
	}
	_loaded = true;
	startPaula();
	return true;
}

void AmigaSfx::playEffect(int effect) {
	Common::StackLock lock(_mutex);
	if (_loaded && effect >= 0 && effect < (int)_effects.size())
		_voices[kEffectChannel].request = effect;
}

void AmigaSfx::stopEffects() {
	Common::StackLock lock(_mutex);
	for (int ch = 0; ch < 4; ++ch) {
		_voices[ch].request = -1;
		_voices[ch].active = false;
		_voices[ch].volume = 0;
		disableChannel(ch);
		_dmaOn[ch] = false;
	}
	_dmaOff = 0x0F;
}

uint16 AmigaSfx::period(uint8 index) const {
	return index < _periods.size() ? _periods[index] : 428;
}

void AmigaSfx::interrupt() {
	// PAULA_Setup: start a requested effect or run the active one, then
	// write the hardware.
	if (!_loaded)
		return;
	for (int ch = 3; ch >= 0; --ch) {
		Voice &v = _voices[ch];
		if (v.request >= 0)
			startVoice(ch, v.request);
		else if (v.active)
			runVoice(ch);
	}
	writeHardware();
}

void AmigaSfx::startVoice(int ch, int effect) {
	// PAULA_Init: the effect's sample at its pitch. All channels stay off
	// this frame; the next PAULA_Play turns this one on.
	Voice &v = _voices[ch];
	const Effect &e = _effects[effect];
	v.request = -1;
	v.sample = e.sample;
	v.param = e.param;
	v.volume = (int8)e.volume;
	v.mode = e.mode;
	v.duration = e.duration;
	v.durationReload = e.duration;
	v.decay = e.decay;
	v.repeats = (int8)e.repeats;
	v.forever = e.repeats == 0xFF;
	v.toggle = false;
	v.loopSet = false;
	v.period = period(e.pitch);

	if (e.sample < _samples.size()) {
		const Sample &s = _samples[e.sample];
		setChannelSampleStart(ch, &_sampleData[s.offset]);
		setChannelSampleLen(ch, MAX<uint16>(s.length / 2, 1));
	} else {
		setChannelSampleStart(ch, kSilence);
		setChannelSampleLen(ch, 1);
	}
	_dmaOff = 0x0F | (1 << ch);
	disableChannel(ch);
	_dmaOn[ch] = false;
	v.active = true;
}

void AmigaSfx::runVoice(int ch) {
	// PAULA_Play
	Voice &v = _voices[ch];
	_dmaOff &= ~(1 << ch);

	// After the first buffer a looping sample repeats its loop part (the
	// original sets it every 20 frames); the others repeat whole.
	if (!v.loopSet && v.sample < _samples.size() && _samples[v.sample].loop) {
		const Sample &s = _samples[v.sample];
		if (s.repeatOffset + s.repeatLength <= s.length) {
			setChannelSampleStart(ch, &_sampleData[s.offset + s.repeatOffset]);
			setChannelSampleLen(ch, MAX<uint16>(s.repeatLength / 2, 1));
		}
		v.loopSet = true;
	}

	if (!v.forever && v.duration == 0) {
		// Duration over: play it again while repeats last, then decay.
		if (v.repeats >= 0) {
			--v.repeats;
			v.duration = v.durationReload;
			return;
		}
		if (v.decay) {
			v.volume -= v.decay;
			if (v.volume >= 0)
				return;
		}
		_dmaOff |= 1 << ch;
		v.volume = 0;
		v.active = false;
		return;
	}
	if (!v.forever)
		--v.duration;

	if (v.mode & kModeTrill) {
		// Alternates the period with the second note.
		if (v.toggle)
			v.toggle = false;
		else {
			v.toggle = true;
			v.period = period(v.param);
		}
	} else if (v.mode & kModeSweepDown) {
		if (v.period != kSweepDownLimit)
			v.period = (v.period + v.param) & 0xFFF;
	} else if (v.mode & kModeSweepUp) {
		if (v.period != kSweepUpLimit)
			v.period = (v.period - v.param) & 0xFFF;
	}
}

void AmigaSfx::writeHardware() {
	// PAULA_Set: periods, DMA and volumes.
	for (int ch = 0; ch < 4; ++ch) {
		const Voice &v = _voices[ch];
		setChannelPeriod(ch, v.period);
		if (_dmaOff & (1 << ch)) {
			disableChannel(ch);
			_dmaOn[ch] = false;
		} else if (!_dmaOn[ch]) {
			enableChannel(ch);
			_dmaOn[ch] = true;
		}
		setChannelVolume(ch, (uint8)CLIP<int>(v.volume, 0, 64));
	}
}

} // namespace Goldbox
