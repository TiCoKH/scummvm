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

#include "goldbox/sound/amiga_music.h"
#include "common/endian.h"
#include "common/file.h"
#include "common/textconsole.h"

namespace Goldbox {

namespace {

// Hunk 28 of the program, loaded at 0x2611a8 in the Ghidra project; all
// offsets below are addresses minus that base.
const uint32 kMusicHunkSize = 0x132A8;
const uint32 kBase = 0x2611A8;

#define OFS(a) ((uint32)(a) - kBase)

const uint32 kPeriods = OFS(0x262890);    // note periods (words)
const uint32 kOutPer = OFS(0x2628F0);     // output period per channel
const uint32 kChan = OFS(0x2628F8);       // byte fields: kChan + field + ch
const uint32 kVolumes = kChan + 0x60;
const uint32 kLoopFlag = kChan + 0x78;
const uint32 kPlaying = OFS(0x26297D);
const uint32 kSpeed = OFS(0x26297E);
const uint32 kSpeedCnt = OFS(0x26297F);
const uint32 kArpBits = OFS(0x262982);
const uint32 kPatOfs = OFS(0x262983);
const uint32 kSlideUp = OFS(0x262986);
const uint32 kDmaOff = OFS(0x262988);     // channels whose DMA stays off
const uint32 kBasePer = OFS(0x26298A);    // words
const uint32 kSlideTgt = OFS(0x262992);   // words
const uint32 kSlideStep = OFS(0x26299A);  // word
const uint32 kSmpStart = OFS(0x2629A4);   // longs
const uint32 kReplen = OFS(0x2629B4);     // words
const uint32 kRepOfs = OFS(0x2629BC);     // words
const uint32 kPatPtr = OFS(0x2629C4);     // longs
const uint32 kTracks = OFS(0x2629D4);     // longs, current song
const uint32 kPatTmp = OFS(0x2629E8);
const uint32 kArpBuf = OFS(0x2629FC);
const uint32 kSamples = OFS(0x262A24);    // longs
const uint32 kInstruments = OFS(0x262A6C);
const uint32 kArpSteps = OFS(0x262ACC);
const uint32 kEnvelopes = OFS(0x262AD4);
const uint32 kSongs = OFS(0x262B24);
const uint32 kPatterns = OFS(0x262B84);

#undef OFS

// A one-shot sample goes on with an empty buffer.
const int8 kSilence[2] = { 0, 0 };

} // namespace

AmigaMusic::AmigaMusic()
	: Paula(true, 44100, 44100 / 50) {
	// Paula: channels 0 and 3 left, 1 and 2 right.
	setChannelPanning(0, 64);
	setChannelPanning(1, 191);
	setChannelPanning(2, 191);
	setChannelPanning(3, 64);
}

bool AmigaMusic::load(const Common::Path &datPath) {
	// poolradamigasound.dat (devtools/create_goldbox/poolrad/
	// create_amiga_sound.py): 'GBAM', version, song count, load address and
	// size, then hunk 28 of the program with its pointers as hunk offsets.
	Common::File file;
	if (!file.open(datPath)) {
		warning("AmigaMusic: %s not found", datPath.toString().c_str());
		return false;
	}
	const uint32 magic = file.readUint32BE();
	const uint16 version = file.readUint16LE();
	const uint16 songs = file.readUint16LE();
	const uint32 address = file.readUint32LE();
	const uint32 size = file.readUint32LE();
	if (magic != MKTAG('G', 'B', 'A', 'M') || version < 1 || version > 2 || address != kBase
			|| size != kMusicHunkSize || songs == 0) {
		warning("AmigaMusic: %s is not a known music file", datPath.toString().c_str());
		return false;
	}
	_mem.resize(size);
	if (file.read(_mem.data(), size) != size) {
		warning("AmigaMusic: %s is truncated", datPath.toString().c_str());
		return false;
	}
	_songCount = songs;
	_loaded = true;
	// The stream runs from now on; without a song it is silent.
	startPaula();
	return true;
}

void AmigaMusic::playSong(int song) {
	Common::StackLock lock(_mutex);
	if (!_loaded)
		return;
	_request = (song >= 1 && song <= _songCount) ? song : 0;
}

// --- memory

uint8 AmigaMusic::b(uint32 a) const {
	return a < _mem.size() ? _mem[a] : 0;
}

void AmigaMusic::sb(uint32 a, uint8 v) {
	if (a < _mem.size())
		_mem[a] = v;
}

uint16 AmigaMusic::w(uint32 a) const {
	return a + 1 < _mem.size() ? READ_BE_UINT16(&_mem[a]) : 0;
}

void AmigaMusic::sw(uint32 a, uint16 v) {
	if (a + 1 < _mem.size())
		WRITE_BE_UINT16(&_mem[a], v);
}

uint32 AmigaMusic::l(uint32 a) const {
	return a + 3 < _mem.size() ? READ_BE_UINT32(&_mem[a]) : 0;
}

void AmigaMusic::sl(uint32 a, uint32 v) {
	if (a + 3 < _mem.size())
		WRITE_BE_UINT32(&_mem[a], v);
}

uint8 AmigaMusic::f(uint field, int ch) const {
	return b(kChan + field + ch);
}

void AmigaMusic::sf(uint field, int ch, uint8 v) {
	sb(kChan + field + ch, v);
}

// --- player

void AmigaMusic::interrupt() {
	if (_loaded)
		tick();
}

void AmigaMusic::tick() {
	// SYSexec (0x26074a): a request of 0 stops, 1-7 starts a song, 0xFF
	// plays on.
	const int request = _request;
	_request = 0xFF;
	if (request == 0) {
		sb(kPlaying, 0);
		stopVoices();
		for (int ch = 0; ch < 4; ++ch) {
			disableChannel(ch);
			_dmaOn[ch] = false;
		}
	} else if (request <= 7) {
		startSong(request);
	} else if (b(kPlaying)) {
		playTick();
	}
}

void AmigaMusic::startSong(int song) {
	const uint32 entry = kSongs + (song - 1) * 16;
	for (int i = 0; i < 4; ++i)
		sl(kTracks + i * 4, l(entry + i * 4));
	for (int ch = 3; ch >= 0; --ch) {
		sf(0x14, ch, 0);
		sf(0x18, ch, 0);
		sf(0x0C, ch, 0);
		nextPattern(ch);
	}
	stopVoices();
	sb(kSpeedCnt, 0);
	sb(kPlaying, 1);
	sb(kDmaOff, 0x0F);
}

void AmigaMusic::stopVoices() {
	for (int ch = 0; ch < 4; ++ch) {
		sb(kVolumes + ch, 0);
		setChannelVolume(ch, 0);
	}
}

void AmigaMusic::nextPattern(int ch) {
	// InitSetup (0x2607c4): the next pattern of the channel's track; other
	// bytes above 0xC0 set the transpose, 0xFF loops the track.
	const uint32 track = l(kTracks + ch * 4);
	for (int guard = 0; guard < 256; ++guard) {
		const uint8 pos = f(0x0C, ch);
		sf(0x0C, ch, pos + 1);
		const uint8 v = b(track + pos);
		if (v <= 0xC0) {
			const uint32 ptr = l(kPatterns + v * 4);
			sl(kPatPtr + ch * 4, ptr);
			sl(kPatTmp, ptr);
			return;
		}
		if (v == 0xFF)
			sf(0x0C, ch, 0);
		else
			sf(0x20, ch, v + 0x20);
	}
}

bool AmigaMusic::playTick() {
	for (int ch = 3; ch >= 0; --ch) {
		if (!channel(ch))
			return false;      // the song was stopped or changed
	}
	writeHardware();

	// The loop part plays after the note's first buffer.
	for (int ch = 0; ch < 4; ++ch) {
		const uint16 replen = w(kReplen + ch * 2);
		if (b(kLoopFlag + ch) == 0) {
			setChannelSampleStart(ch, kSilence);
			setChannelSampleLen(ch, 1);
		} else {
			setLocation(ch, l(kSmpStart + ch * 4) + w(kRepOfs + ch * 2), replen >> 1);
		}
	}

	uint8 cnt = b(kSpeedCnt) - 1;
	if (cnt & 0x80)
		cnt = b(kSpeed);
	sb(kSpeedCnt, cnt);
	return true;
}

void AmigaMusic::notePeriod(int ch) {
	// SYS9: the note's period as the base and output period.
	const uint16 p = w(kPeriods + ((f(0x08, ch) * 2) & 0xFF));
	sw(kBasePer + ch * 2, p);
	sw(kOutPer + ch * 2, p);
}

bool AmigaMusic::channel(int ch) {
	if (b(kSpeedCnt) != 0) {
		if (f(0x34, ch)) {
			sf(0x34, ch, 0);
			sf(0x08, ch, f(0x08, ch) - 2);
			notePeriod(ch);
		} else {
			effects(ch);
		}
		envelope(ch);
		return true;
	}

	sf(0x14, ch, f(0x14, ch) - 1);
	const uint8 dur = f(0x14, ch);
	if (f(0x70, ch) == 0 && dur == 0) {
		sb(kDmaOff, b(kDmaOff) | (1 << ch));
	} else if (dur & 0x80) {
		return newNote(ch);
	}
	if (f(0x2C, ch)) {
		sf(0x08, ch, f(0x08, ch) - 1);
		notePeriod(ch);
	} else if (f(0x30, ch)) {
		sf(0x08, ch, f(0x08, ch) + 1);
		notePeriod(ch);
	} else {
		effects(ch);
	}
	envelope(ch);
	return true;
}

bool AmigaMusic::newNote(int ch) {
	if (f(0x18, ch) != 2) {
		sf(0x18, ch, 0);
		sf(0x1C, ch, 0);
	}
	sb(kPatOfs, 0);
	sf(0x24, ch, 0);
	sf(0x2C, ch, 0);
	sf(0x30, ch, 0);
	sf(0x6C, ch, 0);
	sl(kPatTmp, l(kPatPtr + ch * 4));

	uint8 d0 = 0;
	uint8 d4 = 0;
	uint32 a3 = 0;
	auto fetch = [&]() {
		d0 = b(a3++);
		++d4;
	};

	for (int guard = 0; ; ++guard) {
		if (guard > 256)
			return true;
		d4 = b(kPatOfs);
		sb(kPatOfs, d4 + 1);
		a3 = l(kPatTmp) + d4;
		d0 = b(a3++);
		++d4;
		if (d0 == 0xFF) {
			nextPattern(ch);
			sb(kPatOfs, 0);
			continue;
		}
		if (d0 == 0xFE) {
			_request = 0;
			tick();
			return false;
		}
		if (d0 == 0xFC) {
			_request = b(a3);
			tick();
			return false;
		}
		if (d0 > 0xEF) {
			sb(kSpeed, d0 & 0x0F);
			fetch();
		}
		if (d0 > 0xDF) {
			sf(0x4C, ch, (d0 & 0x0F) * 8);
			fetch();
			if (d0 == 0xFF) {
				nextPattern(ch);
				sb(kPatOfs, 0);
				continue;
			}
		}
		if (d0 > 0xBF) {
			sf(0x50, ch, (d0 & 0x1F) * 8);
			fetch();
			if (d0 == 0xFF) {
				nextPattern(ch);
				sb(kPatOfs, 0);
				continue;
			}
		}
		break;
	}

	if (d0 > 0x7F) {
		sf(0x10, ch, d0 & 0x3F);
		fetch();
	}
	if (d0 > 0x5F) {
		if (d0 == 0x7F || d0 == 0x7E) {
			// Pitch slides down (0x7F) or up (0x7E) one note per row.
			sf(0x2C, ch, d0 == 0x7F ? 1 : 0);
			sf(0x30, ch, d0 == 0x7E ? 1 : 0);
			sf(0x18, ch, 1);
			fetch();
		} else if (d0 == 0x7D || d0 == 0x7C) {
			// Arpeggio from a chord bit mask.
			sf(0x18, ch, d0 == 0x7D ? 1 : 2);
			fetch();
			sf(0x1C, ch, d0);
			sb(kArpBits, d0);
			buildArpeggio(ch);
			fetch();
		} else if (d0 == 0x7B) {
			// Portamento to a note.
			sf(0x1C, ch, 0);
			sf(0x18, ch, 1);
			d0 = b(a3++);
			sf(0x24, ch, d0 + f(0x20, ch));
			sf(0x28, ch, b(a3++));
			d4 += 2;
			fetch();
		} else if (d0 == 0x7A) {
			// Vibrato delay.
			sf(0x44, ch, b(a3++));
			d0 = b(a3);
			d4 += 2;
		}
	}

	// 0x260a80: start the note.
	sb(kDmaOff, b(kDmaOff) & ~(1 << ch));
	disableChannel(ch);
	_dmaOn[ch] = false;
	sb(kPatOfs, d4);
	setInstrument(ch);
	setEnvelope(ch);
	const uint8 note = d0 + f(0x20, ch);
	sf(0x08, ch, note);
	if (note && f(0x1C, ch) == 0 && f(0x34, ch))
		sf(0x08, ch, note + 2);
	notePeriod(ch);
	sl(kPatPtr + ch * 4, l(kPatPtr + ch * 4) + b(kPatOfs));
	sf(0x14, ch, f(0x10, ch));
	sf(0x48, ch, f(0x44, ch));
	sf(0x38, ch, f(0x04, ch) + 1);
	envelope(ch);
	return true;
}

void AmigaMusic::buildArpeggio(int ch) {
	// SYS5: semitone steps of the set chord bits, ended by 0xFF.
	uint8 pos = f(0x04, ch) + 1;
	uint8 bits = b(kArpBits);
	for (int k = 0; k < 8; ++k) {
		if (bits & 1)
			sb(kArpBuf + pos++, b(kArpSteps + k));
		bits >>= 1;
	}
	sb(kArpBits, 0);
	sb(kArpBuf + pos, 0xFF);
	sf(0x24, ch, 0);
}

void AmigaMusic::setEnvelope(int ch) {
	// SYS7: start volume, peak, attack, decay, sustain divisor, vibrato and
	// an optional arpeggio.
	const uint32 e = kEnvelopes + f(0x50, ch);
	static const uint kFields[6] = { 0x60, 0x54, 0x58, 0x5C, 0x70, 0x68 };
	for (int i = 0; i < 6; ++i)
		sf(kFields[i], ch, b(e + i));
	if (b(e + 6)) {
		sf(0x1C, ch, b(e + 6));
		sf(0x18, ch, 0x7C);
		sb(kArpBits, b(e + 6));
		buildArpeggio(ch);
	}
}

void AmigaMusic::setInstrument(int ch) {
	// SYS8: sample, loop flag, length, repeat offset and repeat length.
	const uint32 a = kInstruments + f(0x4C, ch);
	const uint8 sample = b(a);
	const uint8 loop = b(a + 1);
	sf(0x78, ch, loop);
	const uint16 length = loop ? w(a + 4) + w(a + 6) : w(a + 2);
	sw(kReplen + ch * 2, w(a + 6));
	sw(kRepOfs + ch * 2, w(a + 4));
	const uint32 loc = l(kSamples + sample * 4);
	sl(kSmpStart + ch * 4, loc);
	setLocation(ch, loc, length >> 1);
}

void AmigaMusic::setLocation(int ch, uint32 loc, uint16 lengthWords) {
	if (lengthWords == 0 || loc + lengthWords * 2 > _mem.size()) {
		setChannelSampleStart(ch, kSilence);
		setChannelSampleLen(ch, 1);
		return;
	}
	setChannelSampleStart(ch, (const int8 *)&_mem[loc]);
	setChannelSampleLen(ch, MIN<uint16>(lengthWords, 16383));
}

void AmigaMusic::effects(int ch) {
	// 0x260b40: arpeggio, portamento and vibrato between rows.
	if (f(0x08, ch) == 0)
		return;
	if (f(0x18, ch) == 0) {
		if (f(0x68, ch)) {
			vibrato(ch);
			return;
		}
		sf(0x80, ch, f(0x4E, ch));
	}
	if (f(0x1C, ch)) {
		uint8 v = 0xFF;
		for (int guard = 0; guard < 2 && v == 0xFF; ++guard) {
			v = b(kArpBuf + f(0x38, ch));
			if (v == 0xFF)
				sf(0x38, ch, f(0x04, ch));
		}
		const uint8 n = v + f(0x08, ch);
		sw(kOutPer + ch * 2, w(kPeriods + ((n * 2) & 0xFF)));
		sf(0x38, ch, f(0x38, ch) + 1);
		return;
	}
	if (!f(0x24, ch))
		return;

	_slideTarget = f(0x24, ch);
	_slideNibble = f(0x28, ch) & 0x0F;
	uint8 d0 = (f(0x28, ch) >> 4) + f(0x14, ch);
	const uint8 len = f(0x10, ch);
	if (!((int8)d0 > (int8)len)) {
		d0 += _slideNibble;
		if (d0 <= len) {
			sw(kBasePer + ch * 2, w(kSlideTgt + ch * 2));
		} else {
			slideSetup(ch, f(0x08, ch));
			uint16 p = w(kBasePer + ch * 2);
			const uint16 step = w(kSlideStep);
			p = b(kSlideUp) ? p - step : p + step;
			sw(kBasePer + ch * 2, p);
			sw(kOutPer + ch * 2, p);
		}
	}
	if (f(0x80, ch))
		vibrato(ch);
}

void AmigaMusic::slideSetup(int ch, uint8 note) {
	// SYS6 (0x260f7a): target period and the step per tick.
	const uint16 pt = w(kPeriods + ((_slideTarget * 2) & 0xFF));
	const uint16 pn = w(kPeriods + ((note * 2) & 0xFF));
	sw(kSlideTgt + ch * 2, pt);
	uint16 step;
	if (note < _slideTarget) {
		sb(kSlideUp, 1);
		step = pn - pt;
	} else {
		sb(kSlideUp, 0);
		step = pt - pn;
	}
	const uint8 div = _slideNibble * (b(kSpeed) + 1);
	sw(kSlideStep, div ? step / div : step);
}

void AmigaMusic::vibrato(int ch) {
	// 0x260b60
	const uint8 v = f(0x68, ch);
	const int shift = v & 0x0F;
	const uint8 width = v >> 4;
	const uint8 idx = (f(0x08, ch) * 2) & 0xFF;
	uint16 delta = w(kPeriods + idx) - w(kPeriods + idx + 2);
	for (int i = 0; i < shift; ++i)
		delta = (delta >> 1) | 1;

	uint8 dir = f(0x3C, ch);
	uint8 cnt = f(0x40, ch);
	if (dir & 0x80) {
		if (--cnt == 0)
			++dir;
	} else if (++cnt > width) {
		--dir;
	}
	sf(0x3C, ch, dir);
	sf(0x40, ch, cnt);

	if (f(0x48, ch)) {
		sf(0x48, ch, f(0x48, ch) - 1);
		return;
	}
	uint16 p = w(kBasePer + ch * 2);
	for (int i = 0; i < (width >> 1); ++i)
		p += delta;
	for (int i = 0; i < cnt; ++i)
		p -= delta;
	sw(kOutPer + ch * 2, p);
}

void AmigaMusic::envelope(int ch) {
	// 0x260d96: attack to the peak until the sustain point, then decay;
	// at volume 0 the channel's DMA goes off.
	const uint8 dur = f(0x14, ch);
	if (dur == 0)
		return;
	if (f(0x6C, ch) == 0) {
		const uint8 div = f(0x70, ch);
		if (div == 0) {
			sf(0x6C, ch, 1);
			return;
		}
		if (div == 0xFF)
			return;
		const uint8 q = f(0x10, ch) / div;
		if ((uint8)(dur - q)) {
			const uint8 vol = f(0x60, ch);
			if (vol != f(0x54, ch))
				sf(0x60, ch, vol + f(0x58, ch));
			return;
		}
	}
	sf(0x6C, ch, 1);
	const uint8 d0 = f(0x60, ch) - f(0x5C, ch);
	if (d0 && !(d0 & 0x80)) {
		sf(0x60, ch, d0);
	} else {
		sf(0x60, ch, 0);
		sb(kDmaOff, b(kDmaOff) | (1 << ch));
	}
}

void AmigaMusic::writeHardware() {
	// SYS_10: periods, DMA on/off and volumes.
	const uint8 off = b(kDmaOff) & 0x0F;
	for (int ch = 0; ch < 4; ++ch) {
		setChannelPeriod(ch, w(kOutPer + ch * 2));
		if (off & (1 << ch)) {
			disableChannel(ch);
			_dmaOn[ch] = false;
		} else if (!_dmaOn[ch]) {
			enableChannel(ch);
			_dmaOn[ch] = true;
		}
		setChannelVolume(ch, MIN<uint8>(b(kVolumes + ch), 64));
	}
}

} // namespace Goldbox
