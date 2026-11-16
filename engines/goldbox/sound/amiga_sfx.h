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

#ifndef GOLDBOX_SOUND_AMIGA_SFX_H
#define GOLDBOX_SOUND_AMIGA_SFX_H

#include "audio/mods/paula.h"
#include "common/array.h"
#include "common/path.h"

namespace Goldbox {

/**
 * The sound effect driver of the Amiga version (SOUND_Play, PAULA_Setup,
 * PAULA_Init, PAULA_Play, PAULA_Set; 0x289522-0x28991a of the program).
 * SoundCommand(id) starts effect id on channel 0; each vertical blank the
 * effect runs its mode (pitch sweep or trill), its duration and repeats,
 * then decays. The data is the effect section of poolradamigasound.dat.
 */
class AmigaSfx : public Audio::Paula {
public:
	AmigaSfx();
	~AmigaSfx() override {}

	/** Load the effects of poolradamigasound.dat; false if missing. */
	bool load(const Common::Path &datPath);

	/** SOUND_Play: start an effect on channel 0. */
	void playEffect(int effect);
	/** Silence all effects (the original stops them for music). */
	void stopEffects();

protected:
	void interrupt() override;

private:
	struct Effect {
		uint8 sample, pitch, param, volume, mode, duration, decay, repeats;
	};
	struct Sample {
		uint32 offset;
		uint16 length, repeatOffset, repeatLength;
		bool loop;
	};
	struct Voice {
		uint8 sample = 0;
		uint8 param = 0;
		int8 volume = 0;
		uint8 mode = 0;
		uint8 duration = 0;
		uint8 durationReload = 0;
		uint8 decay = 0;
		int8 repeats = 0;
		bool forever = false;
		bool active = false;
		bool toggle = false;
		bool loopSet = false;
		uint16 period = 0;
		int request = -1;
	};

	void startVoice(int ch, int effect);
	void runVoice(int ch);
	void writeHardware();
	uint16 period(uint8 index) const;

	Common::Array<uint16> _periods;
	Common::Array<Effect> _effects;
	Common::Array<Sample> _samples;
	Common::Array<int8> _sampleData;
	Voice _voices[4];
	uint8 _dmaOff = 0x0F;      // 0x289a18: channels whose DMA stays off
	bool _dmaOn[4] = {};
	bool _loaded = false;
};

} // namespace Goldbox

#endif
