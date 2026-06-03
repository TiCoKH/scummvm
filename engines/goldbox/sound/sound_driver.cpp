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
#include "common/util.h"

namespace Goldbox {

static const uint32 kPitClock = 1193182;

GoldboxSoundDriver::GoldboxSoundDriver(Audio::Mixer *mixer,
        const SoundData *data, SoundMode mode)
    : _mixer(mixer), _data(data), _mode(mode),
      _speakerEnabled(true), _tickAccumulator(0) {
    _outputRate = mixer->getOutputRate();
    for (int i = 0; i < kSoundChannels; ++i) {
        _channels[i].reset();
        _spkStreams[i] = new Audio::PCSpeakerStream(_outputRate);
    }
    memset(&_handle, 0, sizeof(_handle));
}

GoldboxSoundDriver::~GoldboxSoundDriver() {
    _mixer->stopHandle(_handle);
    for (int i = 0; i < kSoundChannels; ++i)
        delete _spkStreams[i];
}

void GoldboxSoundDriver::init() {
    _mixer->playStream(Audio::Mixer::kMusicSoundType, &_handle, this,
            -1, Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO);
}

void GoldboxSoundDriver::playSong(uint8 songId) {
    Common::StackLock lock(_mutex);
    if (_mode == kSoundOff)
        return;
    if (songId == 0) {
        silenceAll();
        _speakerEnabled = false;
    } else if (songId == 1) {
        _speakerEnabled = true;
    } else if (songId == 0xFF) {
        silenceAll();
    } else {
        loadSong(songId - 2);
    }
}

void GoldboxSoundDriver::stopAll() {
    Common::StackLock lock(_mutex);
    silenceAll();
}

void GoldboxSoundDriver::setSpeakerEnabled(bool enabled) {
    Common::StackLock lock(_mutex);
    _speakerEnabled = enabled;
}

// ---------------------------------------------------------------------------
// AudioStream
// ---------------------------------------------------------------------------

int GoldboxSoundDriver::readBuffer(int16 *buffer, const int numSamples) {
    Common::StackLock lock(_mutex);

    for (int i = 0; i < numSamples; ++i) {
        _tickAccumulator += kTimerFrequency;
        if (_tickAccumulator >= _outputRate) {
            _tickAccumulator -= _outputRate;
            tick();
        }
    }

    int channelLimit = (_mode == kSoundPCSpeaker) ? 1 : kSoundChannels;
    memset(buffer, 0, numSamples * sizeof(int16));

    int16 *chBuf = new int16[numSamples];
    for (int ch = 0; ch < channelLimit; ++ch) {
        _spkStreams[ch]->readBuffer(chBuf, numSamples);
        for (int i = 0; i < numSamples; ++i) {
            int32 mixed = (int32)buffer[i] + (int32)chBuf[i];
            buffer[i] = (int16)CLIP<int32>(mixed, -32767, 32767);
        }
    }
    delete[] chBuf;
    return numSamples;
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void GoldboxSoundDriver::tick() {
    for (int i = 0; i < kSoundChannels; ++i) {
        if (_channels[i].active)
            tickChannel(_channels[i]);
        queueChannelAudio(i);
    }
}

void GoldboxSoundDriver::queueChannelAudio(int chIdx) {
    const SoundChannel &ch = _channels[chIdx];

    if (!_speakerEnabled || !ch.active || ch.volume <= 0 || ch.outFreq <= 0) {
        _spkStreams[chIdx]->playQueue(Audio::PCSpeaker::kWaveFormSilence,
                0.0f, kTickLengthUs);
        return;
    }

    float hz;
    if (_mode == kSoundTandy)
        hz = 111860.0f / (float)((uint16)ch.outFreq >> 2);
    else
        hz = (float)kPitClock / (float)(uint16)ch.outFreq;

    _spkStreams[chIdx]->playQueue(Audio::PCSpeaker::kWaveFormSquare,
            hz, kTickLengthUs);
}

// ---------------------------------------------------------------------------
// Per-channel tick
// ---------------------------------------------------------------------------

void GoldboxSoundDriver::tickChannel(SoundChannel &ch) {
    // 1. Volume slide
    ch.volume = (int16)CLIP<int32>((int32)ch.volume + ch.volDelta, 0, 0x7FFF);

    // 2. Frequency slide
    ch.freq += ch.freqDelta;

    // 3. Vibrato
    int16 vibrato = 0;
    if (ch.vibSpeed != 0 && ch.vibTable != 0) {
        uint16 newPos = ch.vibPos + ch.vibSpeed;
        if (ch.vibWrap != 0 && newPos >= ch.vibWrap)
            newPos -= ch.vibWrap;
        ch.vibPos = newPos;

        // Read sample directly from blob at vibTable address
        int8 sample = (int8)_data->readByte(ch.vibTable + (newPos >> 4));
        vibrato = (int16)(((int32)(sample << 8) * (int32)ch.vibDepth) >> 16);
    }

    // 4. Output frequency
    ch.outFreq = ch.freq + vibrato;

    // 5. Delay timer
    if (ch.delayTimer != 0) {
        ch.delayTimer--;
        if (ch.delayTimer == 0) {
            ch.envOffset = 0;
            ch.envTicks = 1;
        }
    }

    // 6. Main duration
    if (ch.duration != 0) {
        ch.duration--;
        if (ch.duration == 0)
            loadNextCommands(ch);
    }

    // 7. Envelope
    if (ch.envTicks != 0)
        processEnvelope(ch);
}

// ---------------------------------------------------------------------------
// Envelope — reads 4-byte entries from blob at envBase + envOffset
// ---------------------------------------------------------------------------

void GoldboxSoundDriver::processEnvelope(SoundChannel &ch) {
    ch.envTicks--;
    if (ch.envTicks != 0)
        return;
    if (ch.envBase == 0)
        return;

    for (int safety = 0; safety < 64; ++safety) {
        uint16 addr = ch.envBase + ch.envOffset;
        int16 value = (int16)_data->readUint16(addr);
        int16 dur = (int16)_data->readUint16(addr + 2);
        ch.envOffset += 4;

        if (dur == -1) {
            // Set volume absolutely
            ch.volume = value;
            if (value == 0) {
                ch.volDelta = 0;
                ch.envTicks = 0;
                return;
            }
            continue;  // Process next entry immediately
        }

        // Slope: set volDelta, wait dur ticks
        ch.volDelta = value;
        ch.envTicks = (uint16)dur;
        return;
    }
}

// ---------------------------------------------------------------------------
// Command stream execution — reads directly from blob at ch.pc
// ---------------------------------------------------------------------------

void GoldboxSoundDriver::loadNextCommands(SoundChannel &ch) {
    if (ch.pc == 0) {
        ch.active = false;
        return;
    }

    for (int safety = 0; safety < 256; ++safety) {
        byte opcode = _data->readByte(ch.pc);

        switch (opcode) {
        case 0xFB:  // END
            ch.active = false;
            return;

        case 0xFC: {  // JUMP
            uint16 target = _data->readUint16(ch.pc + 1);
            ch.pc = target;
            continue;
        }

        case 0xFD: {  // CALL (save return addr, jump)
            // In the original this stores DI and jumps
            uint16 target = _data->readUint16(ch.pc + 1);
            ch.pc = target;
            continue;
        }

        case 0xFE: {  // LOOP (decrement counter, jump if >0)
            uint16 target = _data->readUint16(ch.pc + 1);
            ch.pc += 3;
            if (ch.loopCtr > 1) {
                ch.loopCtr--;
                ch.pc = target;
            }
            continue;
        }

        case 0xFF: {  // REGISTER WRITE
            uint8 reg = _data->readByte(ch.pc + 1);
            int16 val = (int16)_data->readUint16(ch.pc + 2);
            ch.pc += 4;

            switch (reg) {
            case 0x00:  // DURATION — pauses execution
                if (ch.tempoMult >= 2)
                    ch.duration = (uint16)val * ch.tempoMult;
                else
                    ch.duration = (uint16)val;
                return;

            case 0x04: ch.freq = val; ch.outFreq = val; break;
            case 0x06: ch.freqDelta = val; break;
            case 0x0A: ch.volume = val; break;
            case 0x0C: ch.volDelta = val; break;
            case 0x0E: ch.delayTimer = (uint16)val; break;
            case 0x10: ch.tempoMult = (uint8)(uint16)val; break;
            case 0x12: ch.transpose = val; break;
            case 0x16:  // ENV_BASE — raw segment pointer, used directly
                ch.envBase = (uint16)val;
                ch.envOffset = 0;
                ch.envTicks = 1;
                break;
            case 0x1C: ch.vibTable = (uint16)val; break;  // raw ptr, used directly
            case 0x1E: ch.vibPos = (uint16)val; break;
            case 0x20: ch.vibSpeed = (uint16)val; break;
            case 0x22: ch.vibDepth = val; break;
            case 0x24: ch.vibWrap = (uint16)val; break;
            case 0x26: ch.loopCtr = (uint16)val; break;
            case 0x28: ch.loopCtr2 = (uint16)val; break;
            default: break;
            }
            continue;
        }

        default:
            // Not a control byte — this is note data (v1.0 format)
            // TODO: handle note stream format
            ch.active = false;
            return;
        }
    }

    ch.active = false;
}

// ---------------------------------------------------------------------------
// Song loading
// ---------------------------------------------------------------------------

void GoldboxSoundDriver::loadSong(uint8 songIndex) {
    silenceAll();

    if (!_data || !_data->data || songIndex >= _data->songCount)
        return;

    bool tandy = (_mode == kSoundTandy);

    for (int ch = 0; ch < kSoundChannels; ++ch) {
        uint16 streamAddr = _data->getSongStreamAddr(tandy, songIndex, ch);
        if (streamAddr == 0)
            continue;

        _channels[ch].reset();
        _channels[ch].active = true;
        _channels[ch].pc = streamAddr;
        _channels[ch].duration = 1;  // Trigger immediate load
    }
}

void GoldboxSoundDriver::silenceAll() {
    for (int i = 0; i < kSoundChannels; ++i) {
        _channels[i].reset();
        _spkStreams[i]->stop(0);
    }
}

} // namespace Goldbox
