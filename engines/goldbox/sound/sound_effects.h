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

#ifndef GOLDBOX_SOUND_SOUND_EFFECTS_H
#define GOLDBOX_SOUND_SOUND_EFFECTS_H

#include "common/scummsys.h"

namespace Goldbox {

/**
 * Sound effects by index into the original GDConsts.soundIds table
 * (ARRAY_SOUND_MAP); Engine::soundPlay() takes the same values.
 */
enum SoundEffect {
	kSfxStop = 0,
	kSfxSpellCast = 3,      ///< Default spell projectile
	kSfxDamage = 4,         ///< Damage flash
	kSfxEffect = 5,         ///< Spell effect flash
	kSfxDowned = 6,
	kSfxRangedStart = 7,
	kSfxImpact = 8,         ///< Weapon hit, wall bump
	kSfxLightning = 9,
	kSfxMiss = 10,
	kSfxStep = 11,          ///< Party/creature step and turn
	kSfxFireball = 12,      ///< Also the ECL sound cue
	kSfxRangedEnd = 13,
	// v1.0 only, played only in Tandy mode; v1.3 dropped both tunes and
	// its table entries 12 and 13 are sound effects.
	kSfxTitleMusicV10 = 14, ///< VIEW_Title, Tandy song 12
	kSfxCombatMusicV10 = 15 ///< Combat setup (2D5F:1ED6), Tandy song 13
};

/** "sound_tempo" range and default, in percent of the original rate. */
static const int kMinSoundTempo = 50;
static const int kMaxSoundTempo = 150;
static const int kDefaultSoundTempo = 100;

/** Original per-command auto-stop value (SoundSetAutoStop(10)). */
static const uint16 kSoundAutoStop = 10;

/** ECL sndEffect ($3DE) values understood by the $BA03 sound call. */
static const uint16 kEclSoundStep = 8;
static const uint16 kEclSoundCue = 10;

} // namespace Goldbox

#endif // GOLDBOX_SOUND_SOUND_EFFECTS_H
