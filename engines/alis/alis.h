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

#ifndef ALIS_ALIS_H
#define ALIS_ALIS_H

#include "engines/engine.h"
#include "engines/advancedDetector.h"


#include "common/events.h"
#include "common/platform.h"
#include "common/queue.h"
#include "common/rect.h"
#include "common/str.h"

#include "audio/mixer.h"

#include "alis/detection.h"

#include "video/video_decoder.h"
#include "image/image_decoder.h"

struct ADGameDescription;

namespace Graphics {
struct Surface;
}

namespace Alis {

class Console;

enum AlisDebugChannels {
    kDebugGeneral = 1 << 0
};

typedef uint8             u8;
typedef uint16            u16;
typedef uint32            u32;
typedef int8              s8;
typedef int16             s16;
typedef int32             s32;


#ifndef max
# define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
# define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define kNameMaxLen             (16)
#define kHostRAMSize            (1024 * 1024 * 8)
#define kVirtualRAMSize         (0xffff * sizeof(uint8))

#define kMaxScripts             (256)
#define kBSSChunkLen            (256)

#define SPRITEMEM_PTR image.spritemem + image.basesprite

#define SPRITE_VAR(x) (x ? (sSprite *)(SPRITEMEM_PTR + x) : NULL)

#define ELEMIDX(x) ((((x - 0x78) / 0x30) * 0x28) + 0x8078) // return comparable number to what we see in ST debugger

#define ENTSCR(x) alis.live_scripts[x / sizeof(sScriptLoc)]

#define ALIS_SCR_WCX    0
#define ALIS_SCR_WCY    (alis.platform.version >= 30 ? 8 : 2)
#define ALIS_SCR_WCZ    (alis.platform.version >= 30 ? 16 : 4)

#define ALIS_SCR_WCAX    (alis.platform.version >= 30 ? 24 : -1)
#define ALIS_SCR_WCAY    (alis.platform.version >= 30 ? 32 : -1)
#define ALIS_SCR_WCAZ    (alis.platform.version >= 30 ? 40 : -1)

#define ALIS_SCR_ADDR    (alis.platform.version >= 30 ? 0x32 : 0x8)

#define ALIS_SCR_WCX2    (alis.platform.version >= 30 ? 0x34 : 0x9)
#define ALIS_SCR_WCY2    (alis.platform.version >= 30 ? 0x38 : 0xa)
#define ALIS_SCR_WCZ2    (alis.platform.version >= 30 ? 0x3c : 0xb)


#define ALIS_ERR_FOPEN          (0x01)
#define ALIS_ERR_FWRITE         (0x07)
#define ALIS_ERR_FCREATE        (0x08)
#define ALIS_ERR_FDELETE        (0x09)
#define ALIS_ERR_CDEFSC         (0x0a)
#define ALIS_ERR_VRAM_OVERFLOW  (0x0c)
#define ALIS_ERR_FREAD          (0x0d)
#define ALIS_ERR_FCLOSE         (0x0e)
#define ALIS_ERR_FSEEK          (0x00)


typedef struct {
    uint8   errnum;
    char    name[kNameMaxLen];
    char    descfmt[kDescMaxLen];
} sAlisError;

typedef void    alisRet;
typedef alisRet (*alisOpcode)(void);

#define DECL_OPCODE(n, f, d)    { n, f, #f, d }

typedef struct {
    uint8       code;
    alisOpcode  fptr;
    char        name[kNameMaxLen];
    char        desc[kDescMaxLen];
} sAlisOpcode;

typedef struct __attribute__((packed)) {
    uint32         vram_offset;
    uint16         offset;
} sScriptLoc;

// vm specs, loaded from packed main script header
typedef struct {
    // read values from packed main script
    uint16     script_data_tab_len;
    uint16     script_vram_tab_len;
    uint32     unused;
    uint32     max_allocatable_vram;
    uint32     vram_to_data_offset;
    
    // computed values
    uint32     script_vram_max_addr;
} sAlisSpecs;

typedef struct {
    
    uint32 address;
    uint32 length;
    
} sRawBlock;

typedef struct {
    uint16 val0;
    uint16 val1;
    uint32 val2;
    uint32 val3;
    uint32 val4;
} sMainHeader;

class AlisGame : public Engine {
public:
    AlisGame(OSystem *syst, const Alis::AlisGameDescription *gameDesc);
    ~AlisGame() override;

    
    Common::Error run() override;

protected:
    virtual void readTables() = 0;
    virtual void postSceneBitmaps() = 0;
    virtual bool handlePlatformJoyButton(int button) { return false; }
    virtual bool handlePlatformKeyDown(int button) { return false; }
    virtual void loadImage(const Common::String &name);
    virtual void startGraphics() = 0;
    void blitImageSurface(const Graphics::Surface *surface);
    virtual void blitImage();
    virtual void handleEvent(const Common::Event &event);
    virtual int getSceneNumb(const Common::String &sName);
    virtual void preActions() {}

    static const int kMaxName = 13 + 1;
    static const int kMaxBitmaps = 2000;
    static const int kMaxScene = 100;
    void initTables();

    const Alis::AlisGameDescription *_gameDescription;
    const Alis::sPlatform platform;

    Graphics::PixelFormat _targetFormat;
    Graphics::Surface *_compositeSurface;
    Console *_console;

};

/*
class AlisVM : public AlisGame {
public:
    AlisVM(OSystem *syst, const ADGameDescription *gameDesc);

protected:
    void readTables() override;
    void postSceneBitmaps() override;
    void startGraphics() override;
    void handleEvent(const Common::Event &event) override;
    void blitImage() override;
    int getSceneNumb(const Common::String &sName) override;
    void preActions() override;

private:
    void skipVideo();
    void loadMikeDecision(const Common::String &dirname, const Common::String &baseFilename, uint num);
    void joyUp();
    void joyDown();
    void joyA();
    void updateHiLite();

    bool _cheatEnabled;
    int _cheatFSM;
    bool _leftShoulderPressed;
    int _kbdHiLite;
    int _mouseHiLite;
    int _hiLite;
    Image::ImageDecoder *_ctrlHelpImage;
};
*/

} // End of namespace Alis

#endif
