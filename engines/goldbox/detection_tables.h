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

namespace Goldbox {

const PlainGameDescriptor goldboxGames[] = {
	{ "poolrad", "Pool of Radiance (v1.0/v1.3)" },
	{ "curse", "Curse of the Azure Bonds" },
	{ 0, 0 }
};

const GoldboxGameDescription gameDescriptions[] = {
	{
		{
			"poolrad",
			nullptr,
			AD_ENTRY1s("title.dax", "2c065919198899a59bc3a16786b0212e", 33898),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO1(GUIO_NONE)
		},
		GAMETYPE_POOLRAD
	},

	{
		// Placeholder checksum: no real Curse of the Azure Bonds data files
		// have been analyzed yet, so this entry cannot match a real install.
		// It exists to exercise the GAMETYPE_CURSE -> GameFactory -> engine
		// dispatch path end-to-end; replace with real AD_ENTRY1s data once
		// Curse data files are supported.
		{
			"curse",
			nullptr,
			AD_ENTRY1s("title.dax", "0000000000000000000000000000000000", 0),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE | ADGF_UNSUPPORTED,
			GUIO1(GUIO_NONE)
		},
		GAMETYPE_CURSE
	},

	{AD_TABLE_END_MARKER,0}
};

} // End of namespace Goldbox
