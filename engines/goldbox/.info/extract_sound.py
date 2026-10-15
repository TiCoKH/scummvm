#!/usr/bin/env python3
"""
extract_sound.py - Extract embedded sound data from Pool of Radiance DOS EXE.

Usage:
    python extract_sound.py <path_to_exe> --data-offset 0xNNNN
    python extract_sound.py <path_to_exe> --auto

The sound driver data segment contains (in order):
  1. Frequency table: 12 uint16 values (PIT divisors for chromatic scale)
  2. Song note streams: variable-length bytecode per channel
  3. Song pointer table: N songs x 4 channels x uint16 (pointers into this segment)
  4. Vibrato waveform table: 256-byte sine LUT
  5. Envelope/effect data: sequences of 4-byte entries

Song Note Stream Format:
    byte 0: [ch2:1][note6:0]  where bit7=last_in_group, bits6-5=channel, bits4-0=note
    byte 1: duration (ticks, scaled by tempo)
    Terminator: byte >= 0xFA

Envelope Data Format (referenced by XT_PROGRAM):
    4 bytes per entry: [int16 volume_delta] [int16 duration]
    When duration == -1 (0xFFFF): volume_delta is absolute volume command
    When volume==0 and duration==-1: silence/stop
"""

import struct
import sys
import os
import argparse


NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']


def read_u16(data, offset):
    if offset + 2 > len(data):
        return None
    return struct.unpack_from('<H', data, offset)[0]


def read_i16(data, offset):
    if offset + 2 > len(data):
        return None
    return struct.unpack_from('<h', data, offset)[0]


def read_u8(data, offset):
    if offset >= len(data):
        return None
    return data[offset]


def find_freq_table(data):
    """
    Find the 12-entry frequency table. It's 12 descending uint16 values
    representing PIT divisors (higher value = lower pitch).
    The values descend from ~0xC000 down to ~0x8700 across one octave.
    """
    candidates = []
    for i in range(0, len(data) - 24, 2):
        vals = [read_u16(data, i + j * 2) for j in range(12)]
        if None in vals:
            continue
        # Check descending and in reasonable PIT range
        if all(v is not None and v > 0x80 for v in vals):
            descending = all(vals[j] > vals[j + 1] for j in range(11))
            if descending and vals[0] > 0x3000 and vals[11] > 0x100:
                candidates.append((i, vals))
    return candidates


def find_vibrato_table(data):
    """
    Find the 256-byte sine waveform LUT.
    It goes 0x00 -> 0x7E -> 0x00 -> 0x82 -> 0x00 (signed sine).
    """
    candidates = []
    for i in range(0, len(data) - 256):
        # Check signature: starts at 0, peaks at ~0x7E around byte 64,
        # crosses zero at 128, valley at ~0x82 around byte 192
        if (data[i] == 0x00 and
            data[i + 1] == 0x03 and
            data[i + 64] >= 0x7C and data[i + 64] <= 0x7F and
            data[i + 128] == 0x00 and
            data[i + 192] >= 0x80 and data[i + 192] <= 0x84):
            candidates.append(i)
    return candidates


def find_pointer_table(data, seg_size):
    """
    Find the song pointer table. It's a series of uint16 values that
    point to valid offsets within the segment where note streams live.
    Songs have 4 channel pointers each (8 bytes per song).
    Look for clusters of small pointers that point to data before the table.
    """
    candidates = []

    for i in range(0, len(data) - 16, 2):
        # Check if this looks like 4 consecutive valid pointers
        ptrs = [read_u16(data, i + j * 2) for j in range(4)]
        if None in ptrs:
            continue

        # At least 2 non-zero pointers
        non_zero = [p for p in ptrs if p != 0]
        if len(non_zero) < 1:
            continue

        # All non-zero pointers should be valid offsets and point BEFORE
        # this table position (note data comes before pointer table)
        valid = all(0 < p < i and p < seg_size for p in non_zero)
        if not valid:
            continue

        # Check that the pointed data starts with valid note bytes
        all_valid_streams = True
        for p in non_zero:
            b = read_u8(data, p)
            if b is None or b >= 0xFA:
                # Might be a zero-length song, skip
                pass
            elif b == 0:
                all_valid_streams = False
                break

        if all_valid_streams:
            candidates.append(i)

    return candidates


