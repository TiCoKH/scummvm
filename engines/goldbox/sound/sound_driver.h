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

#ifndef GOLDBOX_SOUND_SOUND_DRIVER_H
#define GOLDBOX_SOUND_SOUND_DRIVER_H

#include "common/scummsys.h"
#include "common/mutex.h"
#include "audio/audiostream.h"
#include "audio/mixer.h"
#include "audio/softsynth/pcspk.h"
#include "goldbox/sound/sound_data.h"

namespace Goldbox {

/**
 * Timer frequency: original PIT divisor 0x13B1 = 5041.
 * Actual Hz = 1193182 / 5041 ~ 236.7 Hz.
 */
static const int kTimerFrequency = 237;

/**
 * Per-channel playback state.
 * Mirrors the original 0x30-byte DOS channel structure.
 */
struct SoundChannel {
    bool active;
    uint16 pc;           ///< Current stream position (segment-relative addr)

    int16 freq;          ///< +0x04: base frequency
    int16 freqDelta;     ///< +0x06: frequency change per tick
    int16 outFreq;       ///< +0x08: final output frequency
    int16 volume;        ///< +0x0A: current volume
    int16 volDelta;      ///< +0x0C: volume change per tick
    uint16 duration;     ///< +0x00: ticks remaining

    uint16 delayTimer;   ///< +0x14: one-shot envelope reset timer
    uint8 tempoMult;     ///< +0x10: duration multiplier
    int16 transpose;     ///< +0x12: note offset

    // Envelope (reads directly from blob)
    uint16 envBase;      ///< +0x16: segment addr of envelope data
    uint16 envOffset;    ///< +0x18: current byte offset into envelope
    uint16 envTicks;     ///< +0x1A: ticks until next envelope step

    // Vibrato (reads waveform directly from blob)
    uint16 vibTable;     ///< +0x1C: segment addr of waveform LUT
    uint16 vibPos;       ///< +0x1E: phase position
    uint16 vibSpeed;     ///< +0x20: phase advance per tick
    int16 vibDepth;      ///< +0x22: amplitude multiplier
    uint16 vibWrap;      ///< +0x24: phase wrap length

    uint16 loopCtr;      ///< +0x26: primary loop counter
    uint16 loopCtr2;     ///< +0x28: secondary loop counter

    void reset() {
        active = false;
        pc = 0;
        freq = 0; freqDelta = 0; outFreq = 0;
        volume = 0; volDelta = 0; duration = 0;
        delayTimer = 0; tempoMult = 0; transpose = 0;
        envBase = 0; envOffset = 0; envTicks = 0;
        vibTable = 0; vibPos = 0; vibSpeed = 0;
        vibDepth = 0; vibWrap = 0;
        loopCtr = 0; loopCtr2 = 0;
    }
};

/**
 * Sound output mode.
 */
enum SoundMode {
    kSoundOff = 0,
    kSoundPCSpeaker = 1,
    kSoundTandy = 2
};

/**
 * Goldbox PC Speaker / Tandy sound driver.
 *
 * Reads command streams directly from a raw binary blob (the dumped
 * sound segment). All pointers in the data self-resolve via
 * SoundData::toOffset().
 */
class GoldboxSoundDriver : public Audio::AudioStream {
public:
    GoldboxSoundDriver(Audio::Mixer *mixer, const SoundData *data,
            SoundMode mode);
    ~GoldboxSoundDriver() override;

    void init();
    void playSong(uint8 songId);
    void stopAll();
    void setSpeakerEnabled(bool enabled);

    // AudioStream interface
    int readBuffer(int16 *buffer, const int numSamples) override;
    bool isStereo() const override { return false; }
    int getRate() const override { return _outputRate; }
    bool endOfData() const override { return false; }

private:
    Audio::Mixer *_mixer;
    Audio::SoundHandle _handle;
    Common::Mutex _mutex;

    const SoundData *_data;
    SoundMode _mode;
    bool _speakerEnabled;
    int _outputRate;

    SoundChannel _channels[kSoundChannels];
    Audio::PCSpeakerStream *_spkStreams[kSoundChannels];

    int _tickAccumulator;
    static const uint32 kTickLengthUs = 1000000 / kTimerFrequency;

    void tick();
    void tickChannel(SoundChannel &ch);
    void loadNextCommands(SoundChannel &ch);
    void processEnvelope(SoundChannel &ch);
    void loadSong(uint8 songIndex);
    void silenceAll();
    void queueChannelAudio(int chIdx);
};

} // namespace Goldbox

#endif // GOLDBOX_SOUND_SOUND_DRIVER_H
