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
#include "common/system.h"
#include "common/platform.h"

#define kPathMaxLen         (256)
#define kDescMaxLen         (1024)
#define kMainScriptName         ("MAIN")

namespace Alis {

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
	GAME_DEUS,
    GAME_UNKNOWN = -1
};

    typedef enum {
        EPxFormatChunky             = 0,
        EPxFormatSTPlanar           = 1,
        EPxFormatAmPlanar           = 2,
    } EPxFormat;

    struct sPlatform {
        Common::Platform kind;
        uint32           version;
        Alis::GameId     uid;           // keep name from original source
        Common::String   ext;           // script file extension
        uint32           ram_sz;        // size of ram, in bytes
        uint32           video_ram_sz;  // size of video ram, in bytes
        uint16           width;         // screen info
        uint16           height;
        uint8            bpp;
        Alis::EPxFormat  px_format;
        bool             dbl_buf;
        bool             is_little_endian;
        Common::String   path;          // path to scripts maybe not needed in scummvm
        Common::String   main;          // path to main script

        // Default constructor
        sPlatform();
        sPlatform(Common::Platform gplatform);
        sPlatform(Common::Platform gplatform, Alis::GameId game);
        ~sPlatform();
private:
        void initMainFileName();

    };
};

#endif //ALIS_PLATFORM_H