def parse_note_stream(data, start, max_len=4096):
    """Parse note bytecode stream. Returns (events, end_offset)."""
    events = []
    pos = start

    if start == 0 or start >= len(data):
        return events, start

    end_limit = min(start + max_len, len(data))

    while pos < end_limit:
        b = read_u8(data, pos)
        if b is None:
            break

        if b >= 0xFA:
            events.append({
                'type': 'END',
                'marker': b,
                'offset': pos
            })
            pos += 1
            break

        dur = read_u8(data, pos + 1)
        if dur is None:
            break

        channel = (b >> 5) & 0x03
        note_num = b & 0x7F
        is_last = (b & 0x80) != 0

        evt = {
            'type': 'NOTE',
            'offset': pos,
            'raw': b,
            'channel': channel,
            'note': note_num,
            'duration': dur,
            'is_last': is_last,
        }

        if note_num == 0x7F:
            evt['name'] = 'REST'
        else:
            octave = 0
            idx = note_num
            while idx > 11:
                idx -= 12
                octave += 1
            if idx < 12:
                evt['name'] = f"{NOTE_NAMES[idx]}{octave}"
            else:
                evt['name'] = f"?{note_num}"

        events.append(evt)
        pos += 2

    return events, pos


def parse_envelope_stream(data, start, max_len=2048):
    """
    Parse envelope data (4-byte entries).
    Returns list of entries and end offset.
    """
    entries = []
    pos = start

    if start >= len(data):
        return entries, start

    end_limit = min(start + max_len, len(data))

    while pos + 4 <= end_limit:
        val = read_i16(data, pos)
        dur = read_i16(data, pos + 2)

        entry = {
            'offset': pos,
            'value': val,
            'duration': dur,
        }

        if dur == -1:
            entry['type'] = 'CMD'
            if val == 0:
                entry['desc'] = 'SILENCE'
                entries.append(entry)
                pos += 4
                break  # End of envelope
            else:
                entry['desc'] = f'SET_VOL={val}'
        else:
            entry['type'] = 'ENV'
            entry['desc'] = f'delta={val} dur={dur}'

        entries.append(entry)
        pos += 4

        # Safety: if we see too many entries, bail
        if len(entries) > 200:
            break

    return entries, pos


def scan_songs_at_table(data, table_offset, max_songs=32):
    """Read song entries from a known table offset."""
    songs = []

    for sid in range(max_songs):
        off = table_offset + sid * 8
        if off + 8 > len(data):
            break

        channels = [read_u16(data, off + ch * 2) for ch in range(4)]
        if None in channels:
            break

        # Stop at all-zero entry (but keep first if it's zero = silence)
        if all(c == 0 for c in channels) and sid > 0:
            break

        # Stop if pointers look invalid
        if any(c > len(data) for c in channels if c != 0):
            break

        songs.append(channels)

    return songs


