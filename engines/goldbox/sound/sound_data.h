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
 * @brief Gold Box sound data — raw blob approach.
 *
 * The entire sound segment is dumped as a single binary blob starting
 * from a known segment offset. All internal pointers (song tables,
 * waveform refs, envelope refs) are segment-relative near pointers.
 *
 * Since we know the segment base offset, ANY raw pointer in the data
 * resolves to a blob position via: blobOffset = rawPtr - segmentBase.
 *
 * No pre-parsing, no pointer maps, no extraction — just the blob,
 * the segment base, and the song table locations.
 */

/**
 * Number of sound channels (4 for Tandy, only ch0 used for PC Speaker).
 */
static const int kSoundChannels = 4;

/**
 * Number of chromatic notes in the frequency table (one octave).
 */
static const int kNotesPerOctave = 12;

/**
 * Channels per song entry in the pointer table.
 */
static const int kChannelsPerSong = 4;

/**
 * Raw sound data blob with self-resolving pointers.
 *
 * All data lives in this blob. All near pointers found within command
 * streams (waveform refs, envelope refs, jump targets) resolve directly
 * via: offset = pointer - segmentBase.
 */
struct SoundData {
    const byte *data;       ///< Raw binary blob
    uint32 size;            ///< Blob size in bytes
    uint16 segmentBase;     ///< Original segment offset of data[0]

    // Song table locations (segment-relative offsets)
    uint16 speakerTableAddr;  ///< Segment offset of speaker song pointer table
    uint16 tandyTableAddr;    ///< Segment offset of tandy song pointer table
    uint8  songCount;         ///< Number of songs in each table

    // Frequency tables (constant across all Gold Box games)
    uint16 speakerFreqTable[kNotesPerOctave];
    uint16 tandyFreqTable[kNotesPerOctave];

    SoundData()
        : data(nullptr), size(0), segmentBase(0),
          speakerTableAddr(0), tandyTableAddr(0), songCount(0) {
        // Speaker: PIT channel 2 divisors
        static const uint16 kSpk[12] = {
            0x8E84, 0x8684, 0x7EF7, 0x77D7,
            0x714F, 0x6AC4, 0x64C6, 0x5F1E,
            0x59C7, 0x54BD, 0x4FFC, 0x4B7E
        };
        // Tandy: SN76489 divisors (pre-shifted <<2)
        static const uint16 kTdy[12] = {
            0xFFC0, 0xF140, 0xE3C0, 0xD700,
            0xCB40, 0xBF80, 0xB4C0, 0xAA80,
            0xA100, 0x9800, 0x8F80, 0x8740
        };
        memcpy(speakerFreqTable, kSpk, sizeof(speakerFreqTable));
        memcpy(tandyFreqTable, kTdy, sizeof(tandyFreqTable));
    }

    /**
     * Convert a raw segment-relative pointer to a blob offset.
     */
    uint32 toOffset(uint16 rawPtr) const {
        return (uint32)(rawPtr - segmentBase);
    }

    /**
     * Read a byte at segment-relative address.
     */
    byte readByte(uint16 addr) const {
        uint32 off = toOffset(addr);
        if (off >= size) return 0;
        return data[off];
    }

    /**
     * Read a uint16 LE at segment-relative address.
     */
    uint16 readUint16(uint16 addr) const {
        uint32 off = toOffset(addr);
        if (off + 1 >= size) return 0;
        return data[off] | (data[off + 1] << 8);
    }

    /**
     * Get the stream pointer for a given song and channel.
     * Returns the segment-relative address of the command stream,
     * or 0 if the channel is unused.
     */
    uint16 getSongStreamAddr(bool tandy, uint8 songIndex, uint8 ch) const {
        uint16 tableAddr = tandy ? tandyTableAddr : speakerTableAddr;
        uint16 entryAddr = tableAddr + (songIndex * kChannelsPerSong + ch) * 2;
        return readUint16(entryAddr);
    }
};

} // namespace Goldbox

#endif // GOLDBOX_SOUND_SOUND_DATA_H
