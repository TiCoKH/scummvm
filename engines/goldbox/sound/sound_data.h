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

#ifndef GOLDBOX_SOUND_SOUND_DATA_H
#define GOLDBOX_SOUND_SOUND_DATA_H

#include "common/scummsys.h"
#include "common/array.h"

namespace Goldbox {

/**
 * @file sound_data.h
 * @brief Internal sound data structures for Gold Box games.
 *
 * These structures hold the manually extracted sound data in a format
 * suitable for the ScummVM sound driver. The data originates from the
 * DOS executable's embedded sound segment but is stored here in a
 * clean, portable representation.
 *
 * ========================================================================
 * HOW THE ORIGINAL STORES DATA
 * ========================================================================
 *
 * In the DOS EXE, a single flat memory segment contains:
 *   - A frequency lookup table (12 notes)
 *   - Two song pointer tables (Tandy + Speaker, 21 songs × 4 channels)
 *   - Vibrato waveform LUTs (sine, noise, etc.)
 *   - Variable-length command streams for all songs/effects
 *
 * All pointers in the tables are segment-relative offsets into this same
 * block. Our internal representation replaces raw offsets with indices
 * into arrays, making the data fully self-contained and relocatable.
 *
 * ========================================================================
 * HOW THIS STRUCT IS USED
 * ========================================================================
 *
 * 1. At build time (or from a resource file), populate SoundData with
 *    the extracted tables and streams.
 * 2. Pass SoundData to the GoldboxSoundDriver.
 * 3. The driver indexes into streams[] by ID, reads commands, and
 *    generates audio.
 *
 * The driver never touches raw EXE data — all access goes through these
 * structures.
 */

/**
 * Number of chromatic notes in the frequency table (one octave).
 */
static const int kNotesPerOctave = 12;

/**
 * Maximum songs in a pointer table.
 */
static const int kMaxSongCount = 21;

/**
 * Channels per song entry.
 */
static const int kChannelsPerSong = 4;

/**
 * Vibrato waveform LUT size (samples).
 */
static const int kWaveformSize = 256;

/**
 * Stream ID indicating "no stream" / silence.
 */
static const uint16 kStreamIdNone = 0xFFFF;

/**
 * A single command in a sound stream.
 *
 * Each command writes a value to a channel register, or performs
 * control flow (loop, jump, end).
 */
struct SoundCommand {
    /**
     * Command type — determines how the driver interprets this entry.
     */
    enum Type : uint8 {
        REG_WRITE,   ///< Write value to a channel register
        LOOP_START,  ///< Set loop counter (value = count)
        LOOP_BACK,   ///< Decrement counter, jump if > 0 (value = target cmd index)
        JUMP,        ///< Unconditional jump (value = target cmd index)
        COND_JUMP,   ///< Conditional jump (value = target cmd index)
        END          ///< End of stream
    };

    /**
     * Register IDs for REG_WRITE commands.
     * These map to channel state fields.
     */
    enum Register : uint8 {
        REG_DURATION    = 0x00,  ///< Ticks until next command block
        REG_FREQ        = 0x04,  ///< Base frequency (PIT divisor)
        REG_FREQ_DELTA  = 0x06,  ///< Frequency slide per tick
        REG_VOLUME      = 0x0A,  ///< Current volume level
        REG_VOL_DELTA   = 0x0C,  ///< Volume change per tick
        REG_DELAY_TIMER = 0x0E,  ///< One-shot envelope reset timer
        REG_TEMPO       = 0x10,  ///< Duration multiplier
        REG_TRANSPOSE   = 0x12,  ///< Note number offset
        REG_ENV_STREAM  = 0x16,  ///< Envelope sub-stream ID (index into streams[])
        REG_VIB_WAVE    = 0x1C,  ///< Vibrato waveform ID (index into waveforms[])
        REG_VIB_POS     = 0x1E,  ///< Vibrato phase position
        REG_VIB_SPEED   = 0x20,  ///< Vibrato phase speed
        REG_VIB_DEPTH   = 0x22,  ///< Vibrato amplitude
        REG_VIB_WRAP    = 0x24,  ///< Vibrato phase wrap length
        REG_LOOP_CTR    = 0x26,  ///< Primary loop counter
        REG_LOOP_CTR2   = 0x28   ///< Secondary loop counter
    };

    Type type;          ///< What this command does
    uint8 reg;          ///< Register ID (for REG_WRITE)
    int16 value;        ///< Value to write, or jump target index
    uint16 loopTarget;  ///< For LOOP_BACK: command index to jump to

    SoundCommand() : type(END), reg(0), value(0), loopTarget(0) {}