def print_analysis(data, data_offset_in_file, verbose=False):
    """Full analysis of the sound data segment."""
    print(f"Analyzing {len(data)} bytes of sound data...")
    print()

    # Find frequency table
    freq_tables = find_freq_table(data)
    if freq_tables:
        ft_off, ft_vals = freq_tables[0]
        print(f"Frequency table at offset 0x{ft_off:04X} "
              f"(file: 0x{data_offset_in_file + ft_off:05X}):")
        for i, v in enumerate(ft_vals):
            print(f"  {NOTE_NAMES[i]:3s}: 0x{v:04X} ({v:5d})")
        print()
    else:
        print("WARNING: Could not find frequency table")
        ft_off = 0

    # Find vibrato table
    vib_tables = find_vibrato_table(data)
    if vib_tables:
        print(f"Vibrato sine table at offset 0x{vib_tables[0]:04X} "
              f"(file: 0x{data_offset_in_file + vib_tables[0]:05X})")
        print()

    # Find pointer table by looking for clusters of pointers
    # The pointer table in the dump appears to start with values like
    # bf 11, c7 11, f3 11... which are offsets 0x11BF, 0x11C7, etc.
    # These point into the envelope data section

    # Strategy: look for the note data first (starts after freq table)
    # Then find the pointer table (comes after note data)

    # The note data in the dump starts at offset ~0x18 with "2b 0d" patterns
    # Let's find where it begins
    note_start = ft_off + 24 if freq_tables else 0x18

    # Find first valid song data
    print("=" * 70)
    print("SONG NOTE STREAMS")
    print("=" * 70)

    # Scan for note stream boundaries
    streams = []
    pos = note_start
    while pos < len(data) - 2:
        b = read_u8(data, pos)
        if b is None:
            break

        # Check if this could be start of a note stream
        if b < 0xFA and b != 0:
            events, end = parse_note_stream(data, pos)
            if len(events) >= 4:  # At least a few notes
                streams.append((pos, end, events))
                pos = end
                continue
        pos += 1

    print(f"\nFound {len(streams)} note stream(s):")
    for i, (start, end, events) in enumerate(streams):
        note_count = sum(1 for e in events if e['type'] == 'NOTE')
        size = end - start
        print(f"\n  Stream {i}: offset=0x{start:04X}-0x{end:04X} "
              f"({size} bytes, {note_count} notes)")
        if verbose:
            for j, evt in enumerate(events[:20]):
                if evt['type'] == 'NOTE':
                    print(f"    [{j:3d}] @0x{evt['offset']:04X}: "
                          f"{evt['name']:5s} dur={evt['duration']:3d} "
                          f"ch={evt['channel']} "
                          f"{'|' if evt['is_last'] else '+'}")
                else:
                    print(f"    [{j:3d}] @0x{evt['offset']:04X}: "
                          f"END (0x{evt['marker']:02X})")
            if len(events) > 20:
                print(f"    ... ({len(events) - 20} more)")

    # Now find the pointer table
    print()
    print("=" * 70)
    print("SONG POINTER TABLE")
    print("=" * 70)

    # The pointer table values should point to our found streams
    stream_starts = set(s[0] for s in streams)

    # Look for uint16 values that match stream start offsets
    for i in range(0, len(data) - 8, 2):
        ptrs = [read_u16(data, i + j * 2) for j in range(4)]
        if None in ptrs:
            continue
        # Check if any of these point to known stream starts
        matches = sum(1 for p in ptrs if p in stream_starts)
        if matches >= 2:
            print(f"\n  Possible song table at offset 0x{i:04X}:")
            # Try to read full table from here
            songs = scan_songs_at_table(data, i)
            for sid, chs in enumerate(songs):
                labels = []
                for ch, ptr in enumerate(chs):
                    if ptr == 0:
                        labels.append("----")
                    elif ptr in stream_starts:
                        labels.append(f"0x{ptr:04X}*")
                    else:
                        labels.append(f"0x{ptr:04X}")
                print(f"    Song {sid}: [{', '.join(labels)}]")
            break

    # Envelope data
    print()
    print("=" * 70)
    print("ENVELOPE / EFFECT DATA")
    print("=" * 70)

    # Look for the envelope pattern: "ff 0a 03 00" or "ff 04 XX XX"
    # These are 4-byte entries where bytes 2-3 are duration (often 0x00FF)
    env_starts = []
    for i in range(0, len(data) - 8):
        # Look for the pattern that starts envelope blocks:
        # 0x0A, 0x03, 0x00, 0xFF (little endian: val=0x030A, dur=0xFF00)
        # Actually from the dump: "ff 0a 03 00" means val=0x0AFF, dur=0x0003
        # Hmm, let's look at the actual pattern from your dump

        # The envelope blocks start with: FF 0A 03 00
        # In LE: word[0] = 0x0AFF, word[1] = 0x0003
        # That means: volume_delta = 0x0AFF (or as signed: 2815), duration = 3
        # This is likely an init sequence

        if (i + 4 <= len(data) and
            data[i] == 0xFF and data[i + 1] == 0x0A and
            data[i + 2] == 0x03 and data[i + 3] == 0x00):
            env_starts.append(i)

    if env_starts:
        print(f"\nFound {len(env_starts)} envelope block(s):")
        for idx, es in enumerate(env_starts[:20]):
            entries, end = parse_envelope_stream(data, es)
            size = end - es
            print(f"  Env {idx}: offset=0x{es:04X} "
                  f"({size} bytes, {len(entries)} entries)")
            if verbose:
                for e in entries[:8]:
                    print(f"    @0x{e['offset']:04X}: {e['desc']}")
                if len(entries) > 8:
                    print(f"    ... ({len(entries) - 8} more)")


