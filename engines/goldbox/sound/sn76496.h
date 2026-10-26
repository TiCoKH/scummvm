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

#ifndef GOLDBOX_SOUND_SN76496_H
#define GOLDBOX_SOUND_SN76496_H

#include "common/scummsys.h"

namespace Goldbox {

/**
 * Register-level SN76496 compatible PSG (Tandy 1000 / PCjr sound chip).
 *
 * Three square wave tone channels and one noise channel, driven by raw
 * register writes to port 0xC0. Rendering box-filters the chip output
 * at its internal step rate (clock / 16) down to the output rate.
 */
class Sn76496 {
public:
    static const uint32 kClock = 3579545;

    Sn76496();

    void reset();

    /** Write a byte to the chip's data port. */
    void write(byte data);

    /** Produce the next mono output sample. */
    int16 nextSample(uint32 outputRate);

private:
    uint16 _tone[3];
    uint8 _noiseControl;
    uint8 _attenuation[4];
    uint8 _latch;

    uint16 _counter[4];
    bool _toneHigh[3];
    bool _noiseFlip;
    uint16 _lfsr;
    uint32 _stepAccumulator;

    void step();
    int32 currentLevel() const;
    uint16 noisePeriod() const;
};

} // End of namespace Goldbox

#endif // GOLDBOX_SOUND_SN76496_H
