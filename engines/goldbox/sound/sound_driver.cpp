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

// PIT clock frequency for frequency-to-Hz conversion
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
        uint8 tableIndex = songId - 2;
        loadSong(tableIndex);
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
// AudioStream — generates PCM output by mixing PCSpeakerStream channels
// ---------------------------------------------------------------------------

int GoldboxSoundDriver::readBuffer(int16 *buffer, const int numSamples) {
    Common::StackLock lock(_mutex);

    // Advance tick timing and queue audio to PCSpeakerStreams
    int samplesPerTick = _outputRate / kTimerFrequency;
    int generated = 0;

    while (generated < numSamples) {
        _tickAccumulator += kTimerFrequency;
        if (_tickAccumulator >= _outputRate) {
            _tickAccumulator -= _outputRate;
            tick();
        }
        generated++;
    }

    // Now read from PCSpeakerStreams and mix
    int channelLimit = (_mode == kSoundPCSpeaker) ? 1 : kSoundChannels;

    // Zero the output buffer
    memset(buffer, 0, numSamples * sizeof(int16));

    // Mix each channel's PCSpeakerStream output
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
// Tick — called at ~237 Hz (original timer rate)
// ---------------------------------------------------------------------------

void GoldboxSoundDriver::tick() {
    for (int i = 0; i < kSoundChannels; ++i) {
        if (_channels[i].active)
            tickChannel(_channels[i]);
        queueChannelAudio(i);
    }
}

// ---------------------------------------------------------------------------
// Queue one tick's worth of audio to the PCSpeakerStream for a channel
// ---------------------------------------------------------------------------

void GoldboxSoundDriver::queueChannelAudio(int chIdx) {
    const SoundChannel &ch = _channels[chIdx];

    if (!_speakerEnabled || !ch.active || ch.volume <= 0 || ch.outFreq <= 0) {
        _spkStreams[chIdx]->playQueue(Audio::PCSpeaker::kWaveFormSilence,
                0.0f, kTickLengthUs);
        return;
    }

    float hz = (float)kPitClock / (float)(uint16)ch.outFreq;
    _spkStreams[chIdx]->playQueue(Audio::PCSpeaker::kWaveFormSquare,
            hz, kTickLengthUs);
}

/**
 * Per-channel tick processing. This is the XT_PROGRAM equivalent.
 *
 * Execution order (matches original):
 *   1. Apply volume envelope slope
 *   2. Apply frequency portamento
 *   3. Calculate vibrato modulation
 *   4. Compute final output frequency
 *   5. Handle delay timer
 *   6. Decrement main duration counter -> load commands if zero
 *   7. Decrement envelope timer -> consume entries if zero
 */
void GoldboxSoundDriver::tickChannel(SoundChannel &ch) {
    // 1. Volume slide
    int32 newVol = (int32)ch.volume + ch.volDelta;
    ch.volume = (int16)CLIP<int32>(newVol, 0, 0x7FFF);

    // 2. Frequency slide
    ch.freq += ch.freqDelta;

    // 3. Vibrato
    int16 vibrato = 0;
    if (ch.vibSpeed != 0 && ch.vibWaveId < _data->waveforms.size()) {
        uint16 newPos = ch.vibPos + ch.vibSpeed;
        if (ch.vibWrap != 0 && newPos >= ch.vibWrap)
            newPos -= ch.vibWrap;
        ch.vibPos = newPos;

        uint8 idx = (uint8)(newPos >> 4);
        int8 sample = (int8)_data->waveforms[ch.vibWaveId].samples[idx];
        vibrato = (int16)(((int32)(sample << 8) *
                (int32)ch.vibDepth) >> 16);
    }

    // 4. Output frequency
    ch.outFreq = ch.freq + vibrato;

    // 5. Delay timer
    if (ch.delayTimer != 0) {
        ch.delayTimer--;
        if (ch.delayTimer == 0) {
            ch.envOffset = 0;
            ch.envTicksLeft = 1;
        }
    }

    // 6. Main duration
    if (ch.ticksLeft != 0) {
        ch.ticksLeft--;
        if (ch.ticksLeft == 0)
            loadNextCommands(ch);
    }

    // 7. Envelope
    if (ch.envTicksLeft != 0)
        processEnvelope(ch);
}

// ---------------------------------------------------------------------------
// Envelope processing
// ---------------------------------------------------------------------------

void GoldboxSoundDriver::processEnvelope(SoundChannel &ch) {
    ch.envTicksLeft--;
    if (ch.envTicksLeft != 0)
        return;

    if (ch.envStreamId >= _data->envelopes.size())
        return;

    const EnvelopeStream &env = _data->envelopes[ch.envStreamId];

    for (int safety = 0; safety < 64; ++safety) {
        if (ch.envOffset >= env.entries.size())
            return;

        const EnvelopeEntry &entry = env.entries[ch.envOffset];
        ch.envOffset++;

        switch (entry.type) {
        case EnvelopeEntry::SET_VOL:
            ch.volume = entry.value;
            if (entry.value == 0)
                ch.volDelta = 0;
            continue;  // Process next immediately

        case EnvelopeEntry::SILENCE:
            ch.volume = 0;
            ch.volDelta = 0;
            ch.envTicksLeft = 0;
            return;

        case EnvelopeEntry::SLOPE:
            ch.volDelta = entry.value;
            ch.envTicksLeft = entry.duration;
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Command stream execution (XT_GameSTART equivalent)
// ---------------------------------------------------------------------------

void GoldboxSoundDriver::loadNextCommands(SoundChannel &ch) {
    if (ch.streamId >= _data->streams.size()) {
        ch.active = false;
        return;
    }

    const SoundStream &stream = _data->streams[ch.streamId];

    for (int safety = 0; safety < 256; ++safety) {
        if (ch.cmdIndex >= stream.commands.size()) {
            ch.active = false;
            return;
        }

        const SoundCommand &cmd = stream.commands[ch.cmdIndex];
        ch.cmdIndex++;

        switch (cmd.type) {
        case SoundCommand::END:
            ch.active = false;
            return;

        case SoundCommand::JUMP:
            ch.cmdIndex = cmd.loopTarget;
            continue;

        case SoundCommand::LOOP_START:
            ch.loopCounter = (uint16)cmd.value;
            continue;

        case SoundCommand::LOOP_BACK:
            if (ch.loopCounter > 1) {
                ch.loopCounter--;
                ch.cmdIndex = cmd.loopTarget;
            }
            continue;

        case SoundCommand::COND_JUMP:
            if (ch.loopCounter2 > 0) {
                ch.loopCounter2--;
                ch.cmdIndex = cmd.loopTarget;
            }
            continue;

        case SoundCommand::REG_WRITE:
            break;  // Handle below
        }

        // Process register write
        uint16 uval = (uint16)cmd.value;
        switch (cmd.reg) {
        case SoundCommand::REG_DURATION:
            if (ch.tempoMult >= 2)
                ch.ticksLeft = uval * ch.tempoMult;
            else
                ch.ticksLeft = uval;
            return;  // Duration set — pause execution

        case SoundCommand::REG_FREQ:
            ch.freq = cmd.value;
            ch.outFreq = cmd.value;
            break;

        case SoundCommand::REG_FREQ_DELTA:
            ch.freqDelta = cmd.value;
            break;

        case SoundCommand::REG_VOLUME:
            ch.volume = cmd.value;
            break;

        case SoundCommand::REG_VOL_DELTA:
            ch.volDelta = cmd.value;
            break;

        case SoundCommand::REG_DELAY_TIMER:
            ch.delayTimer = uval;
            break;

        case SoundCommand::REG_TEMPO:
            ch.tempoMult = (uint8)uval;
            break;

        case SoundCommand::REG_TRANSPOSE:
            ch.transpose = cmd.value;
            break;

        case SoundCommand::REG_ENV_STREAM:
            ch.envStreamId = uval;
            ch.envOffset = 0;
            ch.envTicksLeft = 1;  // Trigger immediately
            break;

        case SoundCommand::REG_VIB_WAVE:
            ch.vibWaveId = uval;
            break;

        case SoundCommand::REG_VIB_POS:
            ch.vibPos = uval;
            break;

        case SoundCommand::REG_VIB_SPEED:
            ch.vibSpeed = uval;
            break;

        case SoundCommand::REG_VIB_DEPTH:
            ch.vibDepth = cmd.value;
            break;

        case SoundCommand::REG_VIB_WRAP:
            ch.vibWrap = uval;
            break;

        case SoundCommand::REG_LOOP_CTR:
            ch.loopCounter = uval;
            break;

        case SoundCommand::REG_LOOP_CTR2:
            ch.loopCounter2 = uval;
            break;

        default:
            break;
        }
    }

    // Safety limit reached
    ch.active = false;
}

// ---------------------------------------------------------------------------
// Song loading
// ---------------------------------------------------------------------------

void GoldboxSoundDriver::loadSong(uint8 songIndex) {
    silenceAll();

    const Common::Array<SongDefinition> &songs =
            (_mode == kSoundTandy) ? _data->tandySongs : _data->speakerSongs;

    if (songIndex >= songs.size())
        return;

    const SongDefinition &song = songs[songIndex];

    for (int ch = 0; ch < kSoundChannels; ++ch) {
        uint16 streamId = song.channelStream[ch];
        if (streamId == kStreamIdNone)
            continue;
        if (streamId >= _data->streams.size())
            continue;

        _channels[ch].reset();
        _channels[ch].active = true;
        _channels[ch].streamId = streamId;
        _channels[ch].cmdIndex = 0;
        _channels[ch].ticksLeft = 1;  // Trigger immediate load
    }
}

void GoldboxSoundDriver::silenceAll() {
    for (int i = 0; i < kSoundChannels; ++i) {
        _channels[i].reset();
        _spkStreams[i]->stop(0);
    }
}

} // namespace Goldbox
