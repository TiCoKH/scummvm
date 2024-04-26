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

#ifndef ALIS_SCRIPT_H
#define ALIS_SCRIPT_H

#include "common/scummsys.h"

#define kMainScriptID       (0)
#define kNameMaxLen         (16)

namespace Alis {

struct ScanInter {
    uint8 scan_off_bit_0 : 1;
    uint8 inter_off_bit_1 : 1;
    uint8 _dummy2 : 1;
    uint8 _dummy3 : 1;
    uint8 _dummy4 : 1;
    uint8 _dummy5 : 1;
    uint8 _dummy6 : 1;
    uint8 scan_clr_bit_7 : 1;
};


   typedef struct sScriptContext {
        // Constructor
        sScriptContext();

        // Getter and setter functions
        uint16 get_0x34_unknown() const;
        void set_0x34_unknown(uint16 value);

        uint8 get_0x32_unknown() const;
        void set_0x32_unknown(uint8 value);

        uint8 get_0x31_unknown() const;
        void set_0x31_unknown(uint8 value);

        uint8 get_0x30_unknown() const;
        void set_0x30_unknown(uint8 value);

        uint8 get_0x2f_chsprite() const;
        void set_0x2f_chsprite(uint8 value);

        uint8 get_0x2e_script_header_word_2() const;
        void set_0x2e_script_header_word_2(uint8 value);

        uint8 get_0x2d_calign() const;
        void set_0x2d_calign(uint8 value);

        uint8 get_0x2c_calign() const;
        void set_0x2c_calign(uint8 value);

        uint8 get_0x2b_cordspr() const;
        void set_0x2b_cordspr(uint8 value);

        uint16 get_0x2a_clinking() const;
        void set_0x2a_clinking(uint16 value);

        uint8 get_0x28_unknown() const;
        void set_0x28_unknown(uint8 value);

        uint8 get_0x27_creducing() const;
        void set_0x27_creducing(uint8 value);

        uint8 get_0x26_creducing() const;
        void set_0x26_creducing(uint8 value);

        uint8 get_0x25_credon_credoff() const;
        void set_0x25_credon_credoff(uint8 value);

        ScanInter get_0x24_scan_inter() const;
        void set_0x24_scan_inter(int8 value);

        uint8 get_0x23_unknown() const;
        void set_0x23_unknown(uint8 value);

        uint8 get_0x22_cworld() const;
        void set_0x22_cworld(uint8 value);

        uint8 get_0x21_cworld() const;
        void set_0x21_cworld(uint8 value);

        uint16 get_0x20_set_vect() const;
        void set_0x20_set_vect(uint16 value);

        int16 get_0x1e_scan_clr() const;
        void set_0x1e_scan_clr(int16 value);

        int16 get_0x1c_scan_clr() const;
        void set_0x1c_scan_clr(int16 value);

        int16 get_0x1a_cforme() const;
        void set_0x1a_cforme(int16 value);

        uint16 get_0x18_unknown() const;
        void set_0x18_unknown(uint16 value);

        uint16 get_0x16_screen_id() const;
        void set_0x16_screen_id(uint16 value);

        uint32 get_0x14_script_org_offset() const;
        void set_0x14_script_org_offset(uint32 value);

        uint16 get_0x10_script_id() const;
        void set_0x10_script_id(uint16 value);

        uint16 get_0x0e_script_ent() const;
        void set_0x0e_script_ent(uint16 value);

        int16 get_0x0c_vacc_offset() const;
        void set_0x0c_vacc_offset(int16 value);

        int16 get_0x0a_vacc_offset() const;
        void set_0x0a_vacc_offset(int16 value);

        uint32 get_0x08_script_ret_offset() const;
        void set_0x08_script_ret_offset(uint32 value);

        uint8 get_0x04_cstart_csleep() const;
        void set_0x04_cstart_csleep(uint8 value);

        uint8 get_0x03_xinv() const;
        void set_0x03_xinv(uint8 value);

        uint8 get_0x02_wait_cycles() const;
        void set_0x02_wait_cycles(uint8 value);

        uint8 get_0x01_wait_count() const;
        void set_0x01_wait_count(uint8 value);

    private:
        uint16 _0x34_unknown;                   // vram - 0x34: ???
        uint8 _0x32_unknown;                    // vram - 0x32: ???
        uint8 _0x31_unknown;                    // vram - 0x31: ???
        uint8 _0x30_unknown;                    // vram - 0x30: ???
        uint8 _0x2f_chsprite;                   // vram - 0x2f: chsprite
        uint8 _0x2e_script_header_word_2;       // vram - 0x2e: script header word 2
        uint8 _0x2d_calign;                     // vram - 0x2d: calign
        uint8 _0x2c_calign;                     // vram - 0x2c: calign
        uint8 _0x2b_cordspr;                    // vram - 0x2b: cordspr
        uint16 _0x2a_clinking;                  // vram - 0x2a: clinking OK
        uint8 _0x28_unknown;                    // vram - 0x28: ???
        uint8 _0x27_creducing;                  // vram - 0x27: creducing
        uint8 _0x26_creducing;                  // vram - 0x26: creducing
        uint8 _0x25_credon_credoff;             // vram - 0x25: credon / credoff
        ScanInter _0x24_scan_inter;             // vram - 0x24: scan/inter bitfield
        uint8 _0x23_unknown;                    // vram - 0x23: ???
        uint8 _0x22_cworld;                     // vram - 0x22: cworld SHORT!?
        uint8 _0x21_cworld;                     // vram - 0x21: cworld
        uint16 _0x20_set_vect;                  // vram - 0x20: csetvect
        int16 _0x1e_scan_clr;                   // vram - 0x1e: oscan / cscanclr
        int16 _0x1c_scan_clr;                   // vram - 0x1c: oscan / cscanclr
        int16 _0x1a_cforme;                     // vram - 0x1a: cforme / cdelform / cmforme
        uint16 _0x18_unknown;                   // vram - 0x18: ??? OK
        uint16 _0x16_screen_id;                 // vram - 0x16: screen position OK
        uint32 _0x14_script_org_offset;         // vram - 0x14: script header origin OK
        uint16 _0x10_script_id;                 // vram - 0x10: script id OK
        uint16 _0x0e_script_ent;                // vram - 0x0e: czap / cexplode / cnoise OK
        int16 _0x0c_vacc_offset;                // vram - 0x0c: vacc offset OK
        int16 _0x0a_vacc_offset;                // vram - 0x0a: vacc offset OK
        uint32 _0x08_script_ret_offset;         // vram - 0x08: script return offset OK
        uint8 _0x04_cstart_csleep;              // vram - 0x04: cstart / csleep OK
        uint8 _0x03_xinv;                       // vram - 0x03: xinv OK
        uint8 _0x02_wait_cycles;                // vram - 0x02: ?? OK
        uint8 _0x01_wait_count;                 // vram - 0x01: cstart OK
    };

