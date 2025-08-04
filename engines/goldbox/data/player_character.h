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

#ifndef GOLDBOX_DATA_PLAYER_CHARACTER_H
#define GOLDBOX_DATA_PLAYER_CHARACTER_H

#include "common/str.h"
#include "common/stream.h"
#include "common/array.h"
#include "common/util.h"

namespace Goldbox {
namespace Data {

//---------------------------------------------------------------
// Helper structures

enum Gender {
	G_MALE   = 0,
	G_FEMALE = 1
};

struct Stat {
	uint8 base;    // Original stat
	uint8 current; // Modified stat

	Stat() : base(0), current(0) {}
};

struct CombatStat {
	uint8 base;
	uint8 current;

	CombatStat() : base(0), current(0) {}
};

struct StatHP {
	uint8 max;
	uint8 current;

	StatHP() : max(0), current(0) {}
};

struct AbilityScores {
	Stat strength;
	Stat intelligence;
	Stat wisdom;
	Stat dexterity;
	Stat constitution;
	Stat charisma;
	Stat strException;
};

struct PortraitData {
	uint8 head;
	uint8 body;

	PortraitData() : head(0), body(0) {}
};

struct CombatIconData {
	uint8 iconHead;
	uint8 iconBody;
	uint8 iconSize;
	uint8 iconColorBody1, iconColorBody2;
	uint8 iconColorArm1, iconColorArm2;
	uint8 iconColorLeg1, iconColorLeg2;
	uint8 iconColorHair, iconColorFace;
	uint8 iconColorShield1, iconColorShield2;
	uint8 iconColorWeapon1, iconColorWeapon2;


	void setBody(uint8 value) {
		iconColorBody1 = value >> 4;
		iconColorBody2 = value & 0x0F;
	}

	void setArm(uint8 value) {
		iconColorArm1 = value >> 4;
		iconColorArm2 = value & 0x0F;
	}

	void setLeg(uint8 value) {
		iconColorLeg1 = value >> 4;
		iconColorLeg2 = value & 0x0F;
	}

	void setHairFace(uint8 value) {
		iconColorHair = value >> 4;
		iconColorFace = value & 0x0F;
	}

	void setShield(uint8 value) {
		iconColorShield1 = value >> 4;
		iconColorShield2 = value & 0x0F;
	}

	void setWeapon(uint8 value) {
		iconColorWeapon1 = value >> 4;
		iconColorWeapon2 = value & 0x0F;
	}
	
};

//---------------------------------------------------------------
// Base class for all player characters

class PlayerCharacter {

public:
	// Identity & Metadata
	Common::String name;
	uint8 race;
	Gender gender;
	uint8 classType;
	uint8 alignment;
	uint16 age;
	AbilityScores abilities;

	StatHP hitPoints;

	PortraitData portrait;

	CombatStat thac0;
	CombatStat armorClass;
	CombatIconData iconData;
	uint8 strengthBonusAllowed = 0;

	uint8 orderNumber;

	uint8 iconDimension;

	Stat movement;

	// Experience & Level
	uint32 experiencePoints = 0;

	// State
	uint8 healthStatus = 0;
	bool inCombat = false;

	//-----------------------------------------------------------
	// Lifecycle

	virtual ~PlayerCharacter();

	virtual const char *getRaceName() const = 0;
	virtual const char *getClassName() const = 0;
	virtual const char *getGenderName() const = 0;
	virtual const char *getAlignmentName() const = 0;
	virtual const char *getStatusName() const = 0;

	//-----------------------------------------------------------
	// Game-specific I/O
	virtual void load(Common::SeekableReadStream &stream) = 0;
	virtual void save(Common::WriteStream &stream) = 0;

	//-----------------------------------------------------------
	// Game-specific behaviors
	virtual void rollAbilityScores() = 0;
	virtual void applyRacialAdjustments() = 0;
	virtual void calculateHitPoints() = 0;

	//-----------------------------------------------------------
	// Common logic

	virtual void damage(uint8 amount);
	virtual void heal(uint8 amount);
	virtual bool isAlive() const;
	int8 getReactionAdjustment() const;
	uint8 getStrengthTier() const;
	int8 getStrengthBonus() const;
	int8 getMeleeDamageBonus() const;
	int8 getDexDefenceBonus() const;
};

} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_PLAYER_CHARACTER_H
