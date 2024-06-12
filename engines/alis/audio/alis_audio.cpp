//
// Copyright 2023 Olivier Huguenot, Vadim Kindl
//
// Permission is hereby granted, free of s8ge, to any person obtaining a copy
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

#include "alis/audio/alis_audio.h"

namespace Alis {

AlisAudio::AlisAudio() {
    for (int i = 0; i < 4; i++) {
        channels[i].type = Alis::eChannelType::eChannelTypeNone;
        channels[i].length = 0;
    } ;
}

AlisAudio::~AlisAudio() {
    // Clean up resources
    for (auto stream : _streams) {
        delete stream;
    }
}

void AlisAudio::addChannel(Audio::SoundHandle *handle, const Audio::AudioStream *stream) {
    _channels.push_back(handle);
    _streams.push_back(stream);
}

void AlisAudio::removeChannel(Audio::SoundHandle *handle) {
    for (size_t i = 0; i < _channels.size(); ++i) {
        if (_channels[i] == handle) {
            _channels.remove_at(i);
            _streams.remove_at(i);
            break;
        }
    }
}

} // End of namespace Alis
/*
extern SDL_AudioSpec *_audio_spec;

sChannel channels[4] = {{.type = 0, .length = 0}, {.type = 0, .length = 0}, {.type = 0, .length = 0}, {.type = 0, .length = 0}};
sAudio audio = {};


void playsample(eChannelType type, u8 *address, s8 freq, u8 volume, u32 length, u16 loop)
{
    s32 channelidx = -1;
    for (s32 i = 0; i < 4; i ++)
    {
        if (channels[i].type == eChannelTypeNone)
        {
            channelidx = i;
            break;
        }
    }
    
    if (channelidx >= 0)
    {
        if (freq < 0x1 || 0x14 < freq)
            freq = 10;
        
        channels[channelidx].address = address;
        channels[channelidx].volume = volume;
        channels[channelidx].length = length;
        channels[channelidx].freq = freq * 1000;
        channels[channelidx].loop = loop;
        channels[channelidx].played = 0;
        channels[channelidx].type = type;
    }
}

#define WAVE_FORMAT_IEEE_FLOAT 3

s32 save_wave_file(const char *name, float *data, s32 sample_rate, s32 channel_count, s32 sample_count)
{
    s8 header[58];
    s32 sample_size = sizeof(float);
    s32 data_size = sample_size * channel_count * sample_count;
    s32 pad = data_size & 1;
    memcpy(&header[0], "RIFF", 4);
    *((s32*) (header + 4)) = 50 + data_size + pad;
    memcpy(&header[8], "WAVE", 4);
    memcpy(&header[12], "fmt ", 4);
    *((s32*) (header + 16)) = 18;
    *((s16*) (header + 20)) = WAVE_FORMAT_IEEE_FLOAT;
    *((s16*) (header + 22)) = (s16) channel_count;
    *((s32*) (header + 24)) = sample_rate;
    *((s32*) (header + 28)) = sample_rate * sample_size * channel_count;
    *((s16*) (header + 32)) = (s16) (sample_size * channel_count);
    *((s16*) (header + 34)) = (s16) (8 * sample_size);
    *((s16*) (header + 36)) = 0;
    memcpy(&header[38], "fact", 4);
    *((s32*) (header + 42)) = 4;
    *((s32*) (header + 46)) = channel_count * sample_count;
    memcpy(&header[50], "data", 4);
    *((s32*) (header + 54)) = data_size;
    FILE *f = fopen(name, "wb");
    if (f == NULL)
    {
        return 0;
    }
    
    fwrite(header, 1, sizeof(header), f);
    fwrite(data, 1, data_size, f);
    if (pad)
    {
        fwrite(&pad, 1, 1, f);
    }
    
    fclose(f);
    return 1;
}

void runson(eChannelType type, s8 pereson, s8 priorson, s16 volson, u16 freqson, u16 longson, s16 dvolson, s16 dfreqson)
{
    sChannel *canal;
    
    if (priorson < 0)
    {
        for (int i = 0; i < 2; i++)
        {
            canal = &channels[i];
            if (priorson == canal->curson)
                goto runson10;
        }
        
        canal = &channels[2];
        if (priorson == channels[2].curson)
            goto runson10;
    }
    else
    {
        for (int i = 0; i < 2; i++)
        {
            canal = &channels[i];
            if (pereson == canal->pere && priorson == canal->curson)
                goto runson10;
        }
        
        if (pereson == channels[2].pere)
        {
            canal = &channels[2];
            if (priorson == channels[2].curson)
                goto runson10;

        }
    }
    
    canal = &channels[0];
    char cson = channels[0].curson;
    if (channels[1].curson < channels[0].curson)
    {
        canal = &channels[1];
        cson = channels[1].curson;
    }
    
    if (channels[2].curson < cson)
    {
        canal = &channels[2];
        cson = channels[2].curson;
    }
    
//    if (channels[3].curson < cson)
//    {
//        canal = &channels[3];
//        cson = channels[3].curson;
//    }
    
    if (((-1 < priorson) && (cson < 0)) && (cson != -0x80))
    {
        return;
    }
    
    if (priorson < cson)
    {
        return;
    }
    
runson10:
    
//    if ((s8)type < '\0')
//    {
//        gosound(canal);
//    }
//    else
    {
        canal->state = -0x80;
        canal->curson = priorson;
        canal->type = type;
        canal->freq = freqson;
        canal->delta_freq = dfreqson;
        canal->volume = volson;
        canal->unk0x7 = '\0';
        canal->delta_volume = dvolson;
        canal->length = longson;
        canal->pere = pereson;
        canal->state = 2;
    }
}
*/
