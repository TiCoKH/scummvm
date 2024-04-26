#include "engines/alis/platform.h"

// TODO: Manhattan Dealers, the first game using ALIS engine is different. Its using 'OO' and 'SNG' file extensions and main script is in 'man.sng'

namespace Alis {

// Default constructor implementation
sPlatform::sPlatform()
    : kind(Common::kPlatformUnknown),
//      name(""),
      version(20),
//      uid(kGameUnknown),
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

}

}
