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

#ifndef GOLDBOX_DATA_CHARACTER_H
#define GOLDBOX_DATA_CHARACTER_H

#include "common/str.h"
#include "common/stream.h"

namespace Goldbox {
namespace Data {

struct Stat {
    uint8 _base;
    uint8 _current;

	operator uint8() const { return _current; }
	Stat &operator=(byte v) {
		_base = _current = v;
		return *this;
	}
	Stat &operator++() {
		if (_base < 255)
			_current = ++_base;
		return *this;
	}
	Stat &operator--() {
		if (_base > 0)
			_current = --_base;
		return *this;
	}
    void set(byte v) { _current = v; }
    uint8 get() { return _current; }
	void clear() { _current = _base = 0; }
	void reset() { _current = _base; }
};

struct CombatStat {
    uint8 _raw_base;
    uint8 _raw_current;

    // Calculate the base value based on the raw base
    int8 calculatedBase() const { return 60 - _raw_base; }

    // Calculate the current value based on the raw current
    int8 calculatedCurrent() const { return 60 - _raw_current; }

    // Implicit conversion to uint8 (returns the raw current value)
    operator uint8() const { return _raw_current; }

    // Assignment operator
    CombatStat &operator=(byte v) {
        _raw_base = _raw_current = v;
        return *this;
    }

    // Pre-increment operator
    CombatStat &operator++() {
        if (_raw_base < 255) {
            _raw_current = ++_raw_base;
        }
        return *this;
    }

    // Pre-decrement operator
    CombatStat &operator--() {
        if (_raw_base > 0) {
            _raw_current = --_raw_base;
        }
        return *this;
    }

    // Set the current value
    void set(byte v) { 
        _raw_current = v; 
    }

    // Get the calculated current value
    int8 get() const { return calculatedCurrent(); }

    // Clear all values to 0
    void clear() { 
        _raw_current = _raw_base = 0; 
    }

    // Reset _current to _base
    void reset() { 
        _raw_current = _raw_base;
    }
};

class Character {
public:
    Character();
    virtual ~Character();

    virtual void loadFrom(Common::SeekableReadStream &stream) = 0;
    virtual Common::String getName();

    // Methods for common actions
    void Damage(uint8 amount);
    void Heal(uint8 amount);
    bool isAlive();
};

}  // End of namespace Data
}  // End of namespace Goldbox

#endif // GOLDBOX_DATA_CHARACTER_H
