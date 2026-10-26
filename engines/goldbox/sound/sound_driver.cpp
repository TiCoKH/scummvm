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

#include "goldbox/sound/sound_driver.h"
#include "common/debug.h"
#include "common/endian.h"
#include "common/textconsole.h"
#include "common/util.h"

namespace Goldbox {

namespace {

const uint32 kPitClock = 1193182;
const uint32 kTimerDivisor = 0x13B1;   // ~236.7 Hz driver tick
const uint16 kChannelSize = 0x30;
const int kBankChannels = 4;
const int kMaxCommands = 1024;
const int kMaxEnvelopeSteps = 64;
const int16 kSpeakerAmplitude = 6000;

/**
 * Blip mode waits WaitSeconds(10) (1/10 s, in 2 BIOS tick granularity)
 * before silencing; at 13 driver ticks per BIOS tick this is 13-26 ticks.
 */
const int kBlipTicks = 20;

// Channel structure field offsets
enum {
    CH_DURATION = 0x00,
    CH_STREAM = 0x02,
    CH_FREQ = 0x04,
    CH_FREQ_DELTA = 0x06,
    CH_OUT_FREQ = 0x08,
    CH_VOLUME = 0x0A,
    CH_VOL_DELTA = 0x0C,
    CH_TEMPO = 0x0E,
    CH_GAP = 0x10,
    CH_TRANSPOSE = 0x12,
    CH_RELEASE = 0x14,
    CH_ENV_BASE = 0x16,
    CH_ENV_POS = 0x18,
    CH_ENV_TICKS = 0x1A,
    CH_VIB_TABLE = 0x1C,
    CH_VIB_POS = 0x1E,
    CH_VIB_SPEED = 0x20,
    CH_VIB_DEPTH = 0x22,
    CH_VIB_WRAP = 0x24
};

} // End of anonymous namespace

GoldboxSoundDriver::GoldboxSoundDriver(Audio::Mixer *mixer, SoundMode mode)
        : _mixer(mixer), _mode(mode), _outputRate(0), _ready(false),
        _soundId(0), _speakerEnable(1), _curChannel(0), _subReturn(0),
        _soundFlag(0), _timerRunning(false), _timerAccumulator(0),
        _blipTicks(0), _pitDivisor(0), _speakerGate(0), _pitPhase(0),
        _pitAccumulator(0) {
    _outputRate = _mixer ? _mixer->getOutputRate() : 0;
    memset(&_speaker, 0, sizeof(_speaker));
    memset(&_tandy, 0, sizeof(_tandy));
}

GoldboxSoundDriver::~GoldboxSoundDriver() {
    if (_ready)
        _mixer->stopHandle(_handle);
}

bool GoldboxSoundDriver::init(const Common::Path &dataFile) {
    if (_mode == kSoundOff || !_mixer || !_outputRate)
        return false;
    if (!_seg.loadFromFile(dataFile))
        return false;

    _layout = _seg.getLayout();
    _speaker.freqBase = _layout.speakerFreq;
    _speaker.chanBase = _layout.speakerChannels;
    _speaker.soundTable = _layout.speakerSounds;
    _tandy.freqBase = _layout.tandyFreq;
    _tandy.chanBase = _layout.tandyChannels;
    _tandy.soundTable = _layout.tandySounds;

    // SetSound startup: start the timer, load silence, stop the timer
    startTimer();
    loadSound(_tandy, 0);
    loadSound(_speaker, 0);
    stopTimer();

    _ready = true;
    _mixer->playStream(Audio::Mixer::kSFXSoundType, &_handle, this, -1,
        Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO, true);
    return true;
}

void GoldboxSoundDriver::play(uint8 sound) {
    if (!_ready)
        return;
    Common::StackLock lock(_mutex);

    if (_soundFlag > 0)
        startTimer();

    switch (sound) {
    case 0:
        loadSound(_tandy, 0);
        loadSound(_speaker, 0);
        _speakerEnable = 0;
        break;
    case 1:
        _speakerEnable = 1;
        break;
    case 0xFF:
        loadSound(_tandy, 0);
        loadSound(_speaker, 0);
        break;
    default:
        loadActive(sound - 1);
        break;
    }

    // The original blocks for the blip length here; the stream ends the
    // blip asynchronously instead.
    _blipTicks = (_soundFlag > 0) ? kBlipTicks : 0;
}

void GoldboxSoundDriver::setup(uint16 flag) {
    if (!_ready)
        return;
    Common::StackLock lock(_mutex);

    if (flag > 0 && _soundFlag == 0)
        stopTimer();
    if (flag == 0) {
        _blipTicks = 0;
        startTimer();
    }
    _soundFlag = flag;
}

void GoldboxSoundDriver::setSoundFlag(uint16 flag) {
    Common::StackLock lock(_mutex);
    _soundFlag = flag;
}

void GoldboxSoundDriver::stopAll() {
    if (!_ready)
        return;
    Common::StackLock lock(_mutex);
    loadSound(_tandy, 0);
    loadSound(_speaker, 0);
    _blipTicks = 0;
    stopTimer();
}

byte GoldboxSoundDriver::read8(uint16 addr) const {
    return _seg.data()[addr];
}

uint16 GoldboxSoundDriver::read16(uint16 addr) const {
    const byte *mem = _seg.data();
    return (uint16)(mem[addr] | (mem[(uint16)(addr + 1)] << 8));
}

void GoldboxSoundDriver::write16(uint16 addr, uint16 value) {
    byte *mem = _seg.data();
    mem[addr] = value & 0xFF;
    mem[(uint16)(addr + 1)] = value >> 8;
}

void GoldboxSoundDriver::startTimer() {
    _timerRunning = true;
}

void GoldboxSoundDriver::stopTimer() {
    _timerRunning = false;
    silence();
}

void GoldboxSoundDriver::silence() {
    _speakerGate = 0;
    writeTandySilence();
}

void GoldboxSoundDriver::writeTandySilence() {
    _psg.write(0x9F);
    _psg.write(0xBF);
    _psg.write(0xDF);
    _psg.write(0xFF);
}

void GoldboxSoundDriver::loadSound(Bank &bank, uint16 id) {
    if (id >= _layout.numSounds)
        id = 0;
    _soundId = id;

    uint16 ch = bank.chanBase;
    for (int i = 0; i < kBankChannels; ++i, ch += kChannelSize) {
        const uint16 stream = read16(bank.soundTable + id * 8 + i * 2);
        if (!stream)
            continue;
        for (uint16 off = 0; off < kChannelSize; off += 2)
            write16(ch + off, 0);
        write16(ch + CH_STREAM, stream);
        bank.owner[i] = (uint8)id;
        write16(ch + CH_DURATION, 1);
    }
}

void GoldboxSoundDriver::loadActive(uint16 id) {
    if (_mode == kSoundPCSpeaker)
        loadSound(_speaker, id);
    else
        loadSound(_tandy, id);
}

void GoldboxSoundDriver::timerTick() {
    if (_mode == kSoundPCSpeaker)
        speakerDma();
    else
        tandyDma();

    if (_blipTicks > 0 && --_blipTicks == 0) {
        loadSound(_tandy, 0);
        loadSound(_speaker, 0);
        stopTimer();
    }
}

void GoldboxSoundDriver::speakerDma() {
    if (_soundId == 0) {
        _speakerGate = 0;
        return;
    }

    uint16 best = 0;
    uint16 si = _speaker.chanBase;
    for (int i = 0; i < kBankChannels; ++i, si += kChannelSize) {
        if (!read16(si + CH_DURATION))
            continue;
        advanceChannel(_speaker, si, i);
        if (!best && read16(si + CH_VOLUME) && read16(si + CH_DURATION))
            best = si;
    }

    if (_speakerEnable && best) {
        _pitDivisor = read16(best + CH_OUT_FREQ);
        _speakerGate = read16(best + CH_VOLUME) & 3;
    } else {
        _speakerGate = 0;
    }
}

void GoldboxSoundDriver::tandyDma() {
    if (_soundId == 0) {
        writeTandySilence();
        return;
    }

    uint16 si = _tandy.chanBase;
    for (int i = 0; i < kBankChannels; ++i, si += kChannelSize) {
        if (read16(si + CH_DURATION))
            advanceChannel(_tandy, si, i);
    }

    if (!_speakerEnable) {
        writeTandySilence();
        return;
    }

    byte dl = 0x80;
    si = _tandy.chanBase;
    for (int i = 0; i < kBankChannels; ++i, si += kChannelSize, dl += 0x20) {
        const uint16 period = read16(si + CH_OUT_FREQ);
        _psg.write(dl | ((period >> 6) & 0x0F));
        if (dl != 0xE0)
            _psg.write((byte)(period >> 10));
        const uint16 inv = 0xFFFF - read16(si + CH_VOLUME);
        _psg.write((byte)(((inv >> 12) ^ dl) + 0x10));
    }
}

void GoldboxSoundDriver::advanceChannel(Bank &bank, uint16 si, int index) {
    write16(si + CH_VOLUME, read16(si + CH_VOLUME) +
        read16(si + CH_VOL_DELTA));
    write16(si + CH_FREQ, read16(si + CH_FREQ) + read16(si + CH_FREQ_DELTA));

    // Vibrato: signed waveform byte * depth, high word of the product
    uint16 dx = 0;
    uint16 pos = read16(si + CH_VIB_POS) + read16(si + CH_VIB_SPEED);
    if (pos != 0) {
        const uint16 wrap = read16(si + CH_VIB_WRAP);
        if (pos >= wrap)
            pos -= wrap;
        write16(si + CH_VIB_POS, pos);
        const uint16 addr = (uint16)(((int16)pos >> 4) +
            read16(si + CH_VIB_TABLE));
        const int32 sample = (int16)(uint16)(read8(addr) << 8);
        const int32 product = sample * (int16)read16(si + CH_VIB_DEPTH);
        dx = (uint16)((uint32)product >> 16);
    }
    write16(si + CH_OUT_FREQ, dx + read16(si + CH_FREQ));

    // Release timer: jump to the envelope's release section
    uint16 release = read16(si + CH_RELEASE);
    if (release != 0) {
        write16(si + CH_RELEASE, --release);
        if (release == 0) {
            write16(si + CH_ENV_POS, 0x10);
            write16(si + CH_ENV_TICKS, 1);
        }
    }

    const uint16 duration = read16(si + CH_DURATION) - 1;
    write16(si + CH_DURATION, duration);
    if (duration == 0)
        soundStart(bank, si, index);

    // Volume envelope: (value, ticks) pairs, ticks == 0xFFFF sets volume
    uint16 envTicks = read16(si + CH_ENV_TICKS);
    if (envTicks == 0)
        return;
    write16(si + CH_ENV_TICKS, --envTicks);
    if (envTicks != 0)
        return;

    for (int steps = 0; steps < kMaxEnvelopeSteps; ++steps) {
        const uint16 bx = read16(si + CH_ENV_BASE) + read16(si + CH_ENV_POS);
        const uint16 value = read16(bx);
        const uint16 ticks = read16(bx + 2);
        write16(si + CH_ENV_POS, read16(si + CH_ENV_POS) + 4);
        if (ticks != 0xFFFF) {
            write16(si + CH_VOL_DELTA, value);
            write16(si + CH_ENV_TICKS, ticks);
            return;
        }
        write16(si + CH_VOLUME, value);
        if (value == 0)
            write16(si + CH_VOL_DELTA, 0);
    }
    warning("Goldbox sound: runaway volume envelope");
}

void GoldboxSoundDriver::resetChannel(uint16 si) {
    static const byte kResetFields[] = {
        CH_FREQ, CH_FREQ_DELTA, CH_OUT_FREQ, CH_VOLUME, CH_VOL_DELTA,
        CH_GAP, CH_TRANSPOSE, CH_ENV_BASE, CH_ENV_POS, CH_ENV_TICKS,
        CH_VIB_TABLE, CH_VIB_POS, CH_VIB_SPEED, CH_VIB_DEPTH, CH_VIB_WRAP
    };
    for (uint i = 0; i < ARRAYSIZE(kResetFields); ++i)
        write16(si + kResetFields[i], 0);
}

void GoldboxSoundDriver::soundStart(Bank &bank, uint16 si, int index) {
    uint16 di = read16(si + CH_STREAM);
    if (!di)
        return;
    _curChannel = si;

    bool events = false;
    bool done = false;
    for (int count = 0; !done; ++count) {
        if (count >= kMaxCommands) {
            warning("Goldbox sound: runaway command stream");
            write16(_curChannel + CH_DURATION, 0);
            break;
        }

        const byte cmd = read8(di++);
        if (events || cmd < 0xFA) {
            // Note event: [channel:3 | ...][note:7 | end:1][length]
            const uint8 target = cmd >> 5;
            const uint16 ch = (uint16)(bank.chanBase + target * kChannelSize);
            const uint8 tempo = read16(si + CH_TEMPO) & 0xFF;
            const uint8 length = read8(di + 1);
            const uint16 duration = (tempo <= 1) ? length :
                (uint16)(tempo * length);
            write16(si + CH_DURATION, duration);

            uint16 note = read8(di) & 0x7F;
            const bool applies = _layout.noteAnyChannel || target == 0;
            if (note != 0x7F && applies) {
                write16(ch + CH_DURATION, duration);
                if (_layout.noteAnyChannel) {
                    const uint16 gap = read16(ch + CH_GAP);
                    write16(ch + CH_RELEASE,
                        duration >= gap ? duration - gap : 0);
                } else {
                    write16(ch + CH_RELEASE, duration);
                }
                note = note + read16(ch + CH_TRANSPOSE);
                write16(ch + CH_ENV_POS, 0);
                write16(ch + CH_ENV_TICKS, 1);

                uint8 octave = 0;
                while (note >= 12) {
                    note -= 12;
                    ++octave;
                }
                const uint16 base = read16(bank.freqBase + note * 2);
                const uint8 shift = octave & 0x1F;
                const uint16 freq = (shift >= 16) ? 0 : (base >> shift);
                write16(ch + CH_FREQ, freq);
                write16(ch + CH_OUT_FREQ, freq);
            }

            const byte flags = read8(di);
            di += 2;
            if (flags & 0x80)
                done = true;
            events = true;
            continue;
        }

        switch (cmd) {
        case 0xFA:
            resetChannel(si);
            break;
        case 0xFB:
            di = _subReturn;
            if (_layout.safeReturn) {
                _subReturn = 0;
                if (!di) {
                    write16(_curChannel + CH_DURATION, 0);
                    done = true;
                }
            }
            break;
        case 0xFC: {
            const uint16 target = read16(di);
            _subReturn = di + 2;
            di = target;
            break;
        }
        case 0xFD:
            si = read16(di) + bank.chanBase;
            di += 2;
            resetChannel(si);
            break;
        case 0xFE: {
            const byte field = read8(di++);
            const uint16 target = read16(di);
            di += 2;
            const uint16 addr = si + field;
            uint16 counter = read16(addr);
            if (counter != 0) {
                write16(addr, --counter);
                if (counter == 0)
                    break;
            }
            di = target;
            break;
        }
        default: {
            // 0xFF: set channel field
            const byte field = read8(di++);
            write16(si + field, read16(di));
            di += 2;
            if (field == 0)
                done = true;
            break;
        }
        }
    }

    si = _curChannel;
    if (read16(si + CH_DURATION) == 0) {
        bank.owner[index] = 0;
        di = 0;
    }
    write16(si + CH_STREAM, di);
}

int16 GoldboxSoundDriver::speakerSample() {
    _pitAccumulator += kPitClock;
    const uint32 clocks = _pitAccumulator / _outputRate;
    _pitAccumulator %= _outputRate;

    if (_speakerGate != 3 || clocks == 0)
        return 0;

    // PIT mode 3 square wave: high for ceil(N/2) clocks, low for floor(N/2)
    const uint32 period = _pitDivisor ? _pitDivisor : 0x10000;
    const uint32 highTime = (period + 1) / 2;
    if (_pitPhase >= period)
        _pitPhase %= period;

    int32 sum = 0;
    uint32 remaining = clocks;
    while (remaining) {
        const bool high = _pitPhase < highTime;
        const uint32 segEnd = high ? highTime : period;
        const uint32 n = MIN(segEnd - _pitPhase, remaining);
        sum += high ? (int32)n : -(int32)n;
        _pitPhase += n;
        remaining -= n;
        if (_pitPhase >= period)
            _pitPhase = 0;
    }

    return (int16)(sum * kSpeakerAmplitude / (int32)clocks);
}

int GoldboxSoundDriver::readBuffer(int16 *buffer, const int numSamples) {
    Common::StackLock lock(_mutex);

    const uint32 tickThreshold = kTimerDivisor * _outputRate;
    for (int i = 0; i < numSamples; ++i) {
        _timerAccumulator += kPitClock;
        while (_timerAccumulator >= tickThreshold) {
            _timerAccumulator -= tickThreshold;
            if (_timerRunning)
                timerTick();
        }

        if (_mode == kSoundPCSpeaker)
            buffer[i] = speakerSample();
        else
            buffer[i] = _psg.nextSample(_outputRate);
    }
    return numSamples;
}

} // namespace Goldbox
