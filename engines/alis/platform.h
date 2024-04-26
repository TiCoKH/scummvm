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

#ifndef ALIS_PLATFORM_H
#define ALIS_PLATFORM_H

#include "common/scummsys.h"
#include "common/platform.h"

#define kPathMaxLen         (256)
#define kDescMaxLen         (1024)

namespace Alis {
/*    typedef enum {
        EGameManhattanDealers        = 0x019bfcc0,
        EGameMadShow                 = 0x00244140,
        EGameWindsurfWilly           = 0x00325aa0,
        EGameTarghan0                = 0x0044aa20,
        EGameTarghan1                = 0x002dc6c0,
        EGameLeFeticheMaya           = 0x00180858,
        EGameColorado                = 0x003ece20,
        EGameStarblade               = 0x00594390,
        EGameStormMaster             = 0x00632ea0,
        EGameMetalMutant             = 0x001b7740,
        EGameCrystalsOfArborea0      = 0x006cb808,
        EGameCrystalsOfArborea1      = 0x006ddd00,
        EGameTransartica             = 0x01312d00,
        EGameBostonBombClub          = 0x00149970,
        EGameBunnyBricks             = 0x002625a0,
        EGameIshar_1                 = 0x00b71b00,
        EGameIshar_2                 = 0x00ec82e0,
        EGameIshar_3                 = 0x015b2330,
        EGameRobinsonsRequiem0       = 0x00f42400,
        EGameRobinsonsRequiem1       = 0x02160ec0,
        EGameAsghan                  = -2,
        EGameDeus                    = -3,
        EGameTournamentOfWarriors    = -4,
        EGameTimeWarriors            = -5,
        EGameXyphoesFantasy          = -6,
        EGameUnknown                 = 0,
    } EGame;
*/
    enum GameId {
	GAME_MANHATTAN,
	GAME_MADSHOW,
	GAME_MAYA,
	GAME_TARGHAN,
	GAME_WINDSURF,
	GAME_COLORADO,
	GAME_ARBOREA,
	GAME_STARBLADE,
	GAME_BOSTON,
	GAME_METAL,
	GAME_STORM,
	GAME_BUNNY,
	GAME_ISHAR1,
	GAME_ISHAR2,
	GAME_ISHAR3,
	GAME_TRANSARCTICA,
	GAME_ROBINSONS,
	GAME_DEUS
};

    typedef enum {
        EPxFormatChunky             = 0,
        EPxFormatSTPlanar           = 1,
        EPxFormatAmPlanar           = 2,
    } EPxFormat;

    struct sPlatform {
        Common::Platform kind;
//        Common::String   name;
        uint32           version;
//        Alis::EGame uid;
        Common::String   ext;           // script file extension
        uint32           ram_sz;        // size of ram, in bytes
        uint32           video_ram_sz;  // size of video ram, in bytes
        uint16           width;         // screen info
        uint16           height;
        uint8            bpp;
        Alis::EPxFormat  px_format;
        bool             dbl_buf;
        bool             is_little_endian;
        Common::String   path;          // path to scripts
        Common::String   main;          // path to main script

        // Default constructor
        sPlatform();
        sPlatform(Common::Platform gplatform);
        sPlatform(Common::Platform gplatform, Alis::GameId game);
        ~sPlatform();

    };
};

#endif //ALIS_PLATFORM_H