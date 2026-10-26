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

#include "goldbox/sound/sn76496.h"
#include "common/util.h"

namespace Goldbox {

namespace {

// 2 dB per attenuation step; step 15 is off
const int16 kVolumeTable[16] = {
    8000, 6355, 5048, 4009, 3185, 2530, 2009, 1596,
    1268, 1007, 800, 635, 505, 401, 318, 0
};

const uint16 kLfsrReset = 0x4000;

} // End of anonymous namespace

Sn76496::Sn76496() {
    reset();
}

void Sn76496::reset() {
    for (int i = 0; i < 3; ++i) {
        _tone[i] = 0;
        _toneHigh[i] = false;
    }
    for (int i = 0; i < 4; ++i) {
        _attenuation[i] = 15;
        _counter[i] = 1;
    }
    _noiseControl = 0;
    _latch = 0;
    _noiseFlip = false;
    _lfsr = kLfsrReset;
    _stepAccumulator = 0;
}

void Sn76496::write(byte data) {
    uint8 reg = _latch;
    if (data & 0x80) {
        reg = (data >> 4) & 7;
        _latch = reg;
    }

    const uint8 channel = reg >> 1;
    if (reg & 1) {
        _attenuation[channel] = data & 0x0F;
    } else if (channel < 3) {
        if (data & 0x80)
            _tone[channel] = (_tone[channel] & 0x3F0) | (data & 0x0F);
        else
            _tone[channel] = (_tone[channel] & 0x0F) | ((data & 0x3F) << 4);
    } else {
        // Any write to the noise register restarts the shift register
        _noiseControl = data & 7;
        _lfsr = kLfsrReset;
    }
}

uint16 Sn76496::noisePeriod() const {
    switch (_noiseControl & 3) {
    case 0:
        return 0x10;
    case 1:
        return 0x20;
    case 2:
        return 0x40;
    default:
        return _tone[2] ? _tone[2] : 0x400;
    }
}

void Sn76496::step() {
    for (int i = 0; i < 3; ++i) {
        if (--_counter[i] == 0) {
            _counter[i] = _tone[i] ? _tone[i] : 0x400;
            _toneHigh[i] = !_toneHigh[i];
        }
    }

    if (--_counter[3] == 0) {
        _counter[3] = noisePeriod();
        _noiseFlip = !_noiseFlip;
        if (_noiseFlip) {
            uint16 feedback;
            if (_noiseControl & 4)
                feedback = (_lfsr ^ (_lfsr >> 1)) & 1;
            else
                feedback = _lfsr & 1;
            _lfsr = (_lfsr >> 1) | (feedback << 14);
        }
    }
}

int32 Sn76496::currentLevel() const {
    int32 level = 0;
    for (int i = 0; i < 3; ++i) {
        const int16 vol = kVolumeTable[_attenuation[i]];
        level += _toneHigh[i] ? vol : -vol;
    }
    const int16 noiseVol = kVolumeTable[_attenuation[3]];
    level += (_lfsr & 1) ? noiseVol : -noiseVol;
    return level;
}

int16 Sn76496::nextSample(uint32 outputRate) {
    const uint32 threshold = 16 * outputRate;
    int32 sum = 0;
    int32 steps = 0;

    _stepAccumulator += kClock;
    while (_stepAccumulator >= threshold) {
        _stepAccumulator -= threshold;
        step();
        sum += currentLevel();
        ++steps;
    }

    const int32 level = steps ? sum / steps : currentLevel();
    return (int16)CLIP<int32>(level, -32767, 32767);
}

} // End of namespace Goldbox