    static SoundCommand makeRegWrite(uint8 r, int16 val) {
        SoundCommand cmd;
        cmd.type = REG_WRITE;
        cmd.reg = r;
        cmd.value = val;
        return cmd;
    }

    static SoundCommand makeEnd() {
        SoundCommand cmd;
        cmd.type = END;
        return cmd;
    }

    static SoundCommand makeLoopStart(uint16 count) {
        SoundCommand cmd;
        cmd.type = LOOP_START;
        cmd.value = (int16)count;
        return cmd;
    }

    static SoundCommand makeLoopBack(uint16 targetCmdIndex) {
        SoundCommand cmd;
        cmd.type = LOOP_BACK;
        cmd.loopTarget = targetCmdIndex;
        return cmd;
    }

    static SoundCommand makeJump(uint16 targetCmdIndex) {
        SoundCommand cmd;
        cmd.type = JUMP;
        cmd.loopTarget = targetCmdIndex;
        return cmd;
    }
};

/**
 * A single envelope entry (volume shaping over time).
 *
 * The envelope sub-stream runs in parallel with the main command stream
 * and controls volume slope independently of note timing.
 */
struct EnvelopeEntry {
    /**
     * Entry type.
     */
    enum Type : uint8 {
        SLOPE,     ///< Set volDelta = value, wait duration ticks
        SET_VOL,   ///< Set volume = value immediately, continue
        SILENCE    ///< Set volume = 0, stop envelope
    };

    Type type;
    int16 value;      ///< Volume delta (SLOPE) or absolute volume (SET_VOL)
    uint16 duration;  ///< Ticks to hold this slope (SLOPE only)

    EnvelopeEntry() : type(SILENCE), value(0), duration(0) {}

    static EnvelopeEntry makeSlope(int16 delta, uint16 dur) {
        EnvelopeEntry e;
        e.type = SLOPE;
        e.value = delta;
        e.duration = dur;
        return e;
    }

    static EnvelopeEntry makeSetVol(int16 vol) {
        EnvelopeEntry e;
        e.type = SET_VOL;
        e.value = vol;
        return e;
    }

    static EnvelopeEntry makeSilence() {
        EnvelopeEntry e;
        e.type = SILENCE;
        return e;
    }
};

/**
 * A command stream — the main playback program for one channel.
 *
 * This is an ordered list of commands that the driver executes
 * sequentially. When a DURATION register write is encountered,
 * the driver pauses for that many ticks before continuing.
 */
struct SoundStream {
    uint16 id;                          ///< Unique stream ID
    Common::Array<SoundCommand> commands; ///< Command sequence
};

/**
 * An envelope stream — parallel volume shaping for one channel.
 */
struct EnvelopeStream {
    uint16 id;                            ///< Unique stream ID
    Common::Array<EnvelopeEntry> entries;  ///< Entry sequence
};

/**
 * A vibrato waveform lookup table.
 */
struct Waveform {
    uint16 id;                  ///< Unique waveform ID
    uint8 samples[kWaveformSize]; ///< 256 signed 8-bit samples
};

/**
 * One song definition — maps song ID to channel streams.
 */
struct SongDefinition {
    uint16 channelStream[kChannelsPerSong]; ///< Stream ID per channel (kStreamIdNone = unused)

    SongDefinition() {
        for (int i = 0; i < kChannelsPerSong; ++i)
            channelStream[i] = kStreamIdNone;
    }
};

/**
 * Complete sound data for one game.
 *
 * This is the top-level container holding all extracted sound resources.
 * Populate this from the DOS executable data, then pass to the driver.
 */
struct SoundData {
    /**
     * Frequency table: 12 PIT divisor values for octave 0.
     * Index by (noteNumber % 12), shift right by (noteNumber / 12).
     */
    uint16 freqTable[kNotesPerOctave];

    /**
     * Song definitions for Tandy mode.
     * Index: 0-based song number (game uses 1-based, subtract 1).
     */
    Common::Array<SongDefinition> tandySongs;

    /**
     * Song definitions for PC Speaker mode.
     */
    Common::Array<SongDefinition> speakerSongs;

    /**
     * All command streams, indexed by stream ID.
     * Songs reference these by ID in their channelStream[] fields.
     */
    Common::Array<SoundStream> streams;

    /**
     * All envelope streams, indexed by stream ID.
     * Referenced by REG_ENV_STREAM commands in the main streams.
     */
    Common::Array<EnvelopeStream> envelopes;

    /**
     * Vibrato waveform tables.
     * Referenced by REG_VIB_WAVE commands.
     */
    Common::Array<Waveform> waveforms;

    SoundData() {
        memset(freqTable, 0, sizeof(freqTable));
    }
};

} // namespace Goldbox

#endif // GOLDBOX_SOUND_SOUND_DATA_H
