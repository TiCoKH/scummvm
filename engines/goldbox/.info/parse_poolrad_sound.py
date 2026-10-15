#!/usr/bin/env python3
"""
parse_poolrad_sound.py - Parse Pool of Radiance embedded sound data.

The sound data at segment 18AE consists of:
  - Frequency table at 0x0547 (12 x uint16)
  - Song pointer tables at 0x055F (Tandy) and 0x0607 (Speaker)
  - Envelope/instrument command data at 0x0C00+

Envelope Command Format:
  The data is a stream of commands separated by 0xFF bytes.
  Format: [0xFF] [cmd_byte] [val_lo] [val_hi]
  
  The 0xFF byte means "process next command immediately".
  Other control bytes:
    0xFE = loop back (followed by loop params)
    0xFD = conditional jump
    0xFC = unconditional jump  
    0xFB = end of envelope / return

  Command bytes map to channel struct offsets:
    0x00 = duration ticks
    0x04 = base frequency  
    0x06 = frequency delta (portamento/slide)
    0x0A = volume
    0x0C = volume delta (envelope slope)
    0x0E = ???
    0x10 = delay counter
    0x12 = ??? 
    0x16 = envelope base pointer
    0x1C = vibrato waveform pointer
    0x1E = vibrato position
    0x20 = vibrato speed
    0x22 = vibrato depth
    0x24 = vibrato wrap length
    0x26 = loop counter (for FE loops)
    0x28 = ???
"""

import struct
import sys

# Command names for channel struct offsets
CMD_NAMES = {
    0x00: "DURATION",
    0x04: "FREQ",
    0x06: "FREQ_DELTA",
    0x08: "OUT_FREQ",
    0x0A: "VOLUME",
    0x0C: "VOL_DELTA",
    0x0E: "UNK_0E",
    0x10: "UNK_10",
    0x12: "UNK_12",
    0x14: "DELAY_CTR",
    0x16: "ENV_BASE",
    0x18: "ENV_OFFSET",
    0x1A: "NOTE_TICKS",
    0x1C: "VIB_TABLE",
    0x1E: "VIB_POS",
    0x20: "VIB_SPEED",
    0x22: "VIB_DEPTH",
    0x24: "VIB_WRAP",
    0x26: "LOOP_CTR",
    0x28: "UNK_28",
}

# Control flow bytes
CTRL_END = 0xFB
CTRL_JUMP = 0xFC
CTRL_COND = 0xFD
CTRL_LOOP = 0xFE
CTRL_NEXT = 0xFF


def cmd_name(cmd):
    return CMD_NAMES.get(cmd, f"UNK_{cmd:02X}")


