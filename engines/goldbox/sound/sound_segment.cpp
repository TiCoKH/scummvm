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

#include "goldbox/sound/sound_segment.h"
#include "common/endian.h"
#include "common/file.h"
#include "common/textconsole.h"

namespace Goldbox {

namespace {

const uint32 kSoundDatMagic = MKTAG('G', 'B', 'S', 'D');
const uint16 kSoundDatVersion = 1;
const uint32 kSoundDatHeaderSize = 26;
const uint16 kFlagNoteAnyChannel = 0x01;
const uint16 kFlagSafeReturn = 0x02;
const uint16 kFlagNoteDurationTable = 0x04;
const uint16 kNoteDurationTableOffset = 0x00BA;
const uint16 kNoteDurationTableSize = 19;
const uint32 kChannelBankSize = 8 * 0x30;
const uint32 kFreqTableSize = 12 * 2;

bool fits(uint32 offset, uint32 size) {
    return offset + size <= SoundSegment::kSegmentSize;
}

} // End of anonymous namespace

SoundSegment::SoundSegment() : _loaded(false) {
}

bool SoundSegment::loadFromFile(const Common::Path &fileName) {
    _loaded = false;

    Common::File f;
    if (!f.open(fileName)) {
        warning("Sound: cannot open %s", fileName.toString().c_str());
        return false;
    }
    return loadFromStream(f, fileName.toString().c_str());
}

bool SoundSegment::loadFromStream(Common::SeekableReadStream &f,
        const char *name) {
    _loaded = false;

    if (f.size() < (int64)kSoundDatHeaderSize ||
            f.readUint32BE() != kSoundDatMagic) {
        warning("Sound: %s is not a sound data file", name);
        return false;
    }

    uint16 version = f.readUint16LE();
    if (version != kSoundDatVersion) {
        warning("Sound: %s has unsupported version %d",
            name, version);
        return false;
    }

    uint16 flags = f.readUint16LE();
    SoundSegmentLayout layout;
    layout.numSounds = f.readUint16LE();
    layout.speakerFreq = f.readUint16LE();
    layout.speakerChannels = f.readUint16LE();
    layout.speakerSounds = f.readUint16LE();
    layout.tandyFreq = f.readUint16LE();
    layout.tandyChannels = f.readUint16LE();
    layout.tandySounds = f.readUint16LE();
    layout.safeReturn = (flags & kFlagSafeReturn) != 0;
    layout.noteDurationTable = (flags & kFlagNoteDurationTable) != 0;
    layout.noteAnyChannel = (flags & kFlagNoteAnyChannel) != 0 ||
        layout.noteDurationTable;
    uint16 dataStart = f.readUint16LE();
    uint16 dataLength = f.readUint16LE();

    uint32 soundTableSize = (uint32)layout.numSounds * 8;
    if (layout.numSounds == 0 || !fits(dataStart, dataLength) ||
            (layout.noteDurationTable &&
                (dataStart > kNoteDurationTableOffset ||
                    dataStart + dataLength <
                        kNoteDurationTableOffset + kNoteDurationTableSize)) ||
            f.size() - f.pos() < dataLength ||
            !fits(layout.speakerFreq, kFreqTableSize) ||
            !fits(layout.tandyFreq, kFreqTableSize) ||
            !fits(layout.speakerChannels, kChannelBankSize) ||
            !fits(layout.tandyChannels, kChannelBankSize) ||
            !fits(layout.speakerSounds, soundTableSize) ||
            !fits(layout.tandySounds, soundTableSize)) {
        warning("Sound: %s has an invalid layout", name);
        return false;
    }

    // Data keeps its original segment offsets so near pointers stay valid
    _mem.clear();
    _mem.resize(kSegmentSize);
    memset(_mem.data(), 0, kSegmentSize);
    if (f.read(_mem.data() + dataStart, dataLength) != dataLength) {
        warning("Sound: %s is truncated", name);
        return false;
    }

    _layout = layout;
    _loaded = true;
    return true;
}

} // End of namespace Goldbox
