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
 * @file sound_driver.h
 * @brief Gold Box series PC Speaker / Tandy 3-voice sound driver.
 *
 * ============================================================================
 * ARCHITECTURE OVERVIEW
 * ============================================================================
 *
 * The original DOS sound driver is a timer-interrupt-driven music player
 * embedded in the game executable (not loaded from external files). It runs
 * at approximately 240 Hz (PIT channel 0 reprogrammed with divisor 0x13B1)
 * and produces sound through either the PC Speaker (PIT channel 2) or the
 * Tandy TI SN76489 PSG chip (I/O port 0xC0).
 *
 * The driver supports:
 *   - PC Speaker mode: 1 square-wave voice
 *   - Tandy mode: 4 voices (3 tone + 1 noise), each with independent
 *     frequency, volume, vibrato, and envelope control
 *
 * ============================================================================
 * DATA LAYOUT IN THE EXECUTABLE
 * ============================================================================
 *
 * All sound data resides in a single segment of the DOS overlay (segment
 * 18AE in Pool of Radiance v1.3). The layout is:
 *
 *   Offset  Size    Content
 *   ------  ------  -------------------------------------------------------
 *   0x0547  24      Frequency table: 12 x uint16 LE
 *                   PIT divisors for chromatic notes C through B (octave 0).
 *                   Higher octaves computed by right-shifting: freq >> octave.
 *                   Values descend: C=0xFFC0, C#=0xF140, ... B=0x8740.
 *
 *   0x055F  168     Tandy song pointer table: 21 songs x 4 channels x uint16
 *                   Each uint16 is a segment-relative offset to a command
 *                   stream. Value 0x0000 = channel unused. The "silence"
 *                   stream is at 0x0D2B (immediately returns/does nothing).
 *
 *   0x0607  168     Speaker song pointer table: same layout as Tandy.
 *                   Silence stream at 0x11BF.
 *
 *   0x06AF  256     Sine vibrato waveform LUT (signed 8-bit samples).
 *                   Smooth sine: 0 -> +126 -> 0 -> -126 -> 0.
 *
 *   0x07AF  16      Zero waveform (no modulation).
 *
 *   0x07BF  16      DC offset waveform (constant 0x78).
 *
 *   0x07CF  ~288    Noise/random waveform (pseudo-random bytes).
 *                   Used for percussive/harsh sound effects.
 *
 *   0x09CF  ~2432   Command stream data for all songs/effects.
 *                   Variable-length streams terminated by control bytes.
 *
 * ============================================================================
 * COMMAND STREAM FORMAT
 * ============================================================================
 *
 * Each song channel points to a "command stream" — a sequence of register
 * writes that program the channel's state. The stream is processed by
 * XT_GameSTART when the channel's duration counter reaches zero.
 *
 * Basic format: [separator_byte] [register_id] [value_lo] [value_hi]
 *
 * Separator bytes (4th byte of each command group):
 *   0xFF = Execute immediately, then read next command (no delay)
 *   0xFE = Loop marker: next 2 bytes are [loop_register, loop_target_lo]
 *   0xFD = Conditional jump (subroutine-like)
 *   0xFC = Unconditional jump to address
 *   0xFB = End of stream / return
 *
 * Register IDs (map to channel struct offsets):
 *   0x00 = DURATION    - Ticks to wait before loading next command block
 *   0x04 = FREQ        - Base frequency (16-bit PIT/PSG divisor)
 *   0x06 = FREQ_DELTA  - Added to freq each tick (portamento/slide)
 *   0x0A = VOLUME      - Current volume level (16-bit)
 *   0x0C = VOL_DELTA   - Added to volume each tick (envelope slope)
 *   0x0E = UNK_0E      - Unknown parameter
 *   0x10 = TEMPO_MULT  - Duration multiplier
 *   0x12 = TRANSPOSE   - Added to note numbers before frequency lookup
 *   0x16 = ENV_BASE    - Pointer to envelope sub-stream
 *   0x1C = VIB_TABLE   - Pointer to vibrato waveform (sine/noise)
 *   0x1E = VIB_POS     - Current vibrato phase position
 *   0x20 = VIB_SPEED   - Vibrato phase advance per tick
 *   0x22 = VIB_DEPTH   - Vibrato amplitude multiplier
 *   0x24 = VIB_WRAP    - Vibrato phase wrap length
 *   0x26 = LOOP_CTR    - Repeat counter for FE loops
 *   0x28 = LOOP_CTR2   - Secondary loop counter
 *
 * ============================================================================
 * ENVELOPE SUB-STREAM FORMAT
 * ============================================================================
 *
 * Separate from the main command stream, each channel can have an
 * "envelope sub-stream" that runs in parallel. It controls volume
 * over time with ADSR-like shaping. Referenced by ENV_BASE + ENV_OFFSET.
 *
 * Format: sequence of 4-byte entries:
 *   [int16 value] [int16 duration]
 *
 *   - If duration != -1: set VOL_DELTA = value, wait 'duration' ticks
 *   - If duration == -1 (0xFFFF): set VOLUME = value (absolute), continue
 *   - If duration == -1 AND value == 0: silence channel, stop envelope
 *
 * ============================================================================
 * PLAYBACK PIPELINE (every tick at ~240 Hz)
 * ============================================================================
 *
 * For each active channel:
 *
 *   1. VOLUME ENVELOPE
 *      volume += volDelta
 *
 *   2. FREQUENCY SLIDE
 *      freq += freqDelta
 *
 *   3. VIBRATO CALCULATION
 *      vibPos += vibSpeed
 *      if (vibPos >= vibWrap) vibPos -= vibWrap
 *      sample = vibTable[vibPos >> 4]  // 256-entry LUT
 *      vibrato = (sample * vibDepth) >> 16  // fixed-point multiply
 *
 *   4. FINAL OUTPUT
 *      outFreq = freq + vibrato
 *
 *   5. MAIN TIMER
 *      ticksLeft--
 *      if (ticksLeft == 0) → load next command block from stream
 *
 *   6. ENVELOPE TIMER
 *      envTicksLeft--
 *      if (envTicksLeft == 0) → consume next envelope entry
 *
 *   7. HARDWARE OUTPUT (not emulated — ScummVM generates PCM instead)
 *      PC Speaker: PIT channel 2 = outFreq, port 61h bits = volume & 3
 *      Tandy PSG: SN76489 freq registers + attenuation
 *
 * ============================================================================
 * SONG ID MAPPING
 * ============================================================================
 *
 * The game calls SOUND_Play(s_id) where:
 *   s_id=0:   Stop all sound, disable speaker
 *   s_id=1:   Enable speaker output (unmute)
 *   s_id=0xFF: Stop songs but keep speaker enabled
 *   s_id=2+:  Play song (s_id - 1) as table index, so song IDs are 1-based
 *
 * The song table has 21 entries (indices 0-20), so valid play IDs are 2-22.
 */

