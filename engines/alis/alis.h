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

#include "common/error.h"
#include "common/keyboard.h"
#include "common/file.h"
#include "common/random.h"
#include "common/rect.h"
#include "common/scummsys.h"
#include "common/system.h"
#include "common/memstream.h"

#include "engines/engine.h"

#include "graphics/surface.h"

#include "platform.h"
#include "script.h"

#define kHostRAMSize            (1024 * 1024 * 8)
#define kVirtualRAMSize         (0xffff * sizeof(uint8))

#define kMaxScripts             (256)
#define kBSSChunkLen            (256)

namespace Alis {

enum AlisDebugChannels {
    kDebugPath = 1 << 0,
    kDebugGraphics = 1 << 1,
    kDebugVideo = 1 << 2,
    kDebugActor = 1 << 3,
    kDebugObject = 1 << 4,
    kDebugCollision = 1 << 5
};

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

class AlisVM {

    public:
        // platform
        sPlatform        platform;

        sMainHeader      header;

        sAlisSpecs       specs;


        // Absolute address of vm's virtual ram.
        // On atari the operating system gives us $22400.
        #define ALIS_VM_RAM_ORG (0x22400)
        uint8            *vram_org;

        // On atari, it's a stack of absolute script data adresses,
        //   each address being 4 bytes long.
        // The maximum count of script data adresses is given by
        //   the packed main script header (word at offset $6).
        // This table is located at ALIS_VM_RAM_ORG.
        uint32           *script_data_orgs;

        // A stack of tuples made of:
        //   absolute script vram adresses (uint32)
        //   offset (uint16)
        //
        // Located at VRAM_ORG + (max_script_addrs * sizeof(uint32))
        // On atari it's ($22400 + ($3c * 4)) ==> $224f0
        // $224f0
        sScriptLoc       *atent_ptr;
        uint32           *atprog_ptr;

    //    uint8              nmode;
        uint8            automode;

        uint8            fallent;
        uint8            fseq;
        uint8            fmuldes;
        uint8            fadddes;
        uint8            ferase;

        uint32           atprog;     // 0x22400
        uint32           debprog;    // 0x2edd8
        uint32           finprog;
        uint32           dernprog;
        uint16           maxprog;
        uint16           nbprog;

        uint16           saversp;

        uint16           fview;
        uint32           valnorme;
        int16            valchamp;
        int16            *ptrent;
        int16            tablent[128];
        int16            matent[128];

        int32            atent;      // 0x224f0
        int32            debent;     // 0x2261c
        int32            finent;
        int32            maxent;
        int16            nbent;
        int16            dernent;

        uint32           finmem;     // 0xf6e98

        int32            basemem;    // 0x22400
        int32            basevar;    // 0x0
        int32            basemain;   // 0x22690

        // mouse
        uint8            mousflag;
        uint8            fmouse;
        uint8            fremouse;
        uint8            fremouse2;
        uint8            butmouse;
        uint32           cbutmouse;
        uint16           oldmouse;
        uint8            *desmouse;

        int16            prevkey;

        int16            wcx;
        int16            wcy;
        int16            wcz;

        int16            wforme;
        int16            matmask;

        int16            poldy;
        int16            poldx;

        // true if disasm only
        uint8            disasm;

        // true if vm is running
        uint8            running;

        // virtual 16-bit accumulator (A4)
        int16            *acc;
        int16            *acc_org;

        // MEMORY
        uint8            *mem; // host: system memory (hardware)

        uint8            flagmain;

        // in atari, located at $22400
        // contains the addresses of the loaded scripts' data
        // 60 dwords max (from $22400 -> $224f0)
        // uint32             script_data_offsets[kMaxScripts];
        // uint8              script_id_stack[kMaxScripts]; // TODO: use a real stack ?
        // uint8              script_count;
        uint8            script_index;

        // SCRIPTS
        // global table containing all depacked scripts
        sAlisScriptLive  *live_scripts[kMaxScripts];
        sAlisScriptData  *loaded_scripts[kMaxScripts];

        // pointer to current script
        sAlisScriptLive  *script;
        sAlisScriptLive  *main;

        // virtual registers
        int16            varD6;
        int16            varD7;

        // running script id
        uint16           varD5;

        // string buffers
        char             *bsd7;
        char             *bsd6;
        char             *bsd7bis;

        char             *sd7;
        char             *sd6;
        char             *oldsd7;

        uint32           vstandard;

        char             autoname[256];

        uint32           tabptr;

        // data buffers
        uint8            buffer[1024];
        sRawBlock        blocks[1024];

        uint8            charmode;

        // font
        uint16           foasc;
        uint16           fonum;
        uint8            folarg;
        uint8            fohaut;
        uint16           fomax;

        uint8            witmov;
        uint8            fmitmov;
        uint16           goodmat;
        uint32           baseform;

        int8             typepack;
        uint8            wordpack;
        uint32           longpack;

        // helper: executed instructions count
        uint32           icount;
        uint8            restart_loop;

        // virtual status register
        struct {
            uint8 zero: 1;
            uint8 neg: 1;
        } sr;

        // helpers
        uint8             oeval_loop;

        // system helpers
        Common::File     *fp;
        uint16           openmode;

        uint16           vquality;

        // misc
        uint8            fswitch;
        uint8            ctiming;
        uint8            cstopret;
        uint16           random_number;

        int8             vprotect;

        uint32           timeclock;

//        struct timeval   time;

        uint8            swap_endianness;

        int32            load_delay;
        int32            unload_delay;

        AlisVM();

};

}// End of namespace Alis

#endif //ALIS_ALIS_H
