#include "engines/alis/platform.h"

// TODO: Manhattan Dealers, the first game using ALIS engine is different. Its using 'OO' and 'SNG' file extensions and main script is in 'man.sng'

namespace Alis {

    // Default constructor implementation
    sPlatform::sPlatform()
        : kind(Common::kPlatformUnknown),
        version(20),
        uid(GAME_UNKNOWN),
        ext("FO"),
        ram_sz(0x400000),
        video_ram_sz(0xfa00),
        width(320),
        height(200),
        bpp(8),
        px_format(EPxFormatChunky), 
        dbl_buf(true),
        is_little_endian(false),
        path(""),
        main("") {}

    sPlatform::sPlatform(Common::Platform plat) : sPlatform() {

        kind = plat;
        switch (kind)
        {
        case Common::kPlatformDOS:
            ext = "IO";
            is_little_endian = true;
            break;
        case Common::kPlatformAmiga:
            ext = "CO";
            ram_sz = 0x100000;
            video_ram_sz = 0x8000;
            bpp = 4;
            px_format = EPxFormatAmPlanar;
            break;
        case Common::kPlatformAtariST:
            ext = "AO";
            ram_sz = 0x100000;
            video_ram_sz = 0x8000;
            bpp = 4;
            px_format = EPxFormatSTPlanar;
            break;
        case Common::kPlatformMacintosh:
            ext = "MO";
            width = 480;
            height = 300;
            bpp = 1;
            break;
        case Common::kPlatformMacintoshII:
            px_format = EPxFormatSTPlanar;
            break;
        case Common::kPlatformAtariFalcon:
            px_format = EPxFormatSTPlanar;
            break;
        case Common::kPlatformAmigaAGA:
            ext = "DO";
            px_format = EPxFormatAmPlanar;
            break;
        default:
            break;
        }

    }

    sPlatform::sPlatform(Common::Platform gplatform, Alis::GameId game) : sPlatform(gplatform) {

        uid = game;
        switch (game)
        {
        case Alis::GameId::GAME_MANHATTAN:
            version = 10;
            ext = "OO";
            bpp = 4;
            dbl_buf = false;
            main = "main.sng";
            break;
        case Alis::GameId::GAME_MADSHOW:
            version = 11;
            bpp = 4;
            break;
        case Alis::GameId::GAME_TARGHAN:
            dbl_buf = false;
            version = 12;
            bpp = 4;
            break;
        case Alis::GameId::GAME_WINDSURF:
            version = 12;
            bpp = 4;
            break;
        case Alis::GameId::GAME_MAYA:
            dbl_buf = false;
            version = 13;
            bpp = 4;
            break;
        case Alis::GameId::GAME_COLORADO:
            version = 13;
            bpp = 4;
            break;
        case Alis::GameId::GAME_STARBLADE:
            dbl_buf = false;
            bpp = 4;
            break;
        case Alis::GameId::GAME_BOSTON:
            version = 21;
            bpp = 4;
            break;
        case Alis::GameId::GAME_STORM:
            bpp = 4;
            break;
        case Alis::GameId::GAME_METAL:
            bpp = 4;
            break;
        case Alis::GameId::GAME_ARBOREA:
            bpp = 4;
            break;
        case Alis::GameId::GAME_BUNNY:
        case Alis::GameId::GAME_ISHAR2:
            version = 21;
            break;
        case Alis::GameId::GAME_TRANSARCTICA:
            version = 22;
            break;
        case Alis::GameId::GAME_ISHAR3:
            version = 30;
            break;
        case Alis::GameId::GAME_ROBINSONS:
            version = 31;
            break;
        case Alis::GameId::GAME_DEUS:
            version = 40;
            break;
        default:
            break;
        }
        initMainFileName();
    }

    sPlatform::~sPlatform() {}

    void sPlatform::initMainFileName(){
        main = kMainScriptName + ext;
    }

}