def parse_envelope_stream(data, start_offset, seg_base=0x0C00):
    """
    Parse an envelope command stream.
    
    Format: sequences of [separator] [command] [value_lo] [value_hi]
    where separator is 0xFF (immediate), 0xFE (loop), etc.
    
    First entry may start without a leading 0xFF if it's the beginning.
    """
    commands = []
    pos = start_offset
    
    while pos < len(data):
        ctrl = data[pos]
        
        if ctrl == CTRL_END:
            # FB = end marker, followed by 3 bytes (often the init params)
            if pos + 3 < len(data):
                val = struct.unpack_from('<H', data, pos + 1)[0]
                commands.append({
                    'offset': seg_base + pos,
                    'type': 'END',
                    'raw': data[pos:pos+4].hex(),
                    'value': val,
                })
            pos += 4  # Skip FB + 3 bytes
            break
            
        elif ctrl == CTRL_JUMP:
            # FC = jump to address (uint16 follows)
            if pos + 2 < len(data):
                target = struct.unpack_from('<H', data, pos + 1)[0]
                commands.append({
                    'offset': seg_base + pos,
                    'type': 'JUMP',
                    'target': target,
                    'raw': data[pos:pos+3].hex(),
                })
            pos += 3
            break
            
        elif ctrl == CTRL_COND:
            # FD = conditional (with params)
            if pos + 2 < len(data):
                val = struct.unpack_from('<H', data, pos + 1)[0]
                commands.append({
                    'offset': seg_base + pos,
                    'type': 'COND',
                    'value': val,
                    'raw': data[pos:pos+3].hex(),
                })
            pos += 3
            
        elif ctrl == CTRL_LOOP:
            # FE = loop back marker
            if pos + 2 < len(data):
                target = struct.unpack_from('<H', data, pos + 1)[0]
                commands.append({
                    'offset': seg_base + pos,
                    'type': 'LOOP',
                    'target': target,
                    'raw': data[pos:pos+3].hex(),
                })
            pos += 3
            
        elif ctrl == CTRL_NEXT:
            # FF = immediate command follows
            if pos + 3 >= len(data):
                break
            cmd = data[pos + 1]
            val = struct.unpack_from('<H', data, pos + 2)[0]
            commands.append({
                'offset': seg_base + pos,
                'type': 'CMD',
                'cmd': cmd,
                'cmd_name': cmd_name(cmd),
                'value': val,
                'raw': data[pos:pos+4].hex(),
            })
            pos += 4
            
        else:
            # First byte of stream or unexpected - might be start of data
            # Try reading as [cmd, val_lo, val_hi, separator]
            if pos + 3 < len(data):
                cmd = ctrl
                val = struct.unpack_from('<H', data, pos + 1)[0]
                sep = data[pos + 3]
                commands.append({
                    'offset': seg_base + pos,
                    'type': 'RAW',
                    'cmd': cmd,
                    'cmd_name': cmd_name(cmd),
                    'value': val,
                    'separator': sep,
                    'raw': data[pos:pos+4].hex(),
                })
                pos += 4
            else:
                pos += 1
                
        # Safety limit
        if len(commands) > 500:
            break
    
    return commands, pos


