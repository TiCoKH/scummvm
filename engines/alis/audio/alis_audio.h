//
// Copyright 2023 Olivier Huguenot, Vadim Kindl
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#ifndef ALIS_AUDIO_H
#define ALIS_AUDIO_H

#include "common/scummsys.h"
#include "common/array.h"
#include "audio/audiostream.h"
#include "audio/mixer.h"

namespace Alis {

typedef enum {
    eChannelTypeNone        = 0,
    eChannelTypeDingZap     = 1,
    eChannelTypeNoise       = 2,
    eChannelTypeExplode     = 3,
    eChannelTypeSample      = 0x80,
} eChannelType;

typedef struct {
    int8 state;
    int8 curson;
    int16 pere;
    eChannelType type;
    uint8 *address;
    uint32 played;
    int16 volume;
    uint8 unk0x7;
    uint32 length;
    int16 freq;
    uint8 loop;

    int16 delta_volume;
    int16 delta_freq;
} sChannel;

typedef struct {
    uint32 address;
    uint32 unknown;
} sAudioInstrument;

typedef struct {
    uint16 data;
    uint16 loop;
    uint32 frqmod;
} sAudioTrkfrq;

typedef struct {
    uint16 freqsam;
    uint32 startsam1;
    uint32 longsam1;
    uint16 volsam;
    uint32 startsam2;
    uint32 longsam2;
    int16 value;
    uint8 type;
    uint8 delta;
    uint16 loopsam;
    uint16 unknown1;
    uint16 unknown2;
    uint16 unknown3;
    
    uint32 sample;
} sAudioVoice;

typedef struct {

    sAudioInstrument tabinst[128];
    uint8 fsound;
    uint8 fmusic;
    uint32 mupnote;
    uint8 muvolume;
    uint16 maxvolume;
    uint8 muvol;
    uint8 mustate;
    uint8 mutempo;
    uint8 mutemp;
    uint16 muattac;
    uint16 muchute;
    int16 muduree;
    uint16 dattac;
    uint16 dchute;
    uint16 muflag;
    uint16 mutaloop;
    int16 muadresse[0xffff];
    uint32 smpidx;

    void (*soundrout)(void);
} sAudio;

class AlisAudio {

public:
    AlisAudio();
    ~AlisAudio();

    void addChannel(Audio::SoundHandle *handle, const Audio::AudioStream *stream);
    void removeChannel(Audio::SoundHandle *handle);

    void playsample(eChannelType type, uint8 *address, int8 freq, uint8 volume, uint32 length, uint16 loop);
    void playsound(eChannelType type, uint8 pereson, uint8 priorson, int16 volson, uint16 freqson, uint16 longson, int16 dvolson, int16 dfreqson);
    void runson(eChannelType type, int8 pereson, int8 priorson, int16 volson, uint16 freqson, uint16 longson, int16 dvolson, int16 dfreqson);

    // older music variant (atari st/amiga ishar and older)

    void mv1_gomusic(void);
    void mv1_offmusic(uint32 much);

    // newer music variant (ishar 1 falcon/dos and later)

    void mv2_gomusic(void);
    void mv2_offmusic(uint32 much);

    // ym

    void io_canal(sChannel *channel, int16 index);

private:
    Common::Array<Audio::SoundHandle *> _channels;
    Common::Array<const Audio::AudioStream *> _streams;
    Alis::sChannel channels[4];
    Alis::sAudio audio;

};

}
#endif //ALIS_AUDIO_H