/**
 * Number of sound channels (4 for Tandy, only ch0 used for PC Speaker).
 */
static const int kSoundChannels = 4;

/**
 * Timer frequency: original PIT divisor 0x13B1 = 5041.
 * Actual Hz = 1193182 / 5041 ≈ 236.7 Hz.
 */
static const int kTimerFrequency = 237;

/**
 * Maximum number of songs in the pointer table.
 */
static const int kMaxSongs = 21;

/**
 * Per-channel playback state.
 *
 * This mirrors the original 0x30-byte DOS channel structure.
 * All fields are updated by the tick routine at ~240 Hz.
 */
struct SoundChannel {
    bool active;         ///< Channel is producing sound
    uint16 ticksLeft;    ///< Main countdown; 0 triggers next command load

    // Stream position (indexes into SoundData::streams[])
    uint16 streamId;     ///< Which command stream is playing
    uint16 cmdIndex;     ///< Current command index within that stream

    int16 freq;          ///< Base frequency (PIT divisor value)
    int16 freqDelta;     ///< Frequency change per tick (portamento)
    int16 outFreq;       ///< Final frequency after vibrato

    int16 volume;        ///< Current volume (16-bit, clamped to 0-max)
    int16 volDelta;      ///< Volume change per tick (envelope)

    uint16 delayTimer;   ///< One-shot timer; when 0, resets envelope offset
    uint8 tempoMult;     ///< Duration multiplier (0 or 1 = normal)
    int16 transpose;     ///< Added to note numbers before freq lookup

    // Envelope sub-stream (indexes into SoundData::envelopes[])
    uint16 envStreamId;  ///< Which envelope stream is active
    uint16 envOffset;    ///< Current entry index within that envelope
    uint16 envTicksLeft; ///< Ticks until next envelope entry consumed

