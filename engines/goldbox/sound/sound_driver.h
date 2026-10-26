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
#include "common/path.h"
#include "audio/audiostream.h"
#include "audio/mixer.h"
#include "goldbox/sound/sn76496.h"
#include "goldbox/sound/sound_segment.h"

namespace Goldbox {

/**
 * Sound output mode.
 */
enum SoundMode {
    kSoundOff = 0,
    kSoundPCSpeaker = 1,
    kSoundTandy = 2
};

/**
 * Goldbox PC speaker / Tandy sound driver.
 *
 * A port of the original DOS driver: a 236.7 Hz timer interrupt runs a
 * bytecode sequencer over 4 channels, whose state lives in the original
 * sound data segment (loaded from sound.dat). The result is
 * sent to an emulated PIT channel 2 / PC speaker gate or to an emulated
 * SN76496 compatible chip at register level.
 */
class GoldboxSoundDriver : public Audio::AudioStream {
public:
    GoldboxSoundDriver(Audio::Mixer *mixer, SoundMode mode);
    ~GoldboxSoundDriver() override;

    /**
     * Load the sound data segment from a sound.dat file and start the
     * output stream. Initializes like the original SetSound startup.
     * @return false if the sound data could not be loaded
     */
    bool init(const Common::Path &dataFile);

    /** SOUND_Play: 0 = sound off, 1 = sound on, 0xFF = silence,
     *  n >= 2 = play sound table entry n - 1. */
    void play(uint8 sound);

    /** SOUND_Setup: 0 = continuous mode, > 0 = short blip mode. */
    void setup(uint16 flag);

    /** Raw access to WORD_SOUND_FLAG (written by ECL scripts). */
    void setSoundFlag(uint16 flag);
    uint16 getSoundFlag() const { return _soundFlag; }

    void stopAll();

    // AudioStream interface
    int readBuffer(int16 *buffer, const int numSamples) override;
    bool isStereo() const override { return false; }
    int getRate() const override { return (int)_outputRate; }
    bool endOfData() const override { return false; }

private:
    struct Bank {
        uint16 freqBase;
        uint16 chanBase;
        uint16 soundTable;
        uint8 owner[4];
    };

    Audio::Mixer *_mixer;
    Audio::SoundHandle _handle;
    Common::Mutex _mutex;

    SoundSegment _seg;
    SoundSegmentLayout _layout;
    SoundMode _mode;
    uint32 _outputRate;
    bool _ready;

    Bank _speaker;
    Bank _tandy;
    uint16 _soundId;       ///< WORD_SOUND_ID
    uint8 _speakerEnable;  ///< BYTE_SPEAKER_ENABLE
    uint16 _curChannel;    ///< PTR_CURRENT_CHANNEL
    uint16 _subReturn;     ///< PTR_SUB_RET_ADDR
    uint16 _soundFlag;     ///< WORD_SOUND_FLAG

    bool _timerRunning;
    uint32 _timerAccumulator;
    int _blipTicks;

    // PC speaker: PIT channel 2 divisor and port 61h bits 0-1
    uint16 _pitDivisor;
    uint8 _speakerGate;
    uint32 _pitPhase;
    uint32 _pitAccumulator;

    Sn76496 _psg;

    byte read8(uint16 addr) const;
    uint16 read16(uint16 addr) const;
    void write16(uint16 addr, uint16 value);

    void startTimer();
    void stopTimer();
    void silence();

    void loadSound(Bank &bank, uint16 id);
    void loadActive(uint16 id);
    void timerTick();
    void speakerDma();
    void tandyDma();
    void writeTandySilence();
    void advanceChannel(Bank &bank, uint16 si, int index);
    void soundStart(Bank &bank, uint16 si, int index);
    void resetChannel(uint16 si);
    int16 speakerSample();
};

} // namespace Goldbox

#endif // GOLDBOX_SOUND_SOUND_DRIVER_H
