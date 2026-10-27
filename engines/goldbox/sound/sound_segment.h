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

#ifndef GOLDBOX_SOUND_SOUND_SEGMENT_H
#define GOLDBOX_SOUND_SOUND_SEGMENT_H

#include "common/scummsys.h"
#include "common/array.h"
#include "common/path.h"
#include "common/stream.h"

namespace Goldbox {

/**
 * Addresses (segment-relative offsets) of the sound driver tables and
 * channel banks, as stored in the sound.dat header.
 */
struct SoundSegmentLayout {
    uint16 speakerFreq;     ///< ARRAY_SPEAKER_FREQ_BASE (PIT divisors)
    uint16 speakerChannels; ///< ARRAY_SPEAKER_CHANNEL (8 x 0x30 bytes)
    uint16 speakerSounds;   ///< ARRAY_SOUNDS_SPEAKER (4 stream ptrs/sound)
    uint16 tandyFreq;       ///< ARRAY_TANDY_FREQ_BASE (tone periods << 6)
    uint16 tandyChannels;   ///< ARRAY_TANDY_CHANNEL (8 x 0x30 bytes)
    uint16 tandySounds;     ///< ARRAY_SOUNDS_TANDY (4 stream ptrs/sound)
    uint16 numSounds;       ///< Number of entries in each sound table

    /**
     * Note events address any channel of the bank and subtract the target
     * channel's +0x10 gap from the note length. The duration-table format
     * also enables this behavior.
     */
    bool noteAnyChannel;

    /** Note-event low bits index the duration table at segment offset 0xBA. */
    bool noteDurationTable;

    /** FB (return) clears the return address and ends on a null return. */
    bool safeReturn;

    SoundSegmentLayout() : speakerFreq(0), speakerChannels(0),
        speakerSounds(0), tandyFreq(0), tandyChannels(0), tandySounds(0),
        numSounds(0), noteAnyChannel(false), noteDurationTable(false),
        safeReturn(false) {}
};

/**
 * The original DOS sound driver data segment.
 *
 * The driver's data segment (sound tables, waveforms, envelopes, command
 * streams and channel banks) is loaded from sound.dat, which is built by
 * devtools/create_goldbox/poolrad/create_sound_dat.py. Data is kept as a
 * flat 64 KB image at its original offsets so that all near pointers used
 * by the original driver resolve directly as offsets into it.
 */
class SoundSegment {
public:
    static const uint32 kSegmentSize = 0x10000;

    SoundSegment();

    /**
     * Load the sound segment from a sound.dat file.
     * @return true on success
     */
    bool loadFromFile(const Common::Path &fileName);

    /**
     * Load the sound segment from a stream with sound.dat contents.
     * @param name  name used in warnings
     * @return true on success
     */
    bool loadFromStream(Common::SeekableReadStream &stream, const char *name);

    bool isLoaded() const { return _loaded; }
    const SoundSegmentLayout &getLayout() const { return _layout; }

    /** Mutable segment image (the driver keeps channel state in it). */
    byte *data() { return _mem.data(); }
    const byte *data() const { return _mem.data(); }

private:
    Common::Array<byte> _mem;
    SoundSegmentLayout _layout;
    bool _loaded;
};

} // End of namespace Goldbox

#endif // GOLDBOX_SOUND_SOUND_SEGMENT_H