def dump_all(data, data_offset, output_dir):
    """Dump all sound data to files."""
    os.makedirs(output_dir, exist_ok=True)

    # Dump the whole segment
    seg_path = os.path.join(output_dir, "sound_segment.bin")
    with open(seg_path, 'wb') as f:
        f.write(data)
    print(f"Wrote full segment: {seg_path} ({len(data)} bytes)")

    # Find and dump individual streams
    streams = []
    pos = 0
    while pos < len(data) - 2:
        b = read_u8(data, pos)
        if b is None:
            break
        if b < 0xFA and b != 0 and (b & 0x1F) < 0x1F:
            events, end = parse_note_stream(data, pos)
            if len(events) >= 4:
                streams.append((pos, end, events))
                pos = end
                continue
        pos += 1

    for i, (start, end, events) in enumerate(streams):
        stream_path = os.path.join(output_dir, f"stream_{i:02d}_0x{start:04X}.bin")
        with open(stream_path, 'wb') as f:
            f.write(data[start:end])
        print(f"Wrote stream {i}: {stream_path} ({end - start} bytes)")


def auto_find_sound_data(exe_data):
    """
    Find the sound segment by looking for the frequency table
    followed by note data patterns.
    """
    # Look for descending frequency table pattern
    freq_tables = find_freq_table(exe_data)

    results = []
    for ft_off, ft_vals in freq_tables:
        # Check if note-like data follows within ~32 bytes
        check_start = ft_off + 24
        if check_start + 10 > len(exe_data):
            continue

        # After freq table, we expect note stream data
        # Note bytes have bits 4-0 as note (0-30ish) and duration > 0
        b0 = exe_data[check_start]
        b1 = exe_data[check_start + 1]
        if b0 < 0xFA and b1 > 0 and b1 < 0x80:
            # Likely start of sound data segment
            # Back up to include any header/padding before freq table
            seg_start = max(0, ft_off - 8)
            results.append(seg_start)

    return results


def main():
    parser = argparse.ArgumentParser(
        description='Extract sound data from Pool of Radiance DOS EXE')
    parser.add_argument('exe_path', help='Path to the DOS executable')
    parser.add_argument('--data-offset', type=lambda x: int(x, 0),
                        default=None,
                        help='File offset where sound data starts (hex)')
    parser.add_argument('--data-size', type=lambda x: int(x, 0),
                        default=0x2000,
                        help='Amount of data to analyze (default: 0x2000)')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Show detailed note/envelope data')
    parser.add_argument('--dump-dir', '-d', default=None,
                        help='Directory to dump raw sound data')
    parser.add_argument('--auto', '-a', action='store_true',
                        help='Auto-detect sound data location')
    parser.add_argument('--raw-hex', type=str, default=None,
                        help='Analyze raw hex string directly (for testing)')

    args = parser.parse_args()

    if args.raw_hex:
        # Direct hex input mode (for testing with pasted data)
        hex_str = args.raw_hex.replace(' ', '').replace('\n', '')
        data = bytes.fromhex(hex_str)
        print(f"Analyzing {len(data)} bytes of raw hex input")
        print_analysis(data, 0, verbose=args.verbose)
        if args.dump_dir:
            dump_all(data, 0, args.dump_dir)
        return

    with open(args.exe_path, 'rb') as f:
        exe_data = f.read()

    print(f"Loaded {args.exe_path} ({len(exe_data)} bytes)")

    if args.auto or args.data_offset is None:
        print("\nAuto-detecting sound data...")
        candidates = auto_find_sound_data(exe_data)

        if not candidates:
            print("ERROR: Could not find sound data.")
            print("Use --data-offset to specify manually.")
            sys.exit(1)

        print(f"Found {len(candidates)} candidate location(s):")
        for i, off in enumerate(candidates):
            print(f"  [{i}] file offset 0x{off:05X}")

        # Analyze first valid candidate
        for off in candidates:
            end = min(off + args.data_size, len(exe_data))
            seg = exe_data[off:end]
            print(f"\n>>> Analyzing candidate at 0x{off:05X}")
            print_analysis(seg, off, verbose=args.verbose)
            if args.dump_dir:
                dump_all(seg, off, args.dump_dir)
            break
    else:
        off = args.data_offset
        end = min(off + args.data_size, len(exe_data))
        seg = exe_data[off:end]
        print(f"Reading {len(seg)} bytes from offset 0x{off:05X}")
        print_analysis(seg, off, verbose=args.verbose)
        if args.dump_dir:
            dump_all(seg, off, args.dump_dir)


if __name__ == '__main__':
    main()