    // Vibrato state
    uint16 vibWaveId;    ///< Waveform index (into SoundData::waveforms[])
    uint16 vibPos;       ///< Current phase position (fixed-point)
    uint16 vibSpeed;     ///< Phase advance per tick
    int16 vibDepth;      ///< Amplitude multiplier
    uint16 vibWrap;      ///< Phase wrap-around length

    // Loop control
    uint16 loopCounter;  ///< Repeat count for FE loop commands
    uint16 loopCounter2; ///< Secondary loop counter (nested loops)

    void reset() {
        active = false;
        ticksLeft = 0;
        streamId = 0xFFFF;
        cmdIndex = 0;
        freq = 0;
        freqDelta = 0;
        outFreq = 0;
        volume = 0;
        volDelta = 0;
        delayTimer = 0;
        tempoMult = 0;
        transpose = 0;
        envStreamId = 0xFFFF;
        envOffset = 0;
        envTicksLeft = 0;
        vibWaveId = 0xFFFF;
        vibPos = 0;
        vibSpeed = 0;
        vibDepth = 0;
        vibWrap = 0;
        loopCounter = 0;
        loopCounter2 = 0;
    }
};

/**
 * Sound output mode (matches original SM_ constants).
 */
enum SoundMode {
    kSoundOff = 0,
    kSoundPCSpeaker = 1,
    kSoundTandy = 2
};

/**
 * Goldbox PC Speaker / Tandy sound driver.
 *
 * This class emulates the original timer-interrupt-driven sound player.
 * It reads pre-extracted SoundData structures and generates PCM audio
 * for ScummVM's mixer.
 *
 * Usage:
 *   1. Populate a SoundData struct with extracted game sound resources
 *   2. Construct the driver with a pointer to it
 *   3. Call init() to register with the mixer
 *   4. Call playSong(id) to start playback
 *   5. Call stopAll() to silence
 */
class GoldboxSoundDriver : public Audio::AudioStream {
public:
    /**
     * Construct the sound driver.
     *
     * @param mixer     ScummVM audio mixer
     * @param data      Pre-extracted sound data. Must remain valid for
     *                  the driver's lifetime.
     * @param mode      Sound output mode (Speaker or Tandy)
     */
    GoldboxSoundDriver(Audio::Mixer *mixer, const SoundData *data,
            SoundMode mode);
    ~GoldboxSoundDriver() override;

    /**
     * Initialize the driver and start the audio stream.
     */
    void init();

    /**
     * Play a song by ID (1-based, matching SOUND_Play convention).
     * ID 0 = stop, 1 = unmute, 0xFF = stop songs, 2+ = play song.
     */
    void playSong(uint8 songId);

    /**
     * Stop all sound output.
     */
    void stopAll();

    /**
     * Set speaker enabled/disabled (mute control).
     */
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

    const SoundData *_data;    ///< Extracted sound resources
    SoundMode _mode;           ///< Speaker or Tandy
    bool _speakerEnabled;      ///< Speaker gate (mute control)
    int _outputRate;           ///< PCM output sample rate

    SoundChannel _channels[kSoundChannels];

    /// PCSpeakerStream instances for waveform generation (one per channel)
    Audio::PCSpeakerStream *_spkStreams[kSoundChannels];

    // Timing: fractional accumulator for tick generation
    int _tickAccumulator;

    /// Microseconds per tick (~4220 µs at 237 Hz)
    static const uint32 kTickLengthUs = 1000000 / kTimerFrequency;

    /**
     * Advance the sound engine by one tick (~1/237 second).
     */
    void tick();

    /**
     * Process one tick for a single channel (XT_PROGRAM equivalent).
     */
    void tickChannel(SoundChannel &ch);

    /**
     * Load the next command block (XT_GameSTART equivalent).
     */
    void loadNextCommands(SoundChannel &ch);

    /**
     * Process envelope sub-stream for a channel.
     */
    void processEnvelope(SoundChannel &ch);

    /**
     * Load song data into channels from the appropriate table.
     */
    void loadSong(uint8 songIndex);

    /**
     * Silence all channels.
     */
    void silenceAll();

    /**
     * Queue current channel state to the PCSpeakerStream for one tick.
     */
    void queueChannelAudio(int chIdx);
};

} // namespace Goldbox

#endif // GOLDBOX_SOUND_SOUND_DRIVER_H