def main():
    # Song pointer tables (21 songs x 4 channels each)
    tandy_table = [
        [0x0D2B, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 0
        [0x0D33, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 1
        [0x0C2B, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 2
        [0x0C9B, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 3
        [0x0CC3, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 4
        [0x0C63, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 5
        [0x0CFB, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 6
        [0x0C03, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 7
        [0x0BBF, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 8
        [0x0BDB, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 9
        [0x09CF, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 10
        [0x0D7B, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 11
        [0x0A0F, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 12
        [0x0A4F, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 13
        [0x0B57, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 14
        [0x0B7F, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 15
        [0x0ACF, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 16
        [0x0B13, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 17
        [0x0A8F, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 18
        [0x0E7F, 0x0D2B, 0x0D2B, 0x0D2B],  # Song 19
        [0x0DA3, 0x0000, 0x0000, 0x0000],  # Song 20
    ]

    speaker_table = [
        [0x11BF, 0x11BF, 0x11BF, 0x11BF],  # Song 0 (silence)
        [0x11C7, 0x11F3, 0x121F, 0x11BF],  # Song 1
        [0x11BF, 0x11BF, 0x11BF, 0x11A3],  # Song 2
        [0x110B, 0x1107, 0x1103, 0x11BF],  # Song 3
        [0x1127, 0x11BF, 0x116B, 0x1183],  # Song 4
        [0x10DB, 0x11BF, 0x11BF, 0x11BF],  # Song 5
        [0x11BF, 0x11BF, 0x10B3, 0x10C3],  # Song 6
        [0x1087, 0x1083, 0x11BF, 0x11BF],  # Song 7
        [0x11BF, 0x11BF, 0x11BF, 0x104B],  # Song 8
        [0x11BF, 0x11BF, 0x11BF, 0x106B],  # Song 9
        [0x11BF, 0x11BF, 0x0E97, 0x0EAF],  # Song 10
        [0x124B, 0x1263, 0x127B, 0x11BF],  # Song 11
        [0x11BF, 0x11BF, 0x0ECF, 0x0EE7],  # Song 12
        [0x0F2F, 0x11BF, 0x0F07, 0x0F17],  # Song 13
        [0x11BF, 0x11BF, 0x0FF3, 0x0FFF],  # Song 14
        [0x11BF, 0x11BF, 0x1013, 0x102B],  # Song 15
        [0x11BF, 0x11BF, 0x0F8F, 0x0F9B],  # Song 16
        [0x11BF, 0x11BF, 0x0FBF, 0x11BF],  # Song 17
        [0x0F73, 0x11BF, 0x0F4B, 0x0F5B],  # Song 18
        [0x135C, 0x11BF, 0x11BF, 0x11BF],  # Song 19
        [0x1293, 0x11BF, 0x11BF, 0x11BF],  # Song 20
    ]

    # The hex data from 18AE:0C00 (1912 bytes)
    # Paste the raw bytes here for analysis
    hex_data = (
        "00 00 00 ff 0a 03 00 ff 04 b4 3f ff 26 1e 00 ff"
        " 06 3c fe ff 00 0c 00 ff 06 77 02 ff 00 08 00 fe"
        " 26 0f 0c ff 0a 00 00 ff 00 00 00 ff 0a 03 00 ff"
        " 1c cf 07 ff 24 00 20 ff 22 40 06 ff 20 9b 01 ff"
        " 04 cc 10 ff 06 0a 00 ff 00 1e 00 ff 22 84 03 ff"
        " 04 14 05 ff 06 50 00 ff 00 05 00 ff 0a 00 00 ff"
        " 00 00 00 ff 0a 03 00 ff 26 05 00 ff 04 38 18 ff"
        " 06 00 00 ff 00 06 00 ff 28 0c 00 ff 06 5e fc ff"
        " 00 01 00 ff 06 84 03 ff 00 01 00 fe 28 7b 0c fe"
        " 26 6f 0c ff 0a 00 00 ff 00 00 00 ff 0a 03 00 ff"
        " 04 7b 19 ff 26 0a 00 ff 06 d3 ff ff 00 14 00 ff"
        " 06 f4 01 ff 00 01 00 fe 26 a7 0c ff 0a 00 00 ff"
        " 00 00 00 ff 0a 03 00 ff 04 10 27 ff 1c cf 07 ff"
        " 24 00 20 ff 22 f4 01 ff 20 10 00 ff 06 ce ff ff"
        " 00 1c 00 ff 0a 00 00 ff 00 00 00 ff 0a 03 00 ff"
        " 04 20 4e ff 1c cf 07 ff 24 00 20 ff 22 10 27 ff"
        " 20 10 00 ff 00 05 00 ff 0a 00 00 ff 00 01 00 ff"
        " 0a 03 00 ff 04 34 08 ff 22 98 08 ff 06 64 00 ff"
        " 00 64 00 ff 0a 00 00 ff 00 00 00 ff 0a 03 00 ff"
        " 04 a0 0f ff 00 05 00 ff 04 e8 03 ff 00 04 00 ff"
        " 0a 00 00 ff 00 00 00 ff 0a 03 00 ff 04 b0 36 ff"
        " 06 c8 00 ff 1c cf 07 ff 24 00 20 ff 22 70 17 ff"
        " 20 7b 00 ff 00 08 00 ff 0a 00 00 ff 00 00 00"
    )
    
    data = bytes.fromhex(hex_data.replace(' ', '').replace('\n', ''))
    
    print("=" * 70)
    print("POOL OF RADIANCE SOUND DATA ANALYSIS")
    print("=" * 70)
    print()
    
    # Print song tables
    print("TANDY SONG TABLE (21 songs x 4 channels, at 18AE:055F)")
    print("-" * 60)
    silence_tandy = 0x0D2B
    for i, chs in enumerate(tandy_table):
        active = [f"0x{c:04X}" if c != silence_tandy and c != 0
                  else "------" for c in chs]
        print(f"  Song {i:2d}: {' '.join(active)}")
    
    print()
    print("PC SPEAKER SONG TABLE (21 songs x 4 channels, at 18AE:0607)")
    print("-" * 60)
    silence_spk = 0x11BF
    for i, chs in enumerate(speaker_table):
        active = [f"0x{c:04X}" if c != silence_spk and c != 0
                  else "------" for c in chs]
        print(f"  Song {i:2d}: {' '.join(active)}")
    
    print()
    print()
    
    # Parse the envelope data
    print("ENVELOPE COMMAND DATA (starting at 18AE:0C00)")
    print("-" * 60)
    print()
    print("Format: [FF cmd val16] = immediate register write")
    print("        [FE target16]  = loop back")
    print("        [FD param16]   = conditional")  
    print("        [FC target16]  = jump")
    print("        [FB param16]   = end/init")
    print()
    
    # Find unique stream start addresses from both tables
    all_ptrs = set()
    for chs in tandy_table:
        for p in chs:
            if p != 0 and p != silence_tandy:
                all_ptrs.add(p)
    for chs in speaker_table:
        for p in chs:
            if p != 0 and p != silence_spk:
                all_ptrs.add(p)
    
    # Also add the silence/default streams
    all_ptrs.add(silence_tandy)
    all_ptrs.add(silence_spk)
    
    sorted_ptrs = sorted(all_ptrs)
    
    print(f"Unique stream addresses ({len(sorted_ptrs)}):")
    for p in sorted_ptrs:
        # Determine which songs use this
        tandy_users = []
        for i, chs in enumerate(tandy_table):
            for ch, ptr in enumerate(chs):
                if ptr == p:
                    tandy_users.append(f"T{i}.{ch}")
        spk_users = []
        for i, chs in enumerate(speaker_table):
            for ch, ptr in enumerate(chs):
                if ptr == p:
                    spk_users.append(f"S{i}.{ch}")
        
        users = tandy_users[:3] + spk_users[:3]
        extra = len(tandy_users) + len(spk_users) - len(users)
        user_str = ", ".join(users)
        if extra > 0:
            user_str += f", +{extra} more"
        
        label = ""
        if p == silence_tandy:
            label = " [TANDY SILENCE]"
        elif p == silence_spk:
            label = " [SPEAKER SILENCE]"
            
        print(f"  0x{p:04X}{label}: used by {user_str}")
    
    print()
    print()
    
    # Parse the data we have (starting at 0x0C00)
    # Offset into our data = address - 0x0C00
    data_base = 0x0C00
    
    # Parse streams that fall within our data range
    print("PARSED ENVELOPE STREAMS")
    print("=" * 70)
    
    for ptr in sorted_ptrs:
        local_off = ptr - data_base
        if local_off < 0 or local_off >= len(data):
            print(f"\n  Stream 0x{ptr:04X}: outside available data range")
            continue
            
        commands, end_pos = parse_envelope_stream(data, local_off, data_base)
        
        if not commands:
            continue
            
        # Find which songs use this
        users = []
        for i, chs in enumerate(tandy_table):
            for ch, p in enumerate(chs):
                if p == ptr:
                    users.append(f"Tandy song {i} ch{ch}")
                    break
            if users:
                break
        for i, chs in enumerate(speaker_table):
            for ch, p in enumerate(chs):
                if p == ptr:
                    users.append(f"Speaker song {i} ch{ch}")
                    break
            if users and len(users) > 1:
                break
        
        size = end_pos - local_off
        print(f"\n{'─' * 60}")
        print(f"Stream at 0x{ptr:04X} ({size} bytes) "
              f"- {', '.join(users[:2])}")
        print(f"{'─' * 60}")
        
        for c in commands:
            addr = f"0x{c['offset']:04X}"
            
            if c['type'] == 'CMD':
                val = c['value']
                val_signed = val if val < 0x8000 else val - 0x10000
                print(f"  {addr}: {c['cmd_name']:12s} = "
                      f"0x{val:04X} ({val_signed:+d})")
                      
            elif c['type'] == 'END':
                print(f"  {addr}: END  (val=0x{c['value']:04X})")
                
            elif c['type'] == 'JUMP':
                print(f"  {addr}: JUMP → 0x{c['target']:04X}")
                
            elif c['type'] == 'LOOP':
                print(f"  {addr}: LOOP → 0x{c['target']:04X}")
                
            elif c['type'] == 'COND':
                print(f"  {addr}: COND  (val=0x{c['value']:04X})")
                
            elif c['type'] == 'RAW':
                val = c['value']
                sep_name = {0xFF: 'NEXT', 0xFE: 'LOOP', 0xFD: 'COND',
                           0xFC: 'JUMP', 0xFB: 'END'}.get(
                    c['separator'], f"0x{c['separator']:02X}")
                print(f"  {addr}: [{sep_name}] {c['cmd_name']:12s} = "
                      f"0x{val:04X}")


if __name__ == '__main__':
    main()
