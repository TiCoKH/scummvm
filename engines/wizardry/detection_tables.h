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

#include "wizardry/detection.h"

/*
 * Detection works on the files of the game's UCSD Pascal volume, copied out
 * into a folder (SYSTEM.PASCAL, SCENARIO.DATA, 200.MONSTERS, ...).  Disk
 * images of these releases are not read by the engine.
 *
 * A release is identified by its p-code file, which the game never writes
 * to, together with the native interpreter that came with it.  SCENARIO.DATA
 * is no use as a key: it is the same file on several machines and the game
 * stores the characters in it.
 *
 * TODO: add an extractor to scummvm-tools.  What it has to do is known:
 *  - The directory is at volume block 2 (blocks are 512 bytes): 26-byte
 *    entries holding first block, end block, kind and name.
 *  - IBM-PC Wizardry II-V: 720 KB images in which the volume uses 8 of every
 *    9 sectors; volume block L is image sector L + L / 8.
 *  - FM-7, PC-88 and X1 Wizardry IV, and PC-88 Wizardry V: one volume spread
 *    over 3 or 4 disks.  SYSTEM.RELOC on disk 1 lists, for each disk, runs of
 *    (first volume block, first block on that disk, block count).  Volume
 *    block 0 is at byte 0x2200 (FM-7) or 0x2000 (PC-88, X1) of each disk.
 *
 * TODO: PC-88, FM-7, Sharp X1 and MSX2 releases.  They need platform values
 * that common/platform.h does not have yet.
 */