    // HEADER: read from depacked script 24 BYTES
    typedef struct {
        uint16     id;
        uint8      unknown01;       // seems to be always 0x1700 on atari, copied at (vram - $2e)
        uint8      unknown02;
        uint16     code_loc_offset; // where does code start from (script header + id) ? on atari, always $16
        uint32     ret_offset;      // some scripts have a "sub-script" into them: this is the offset to their code location
        uint32     dw_unknown3;
        uint32     dw_unknown4;
        uint16     w_unknown5;      // almost always 0x20 (is 0xa for "message*" and 0x4 for "objet")
        uint16     vram_alloc_sz;   // number of bytes to alloc fo this script
        uint16     w_unknown7;      // almost always 0x20 (is 0x4 for "message*" and "objet")
    } sScriptHeader;

// =============================================================================
// MARK: - SCRIPT
// =============================================================================

    typedef struct {
        char           name[kNameMaxLen];
        uint32         sz;
        sScriptHeader  header;    // read from depacked file
        uint8          type;
        uint32         data_org;    // offset to script data
    } sAlisScriptData;

    typedef struct {
        char             *name;
        sAlisScriptData  *data;
        uint32           vram_org;    // each script has its own virtual context and memory
        int16            vacc_off;
        uint8            running;
        uint32           pc;     // offset in memory
        uint32           pc_org; // offset in memory
    } sAlisScriptLive;

    class Script {
    private:
        Alis::sScriptContext scontext;
        Alis::sAlisScriptLive slive;
        uint8 is_packedx(uint32 magic);
        int search_insert(uint32 *nums, uint32 size, int target_id);
        void invdigit(uint8 *sample);

    public:
        Script();
//        void script_guess_game(const char *script_path);
        Alis::sAlisScriptData *script_load(const char *script_path);
        void script_unload(Alis::sAlisScriptData *script);
        bool is_delay_script(char *name);
        Alis::sAlisScriptLive *script_live(Alis::sAlisScriptData *script);
        // read data from script, these will increase the virtual program counter
        uint8 script_read8(void);
        uint16 script_read16(void);
        uint32 script_read24(void);
        uint32 script_read32(void);
        void script_read_bytes(uint32 len, uint8 *dest);
        void script_read_until_zero(char *dest);
        void script_jump(int32 offset);
    };

 } // Alis

#endif //ALIS_SCRIPT_H