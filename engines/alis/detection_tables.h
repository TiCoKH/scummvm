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

namespace Alis {

static const AlisGameDescription GAME_DESCRIPTIONS[] = {
#ifdef ENABLE_ALIS
	{
		// Ishar: Legend of the Fortress Atari ST
		{
			"ishar1",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR1,
		0
	},

	{
		// Ishar: Legend of the Fortress Atari Falcon
		{
			"ishar1",
			0,
			AD_ENTRY2s("main.fo", "4f7de68f6689cf9617aa1ea03240137e", 27188,
					   "map.fo", "f99633a0110ccf90837ab161be56cf1c", 13006),
			Common::EN_ANY,
			Common::kPlatformAtariFalcon,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR1,
		0
	},

	{
		// Ishar: Legend of the Fortress Amiga OCS
		{
			"ishar1",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR1,
		0
	},

	{
		// Ishar: Legend of the Fortress Amiga AGA
		{
			"ishar1",
			0,
			AD_ENTRY2s("main.do", "37aa42ad4279a40be703883645d3793e", 27188,
					   "map.do", "5842cc60d0bf1ecc4e473db1579afc96", 13006),
			Common::EN_ANY,
			Common::kPlatformAmigaAGA,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR1,
		0
	},

	{
		// Ishar: Legend of the Fortress PC MS-DOS
		{
			"ishar1",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR1,
		0
	},

	{
		// Ishar: Legend of the Fortress Macintosh Color
		{
			"ishar1",
			0,
			AD_ENTRY2s("main.fo", "37aa42ad4279a40be703883645d3793e", 29952,
					   "map.fo", "5842cc60d0bf1ecc4e473db1579afc96", 15872),
			Common::EN_ANY,
			Common::kPlatformMacintoshII,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR1,
		0
	},

	{
		// Ishar: Legend of the Fortress Macintosh BW
		{
			"ishar1",
			0,
			AD_ENTRY2s("main.mo", "37aa42ad4279a40be703883645d3793e", 24146,
					   "map.mo", "5842cc60d0bf1ecc4e473db1579afc96", 8190),
			Common::EN_ANY,
			Common::kPlatformMacintosh,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR1,
		0
	},

	{
		// Ishar 2: Messengers of Doom Atari ST
		{
			"ishar2",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR2,
		0
	},

	{
		// Ishar 2: Messengers of Doom Atari Falcon
		{
			"ishar2",
			0,
			AD_ENTRY2s("main.fo", "4f7de68f6689cf9617aa1ea03240137e", 27188,
					   "map.fo", "f99633a0110ccf90837ab161be56cf1c", 13006),
			Common::EN_ANY,
			Common::kPlatformAtariFalcon,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR2,
		0
	},

	{
		// Ishar 2: Messengers of Doom Amiga OCS
		{
			"ishar2",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR2,
		0
	},

	{
		// Ishar 2: Messengers of Doom Amiga AGA
		{
			"ishar2",
			0,
			AD_ENTRY2s("main.do", "37aa42ad4279a40be703883645d3793e", 27188,
					   "map.do", "5842cc60d0bf1ecc4e473db1579afc96", 13006),
			Common::EN_ANY,
			Common::kPlatformAmigaAGA,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR2,
		0
	},

	{
		// Ishar 2: Messengers of Doom MS-DOS
		{
			"ishar2",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR2,
		0
	},

	{
		// Ishar 2: Messengers of Doom Macintosh Color
		{
			"ishar2",
			0,
			AD_ENTRY2s("main.fo", "37aa42ad4279a40be703883645d3793e", 29952,
					   "map.fo", "5842cc60d0bf1ecc4e473db1579afc96", 15872),
			Common::EN_ANY,
			Common::kPlatformMacintoshII,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR2,
		0
	},

	{
		// Ishar 3: The Seven Gates of Infinity Atari ST
		{
			"ishar3",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR3,
		0
	},

	{
		// Ishar 3: The Seven Gates of Infinity Atari Falcon
		{
			"ishar3",
			0,
			AD_ENTRY2s("main.fo", "4f7de68f6689cf9617aa1ea03240137e", 27188,
					   "map.fo", "f99633a0110ccf90837ab161be56cf1c", 13006),
			Common::EN_ANY,
			Common::kPlatformAtariFalcon,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR3,
		0
	},

	{
		// Ishar 3: The Seven Gates of Infinity Amiga OCS
		{
			"ishar3",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR3,
		0
	},

	{
		// Ishar 3: The Seven Gates of Infinity Amiga AGA
		{
			"ishar3",
			0,
			AD_ENTRY2s("main.do", "37aa42ad4279a40be703883645d3793e", 27188,
					   "map.do", "5842cc60d0bf1ecc4e473db1579afc96", 13006),
			Common::EN_ANY,
			Common::kPlatformAmigaAGA,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR3,
		0
	},

	{
		// Ishar 3: The Seven Gates of Infinity MS-DOS
		{
			"ishar3",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR3,
		0
	},

	{
		// Ishar 3: The Seven Gates of Infinity Macintosh Color
		{
			"ishar3",
			0,
			AD_ENTRY2s("main.fo", "37aa42ad4279a40be703883645d3793e", 29952,
					   "map.fo", "5842cc60d0bf1ecc4e473db1579afc96", 15872),
			Common::EN_ANY,
			Common::kPlatformMacintoshII,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ISHAR3,
		0
	},

	{
		// Transarctica / Artic Baron Atari ST
		{
			"artic",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_TRANSARCTICA,
		0
	},

	{
		// Transarctica / Artic Baron Atari Falcon
		{
			"artic",
			0,
			AD_ENTRY2s("main.fo", "4f7de68f6689cf9617aa1ea03240137e", 27188,
					   "map.fo", "f99633a0110ccf90837ab161be56cf1c", 13006),
			Common::EN_ANY,
			Common::kPlatformAtariFalcon,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_TRANSARCTICA,
		0
	},

	{
		// Transarctica / Artic Baron Amiga OCS
		{
			"artic",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_TRANSARCTICA,
		0
	},

	{
		// Transarctica / Artic Baron Amiga AGA
		{
			"artic",
			0,
			AD_ENTRY2s("main.do", "37aa42ad4279a40be703883645d3793e", 27188,
					   "map.do", "5842cc60d0bf1ecc4e473db1579afc96", 13006),
			Common::EN_ANY,
			Common::kPlatformAmigaAGA,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_TRANSARCTICA,
		0
	},

	{
		// Transarctica / Artic Baron MS-DOS
		{
			"artic",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_TRANSARCTICA,
		0
	},

	{
		// Transarctica / Artic Baron Macintosh Color
		{
			"artic",
			0,
			AD_ENTRY2s("main.fo", "37aa42ad4279a40be703883645d3793e", 29952,
					   "map.fo", "5842cc60d0bf1ecc4e473db1579afc96", 15872),
			Common::EN_ANY,
			Common::kPlatformMacintoshII,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_TRANSARCTICA,
		0
	},

	{
		// Robinson's Requiem Atari ST
		{
			"robinson",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ROBINSONS,
		0
	},

	{
		// Robinson's Requiem Atari Falcon
		{
			"robinson",
			0,
			AD_ENTRY2s("main.fo", "4f7de68f6689cf9617aa1ea03240137e", 27188,
					   "map.fo", "f99633a0110ccf90837ab161be56cf1c", 13006),
			Common::EN_ANY,
			Common::kPlatformAtariFalcon,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ROBINSONS,
		0
	},

	{
		// Robinson's Requiem Amiga OCS
		{
			"robinson",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ROBINSONS,
		0
	},

	{
		// Robinson's Requiem Amiga AGA
		{
			"robinson",
			0,
			AD_ENTRY2s("main.do", "37aa42ad4279a40be703883645d3793e", 27188,
					   "map.do", "5842cc60d0bf1ecc4e473db1579afc96", 13006),
			Common::EN_ANY,
			Common::kPlatformAmigaAGA,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ROBINSONS,
		0
	},

	{
		// Robinson's Requiem MS-DOS
		{
			"robinson",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ROBINSONS,
		0
	},

	{
		// Robinson's Requiem Macintosh Color
		{
			"robinson",
			0,
			AD_ENTRY2s("main.fo", "37aa42ad4279a40be703883645d3793e", 29952,
					   "map.fo", "5842cc60d0bf1ecc4e473db1579afc96", 15872),
			Common::EN_ANY,
			Common::kPlatformMacintoshII,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ROBINSONS,
		0
	},

	{
		// Robinson's Requiem Panasonic 3DO
		{
			"robinson",
			0,
			AD_ENTRY2s("main.fo", "37aa42ad4279a40be703883645d3793e", 29952,
					   "map.fo", "5842cc60d0bf1ecc4e473db1579afc96", 15872),
			Common::EN_ANY,
			Common::	kPlatform3DO,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ROBINSONS,
		0
	},

		{
		// Targhan Atari ST
		{
			"targhan",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_TARGHAN,
		0
	},

	{
		// Targhan Amiga OCS
		{
			"targhan",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_TARGHAN,
		0
	},

	{
		// Targhan PC MS-DOS
		{
			"targhan",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_TARGHAN,
		0
	},

	{
		// Targhan Macintosh BW
		{
			"targhan",
			0,
			AD_ENTRY2s("main.mo", "37aa42ad4279a40be703883645d3793e", 24146,
					   "map.mo", "5842cc60d0bf1ecc4e473db1579afc96", 8190),
			Common::EN_ANY,
			Common::kPlatformMacintosh,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_TARGHAN,
		0
	},

		{
		// Manhattan Dealers / Operation: Cleanstreets Atari ST
		{
			"manhattan",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_MANHATTAN,
		0
	},

	{
		// Manhattan Dealers / Operation: Cleanstreets Amiga OCS
		{
			"manhattan",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_MANHATTAN,
		0
	},

	{
		// Manhattan Dealers / Operation: Cleanstreets PC MS-DOS
		{
			"manhattan",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_MANHATTAN,
		0
	},

	{
		// Mad Show Atari ST
		{
			"madshow",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_MADSHOW,
		0
	},

	{
		// Mad Show Amiga OCS
		{
			"madshow",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_MADSHOW,
		0
	},

	{
		// Mad Show PC MS-DOS
		{
			"madshow",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_MADSHOW,
		0
	},

	{
		// Windsurf Willy Atari ST
		{
			"windsurf",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_WINDSURF,
		0
	},

	{
		// Windsurf Willy Amiga OCS
		{
			"windsurf",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_WINDSURF,
		0
	},

	{
		// Windsurf Willy PC MS-DOS
		{
			"windsurf",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_WINDSURF,
		0
	},

	{
		// Le Fetiche Maya Atari ST
		{
			"maya",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_MAYA,
		0
	},

	{
		// Le Fetiche Maya Amiga OCS
		{
			"maya",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_MAYA,
		0
	},

	{
		// Le Fetiche Maya PC MS-DOS
		{
			"maya",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_MAYA,
		0
	},

	{
		// Colorado Atari ST
		{
			"colorado",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_COLORADO,
		0
	},

	{
		// Colorado Amiga OCS
		{
			"colorado",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_COLORADO,
		0
	},

	{
		// Colorado PC MS-DOS
		{
			"colorado",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_COLORADO,
		0
	},

	{
		// Crystals of Arborea Atari ST
		{
			"arborea",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ARBOREA,
		0
	},

	{
		// Crystals of Arborea Amiga OCS
		{
			"arborea",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ARBOREA,
		0
	},

	{
		// Crystals of Arborea PC MS-DOS
		{
			"arborea",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_ARBOREA,
		0
	},

	{
		// StarBlade Atari ST
		{
			"starblade",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_STARBLADE,
		0
	},

	{
		// StarBlade Amiga OCS
		{
			"starblade",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_STARBLADE,
		0
	},

	{
		// StarBlade PC MS-DOS
		{
			"starblade",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_STARBLADE,
		0
	},

	{
		// Boston Bomb Club Atari ST
		{
			"boston",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_BOSTON,
		0
	},

	{
		// Boston Bomb Club Amiga OCS
		{
			"boston",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_BOSTON,
		0
	},

	{
		// Boston Bomb Club PC MS-DOS
		{
			"boston",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_BOSTON,
		0
	},

	{
		// Metal Mutant Atari ST
		{
			"metal",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_METAL,
		0
	},

	{
		// Metal Mutant Amiga OCS
		{
			"metal",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_METAL,
		0
	},

	{
		// Metal Mutant PC MS-DOS
		{
			"metal",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_METAL,
		0
	},

	{
		// Bunny Bricks Atari ST
		{
			"bunny",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_BUNNY,
		0
	},

	{
		// Bunny Bricks Amiga OCS
		{
			"bunny",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_BUNNY,
		0
	},

	{
		// Bunny Bricks PC MS-DOS
		{
			"bunny",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_BUNNY,
		0
	},

	{
		// Storm Master Atari ST
		{
			"storm",
			0,
			AD_ENTRY2s("main.ao", "4f7de68f6689cf9617aa1ea03240137e", 10260,
					   "map.ao", "f99633a0110ccf90837ab161be56cf1c", 7412),
			Common::EN_ANY,
			Common::kPlatformAtariST,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_STORM,
		0
	},

	{
		// Storm Master Amiga OCS
		{
			"storm",
			0,
			AD_ENTRY2s("main.co", "37aa42ad4279a40be703883645d3793e", 10260,
					   "map.co", "5842cc60d0bf1ecc4e473db1579afc96", 7412),
			Common::EN_ANY,
			Common::kPlatformAmiga,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_STORM,
		0
	},

	{
		// Storm Master PC MS-DOS
		{
			"storm",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_STORM,
		0
	},

	{
		// Deus PC MS-DOS
		{
			"deus",
			0,
			AD_ENTRY2s("main.io", "37aa42ad4279a40be703883645d3793e", 11240,
					   "map.io", "5842cc60d0bf1ecc4e473db1579afc96", 13014),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_DEUS,
		0
	},
#endif

	{ AD_TABLE_END_MARKER, (GameId)0, 0 }
};

} // End of namespace Alis