namespace Wizardry {

const PlainGameDescriptor wizardryGames[] = {
	{ "wizardry1", "Wizardry: Proving Grounds of the Mad Overlord" },
	{ "wizardry2", "Wizardry II: The Knight of Diamonds" },
	{ "wizardry3", "Wizardry III: Legacy of Llylgamyn" },
	{ "wizardry4", "Wizardry IV: The Return of Werdna" },
	{ "wizardry5", "Wizardry V: Heart of the Maelstrom" },
	{ 0, 0 }
};

const WizardryGameDescription gameDescriptions[] = {
	// Apple II
	{
		{
			"wizardry1",
			"Uncompressed",
			AD_ENTRY1s("scenario.data#050000", "2bcbf8ed9bfdbc78375b43df87e7b759", 59392),
			Common::EN_ANY,
			Common::kPlatformApple2,
			ADGF_UNSTABLE,
			GUIO1(GUIO_NONE)
		},
		kWizardry1, kFamilyApple, 0
	},

	{
		{
			"wizardry1",
			"Uncompressed",
			AD_ENTRY1s("scenario.data", "2bcbf8ed9bfdbc78375b43df87e7b759", 59392),
			Common::EN_ANY,
			Common::kPlatformApple2,
			ADGF_UNSTABLE,
			GUIO1(GUIO_NONE)
		},
		kWizardry1, kFamilyApple, 0
	},

	{
		{
			"wizardry1",
			nullptr,
			AD_ENTRY1s("Wizardry-Proving-Grounds-Original-Scenario.dsk", "1cde92e99c4543c8d06e22a2bd61a9ec", 143360),
			Common::EN_ANY,
			Common::kPlatformApple2,
			ADGF_UNSTABLE,
			GUIO1(GUIO_NONE)
		},
		kWizardry1, kFamilyApple, kFeatureDiskImage
	},

	// IBM-PC
	{
		{
			"wizardry1",
			nullptr,
			AD_ENTRY2s("SYSTEM.PASCAL", "8cd18af2774dba85557f7225fdb69c49", 87040,
			           "SYSTEM.INTERP", "2cd04f35e49fb8f67eba35ac46cd012b", 14336),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO2(GUIO_RENDERCGA, GUIO_RENDERCGABW)
		},
		kWizardry1, kFamilyIBM, 0
	},

	{
		{
			"wizardry2",
			nullptr,
			AD_ENTRY2s("SYSTEM.PASCAL", "f4e0b90b6a973e9f8f1dad6dd346a5c9", 87552,
			           "SYSTEM.INTERP", "5cb65cb701488dc2e4c3f7fc3bab3579", 16384),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO2(GUIO_RENDERCGA, GUIO_RENDERCGABW)
		},
		kWizardry2, kFamilyIBM, 0
	},

	{
		{
			"wizardry3",
			nullptr,
			AD_ENTRY2s("SYSTEM.PASCAL", "18c8fb41ce72370c8d3fc7c89025bfaa", 91136,
			           "SYSTEM.INTERP", "5cb65cb701488dc2e4c3f7fc3bab3579", 16384),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO2(GUIO_RENDERCGA, GUIO_RENDERCGABW)
		},
		kWizardry3, kFamilyIBM, 0
	},

	{
		{
			"wizardry4",
			nullptr,
			AD_ENTRY2s("SYSTEM.PASCAL", "cef69c12d79a7d64259005dcff050efd", 83456,
			           "SYSTEM.INTERP", "321f7402bf73f8dcbaf44d6d8fc2e09d", 16384),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO2(GUIO_RENDERCGA, GUIO_RENDERCGABW)
		},
		kWizardry4, kFamilyIBM, 0
	},

	{
		{
			"wizardry5",
			nullptr,
			AD_ENTRY2s("SYSTEM.PASCAL", "b54c4f89942be01bf4e16642fbd8054b", 129024,
			           "SYSTEM.INTERP", "423662f4e666a867a2455f44ae1d7a86", 16384),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO1(GUIO_RENDERCGA)
		},
		kWizardry5, kFamilyIBM, 0
	},

	// NEC PC-98
	{
		{
			"wizardry1",
			nullptr,
			AD_ENTRY2s("PC98.PASCAL", "d8d467b804cc7a0354680674230d8c65", 82432,
			           "SYSTEM.INTERP", "34b7461bbf35fd8a0c934ed865ad2cce", 20480),
			Common::JA_JPN,
			Common::kPlatformPC98,
			ADGF_UNSTABLE,
			GUIO2(GUIO_RENDERPC98_8C, GAMEOPTION_HIRES)
		},
		kWizardry1, kFamilyJapanese, 0
	},

	{
		{
			"wizardry2",
			nullptr,
			AD_ENTRY2s("SYSTEM.PASCAL", "109621fc24ab04a9aa22fc535063e045", 82944,
			           "SYSTEM.INTERP", "a4acfa9c4cfda16dee0cbedf94e64956", 20480),
			Common::JA_JPN,
			Common::kPlatformPC98,
			ADGF_UNSTABLE,
			GUIO2(GUIO_RENDERPC98_8C, GAMEOPTION_HIRES)
		},
		kWizardry2, kFamilyJapanese, 0
	},

	{
		{
			"wizardry3",
			nullptr,
			AD_ENTRY2s("SYSTEM.PASCAL", "b59e3ea633a19d8ee54bc51b0930a970", 90624,
			           "SYSTEM.INTERP", "a4acfa9c4cfda16dee0cbedf94e64956", 20480),
			Common::JA_JPN,
			Common::kPlatformPC98,
			ADGF_UNSTABLE,
			GUIO2(GUIO_RENDERPC98_8C, GAMEOPTION_HIRES)
		},
		kWizardry3, kFamilyJapanese, 0
	},

	{
		{
			"wizardry4",
			nullptr,
			AD_ENTRY2s("SYSTEM.PASCAL", "cad3a173794e06222a03856f0a4b9993", 85504,
			           "SYSTEM.INTERP", "cf07b41f7d3d5b173926d65fdd368698", 20480),
			Common::JA_JPN,
			Common::kPlatformPC98,
			ADGF_UNSTABLE,
			GUIO1(GUIO_RENDERPC98_8C)
		},
		kWizardry4, kFamilyJapanese, 0
	},

	{
		{
			"wizardry5",
			nullptr,
			AD_ENTRY2s("SYSTEM.PASCAL", "f5fb89aef9c1dc2201b18eb0e22aaa56", 131584,
			           "SYSTEM.INTERP", "56e5051e5c96e4431429d9bf334d8892", 22528),
			Common::JA_JPN,
			Common::kPlatformPC98,
			ADGF_UNSTABLE,
			GUIO1(GUIO_RENDERPC98_8C)
		},
		kWizardry5, kFamilyJapanese, 0
	},

	{ AD_TABLE_END_MARKER, 0, 0, 0 }
};

} // End of namespace Wizardry
