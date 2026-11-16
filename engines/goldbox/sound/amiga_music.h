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

#ifndef GOLDBOX_SOUND_AMIGA_MUSIC_H
#define GOLDBOX_SOUND_AMIGA_MUSIC_H

#include "audio/mods/paula.h"
#include "common/array.h"
#include "common/path.h"

namespace Goldbox {

/**
 * The music player of the Amiga version ("(c)Wally Beben 1988 music on the
 * Amiga"), code 0x260728-0x2611a7 of the game program. Its songs, samples
 * and state are in hunk 28 of the program, extracted to
 * poolradamigasound.dat; the player works on a copy of that hunk at the
 * offsets the 68000 code uses, called at 50 Hz like the vertical blank
 * server.
 *
 * Songs 1-6: 1 is the title music, played over the intro.
 */
class AmigaMusic : public Audio::Paula {
public:
	AmigaMusic();
	~AmigaMusic() override {}

	/** Load poolradamigasound.dat; false if it is missing or unknown. */
	bool load(const Common::Path &datPath);

	/** PlayMusic: start a song (1-6), or stop with 0. */
	void playSong(int song);

protected:
	void interrupt() override;

private:
	// Memory of the music hunk, at the hunk-relative offsets.
	uint8 b(uint32 a) const;
	void sb(uint32 a, uint8 v);
	uint16 w(uint32 a) const;
	void sw(uint32 a, uint16 v);
	uint32 l(uint32 a) const;
	void sl(uint32 a, uint32 v);
	uint8 f(uint field, int ch) const;
	void sf(uint field, int ch, uint8 v);

	void tick();
	void startSong(int song);
	void stopVoices();
	bool playTick();
	bool channel(int ch);
	bool newNote(int ch);
	void nextPattern(int ch);
	void buildArpeggio(int ch);
	void setInstrument(int ch);
	void setEnvelope(int ch);
	void notePeriod(int ch);
	void effects(int ch);
	void slideSetup(int ch, uint8 note);
	void vibrato(int ch);
	void envelope(int ch);
	void writeHardware();
	void setLocation(int ch, uint32 loc, uint16 lengthWords);

	Common::Array<uint8> _mem;
	bool _loaded = false;
	int _songCount = 0;
	int _request = 0;
	// Self-modified immediates of the portamento code (0x260f7d, 0x260d1b).
	uint8 _slideTarget = 0;
	uint8 _slideNibble = 0;
	bool _dmaOn[4] = {};
};

} // namespace Goldbox

#endif
