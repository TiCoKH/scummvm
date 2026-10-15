#!/usr/bin/env python3
"""
Parse Pool of Radiance sound data.

After re-analysis of XT_GameSTART, the song stream format is:

  [note_byte, duration_byte] pairs, where:
    note_byte:
      bit 7: "last in group" / end-of-phrase marker
      bits 6-0: note number (0-127, 0x7F = rest)
        note % 12 = chromatic pitch (C,C#,D,D#,E,F,F#,G,G#,A,A#,B)
        note / 12 = octave
    duration_byte: ticks (multiplied by tempo factor)
  
  When note_byte >= 0xFA: end of song

  The "channel" is NOT encoded in the note stream - it's determined by
  which pointer in the song table was used to load this stream.
  The (bVar7 >> 5) in XT_GameSTART selects which of the 4 hardware
  voice structures to write to, based on a separate "channel assignment"
  byte that precedes or is part of the song header.

Actually wait - re-reading XT_GameSTART one more time:
  bVar7 is the FIRST byte read from the stream.
  Then the while loop reads pairs [note, duration] from pbVar8.
  Channel = bVar7 >> 5 (from that initial byte).
  After the group ends (bit7 set on note), the song pointer is updated.
  Next time XT_GameSTART is called, a NEW bVar7 is read.

So the format is:
  [channel_byte] [note, dur] [note, dur] ... [note|0x80, dur]
  [channel_byte] [note, dur] [note, dur] ... [note|0x80, dur]
  ...
  0xFA-0xFF = end

But looking at the data "2b 0d 2b 0d 2b 0d 2b 0d 33 0d":
  If first byte 0x2B is channel_byte: channel = 0x2B >> 5 = 1
  Then notes start: 0x0D=13(C#1) dur=0x2B=43, 0x0D dur=0x2B=43...
  That gives all C#1 notes with dur=43 which is weird.

  OR: 0x2B is channel_byte (ch1), then first note pair is [0x0D, 0x2B]:
  note=0x0D=13, dur=0x2B=43... same thing.

ALTERNATIVE: Maybe there IS no separate channel byte for PC Speaker mode
(single channel). The channel byte only matters for Tandy (4 channels).
For PC Speaker: stream is just [note, dur] pairs directly.

Looking at SOUND_LoadSpeakerSong vs SOUND_LoadTandySong:
- Tandy loads 4 separate channel streams
- Speaker loads only 1 stream (channel 0)

For the speaker stream, the channel assignment is implicit (always ch0).
The stream data is JUST [note, dur] pairs.

Let's try: 0x2B = note, 0x0D = duration
  note 0x2B = 43: oct = 43/12 = 3, pitch = 43%12 = 7 = G
  → G3, duration 13 ticks

  Then 0x33 = 51: oct = 51/12 = 4, pitch = 51%12 = 3 = D#
  → D#4, duration 13 ticks

This makes musical sense! A G drone with occasional melodic notes.
But then bit7 is never set in "2b"... when does the phrase end?

WAIT - for the speaker, maybe bit7 means something different,
or these are ALL channel 0 notes (bit7=0, bits6-5=00, bits4-0=note):
  0x2B = 0010_1011: bits6-5 = 01, bits4-0 = 0x0B = 11 = B
  So note = B, and "channel 1" in the data... but speaker only has 1 channel.

I think the REAL answer is: for Tandy 4-channel mode, this IS an
interleaved multi-channel stream where each [note,dur] targets the
channel indicated by (note_byte >> 5). For PC Speaker mode, it only
processes notes where channel == 0.

Let's decode with that model:
  0x2B = ch1, note B(11) in octave 0? No, 0x0B=11=B0
  0x33 = ch1, note 0x13=19: 19/12=1r7 = G1
  0x9B = ch4?, note 0x1B=27: 27/12=2r3 = D#2... 
  wait ch = 0x9B>>5 = 4? That's > 3.
  
  0x9B: bit7=1, bits6-5=00, bits4-0=11011=27
  AH! If bit7 is the "last" flag, then:
    channel = (byte & 0x60) >> 5 = (0x9B & 0x60) >> 5 = (0x00) >> 5 = 0
    note = byte & 0x1F = 0x1B = 27... but that only gives 32 note values (0-31)

Hmm, that doesn't span enough range either.

OK let me go back to the decompiled code ONE MORE TIME and trace carefully:

  bVar1 = *pbVar8;                    // THIS is the note byte
  if (((bVar1 & 0x7f) != 0x7f) && (bVar7 >> 5 == 0)) {
    // Only process pitch if note != REST and channel == 0
    uVar6 = (bVar1 & 0x7f) + wVar2;  // note + transpose
    // ... calculate frequency from note number
  }

So: bVar1 & 0x7F = note number (0-126, 127=rest)
    bVar7 >> 5 = channel (from the COMMAND byte, not the note byte!)
    bVar1 & 0x80 = continuation flag (checked later)

This means notes CAN be 0-126 (7 bits), and the channel comes from
a SEPARATE command byte read before the loop.

For your data "2b 0d 2b 0d..." to make sense as pure [note,dur] pairs:
  The channel_byte must have been read BEFORE this data.
  Then 0x2B = note 43 (G3), dur 13.
  But bit7 of 0x2B = 0, so it's NOT the last note in the phrase.
  The phrase continues until a note with bit7=1 appears.

Looking further in your data:
  0x9B = 1001_1011: bit7=1 (LAST!), note = 0x1B = 27 (D#2)
  0x0C = duration 12

  0xC3 = 1100_0011: bit7=1 (LAST!), note = 0x43 = 67... 67/12=5r7 = G5
  Hmm, 0xC3 & 0x7F = 0x43 = 67. That seems high.

  0x63 = 0110_0011: bit7=0, note = 0x63 = 99... 99/12=8r3 = D#8
  That's extremely high, probably wrong.

NEW THEORY: Maybe the note byte uses fewer bits for the note, and the
remaining bits encode something else. Looking at XT_GameSTART again:

  The channel selection uses: (bVar7 >> 5) * channelStride
  This is from the INITIAL bVar7 (first byte of stream).
  
  For the NOTE processing:
    if (((bVar1 & 0x7f) != 0x7f) && (bVar7 >> 5 == 0))
  
  This checks if bVar7>>5 == 0, meaning it only sets frequency
  for CHANNEL 0. For channels 1-3, it still counts duration but
  doesn't set pitch via the PIT (Tandy uses different registers).

  The note lookup does:
    uVar6 = (bVar1 & 0x7f) + wVar2;  // note + transpose (wVar2 = pwVar4[9])
    do { octave++; uVar6 -= 12; } while (uVar6 > 11);
    frequency = freqTable[uVar6] >> octave;

So note values ARE 0-126 (7 full bits), and the algorithm handles
arbitrarily large values by dividing by 12 to get octave.

Going back to the data:
  All the "2b 0d" = note 0x2B=43 (G3), dur 13, NOT last
  After every 3 of these, a different note appears:
    0x33 = note 51 (D#4), dur 13  (difference from 43: +8 semitones)
    0x9B = note 27 (bit7 set, so LAST), note&0x7F=0x1B=27 (D#2), dur 12
    ...

AH WAIT. I miscounted. Let me look at the byte positions:
  Offset 0x1F: 2b 0d  (note 43=G3, dur 13, not last)
  Offset 0x21: 2b 0d  (note 43=G3, dur 13, not last)  
  Offset 0x23: 2b 0d  (note 43=G3, dur 13, not last)
  Offset 0x25: 2b 0d  (note 43=G3, dur 13, not last)
  Offset 0x27: 33 0d  (note 51=D#4, dur 13, not last)
  Offset 0x29: 2b 0d  
  Offset 0x2B: 2b 0d
  Offset 0x2D: 2b 0d
  Offset 0x2F: 2b 0c  (note 43=G3, dur 12, not last)
  Offset 0x31: 2b 0d
  Offset 0x33: 2b 0d
  Offset 0x35: 2b 0d
  Offset 0x37: 9b 0c  (note = 0x9B & 0x7F = 0x1B = 27 = D#2, dur 12, LAST!)

So this is ONE BIG phrase of many notes, ending with 0x9B (bit7 set).
The phrase has a G3 pedal tone with occasional other notes mixed in.
After 0x9B, the next cmd byte would be read.

Actually this makes sense for a DRUM/RHYTHM pattern on the PC speaker:
  Rapid-fire G3 beeps (dur=13 ticks each ≈ 54ms at 240Hz timer)
  = a buzz/drone effect with pitch variations for "melody"

Let me decode it properly now.
"""

import struct

NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']

# Complete hex dump from user
RAW_HEX = (
    "00 00 00 00 00 00 00 c0 ff 40 f1 c0 e3 00 d7 40"
    " cb 80 bf c0 b4 80 aa 00 a1 00 98 80 8f 40 87"
    " 2b 0d 2b 0d 2b 0d 2b 0d 33 0d 2b 0d 2b 0d 2b 0d"
    " 2b 0c 2b 0d 2b 0d 2b 0d 9b 0c 2b 0d 2b 0d 2b 0d"
    " c3 0c 2b 0d 2b 0d 2b 0d 63 0c 2b 0d 2b 0d 2b 0d"
    " fb 0c 2b 0d 2b 0d 2b 0d 03 0c 2b 0d 2b 0d 2b 0d"
    " bf 0b 2b 0d 2b 0d 2b 0d db 0b 2b 0d 2b 0d 2b 0d"
    " cf 09 2b 0d 2b 0d 2b 0d 7b 0d 2b 0d 2b 0d 2b 0d"
    " 0f 0a 2b 0d 2b 0d 2b 0d 4f 0a 2b 0d 2b 0d 2b 0d"
    " 57 0b 2b 0d 2b 0d 2b 0d 7f 0b 2b 0d 2b 0d 2b 0d"
    " cf 0a 2b 0d 2b 0d 2b 0d 13 0b 2b 0d 2b 0d 2b 0d"
    " 8f 0a 2b 0d 2b 0d 2b 0d 7f 0e 2b 0d 2b 0d 2b 0d"
    " a3 0d"
)


def note_name(n):
    """Convert note number 0-126 to name."""
    if n == 127 or n == 0x7F:
        return "REST"
    octave = n // 12
    pitch = n % 12
    return f"{NOTE_NAMES[pitch]}{octave}"


def main():
    data = bytes.fromhex(RAW_HEX.replace(' ', '').replace('\n', ''))
    print(f"Data size: {len(data)} bytes\n")

    # === FREQUENCY TABLE ===
    # 12 uint16 LE values starting at offset 7 (after 7 bytes of padding/header)
    print("=== FREQUENCY TABLE ===")
    print("(16-bit fixed-point PIT divisors, octave 0 base values)")
    print("(Actual PIT value = table[note] >> octave)")
    print()
    
    freq_off = 7
    for i in range(12):
        val = struct.unpack_from('<H', data, freq_off + i * 2)[0]
        # These are shifted left. The actual frequency:
        # PIT clock = 1193182 Hz
        # If these need >> 8 to get actual PIT divisor:
        actual_div = val >> 8
        if actual_div > 0:
            hz = 1193182.0 / actual_div
        else:
            hz = 0
        print(f"  {NOTE_NAMES[i]:3s}: 0x{val:04X} "
              f"(÷256 = {actual_div:4d}, ~{hz:.1f} Hz)")
    
    print()

    # === NOTE STREAM ===
    # Starts after the frequency table (offset 7 + 24 = 31 = 0x1F)
    # Format: [note_byte, duration_byte] pairs
    # note_byte: bit7 = last_in_phrase, bits6-0 = note number (0-126)
    # Preceded by a channel_cmd byte that XT_GameSTART reads first.
    #
    # BUT: Looking at the data, offset 0x1F starts with "2b 0d" directly.
    # There's no obvious single command byte. The stream at offset 0x1F
    # seems to be pointed to by the song table, meaning the FIRST byte
    # IS the initial bVar7 (channel byte).
    #
    # So: channel_byte = 0x2B, channel = 0x2B >> 5 = 1
    # Then note stream: 0x0D, 0x2B, 0x0D, 0x2B, ...
    # note=0x0D=13(C#1), dur=0x2B=43
    # note=0x0D=13(C#1), dur=0x2B=43
    # That gives all C#1 with dur=43.
    #
    # OR: maybe for this game the channel byte is stored in the channel
    # struct separately and the stream IS pure [note,dur] pairs.
    # Let's try both interpretations:

    note_off = 0x1F
    
    print("=== INTERPRETATION A: Pure [note, dur] pairs (no channel byte) ===")
    print("  (Channel determined by which table pointer loaded this stream)")
    print()
    
    pos = note_off
    idx = 0
    phrases = 0
    
    while pos + 1 < len(data):
        nb = data[pos]
        dur = data[pos + 1]
        
        if nb >= 0xFA:
            print(f"  END marker 0x{nb:02X} at offset 0x{pos:04X}")
            break
        
        is_last = (nb & 0x80) != 0
        note = nb & 0x7F
        name = note_name(note)
        
        marker = "█" if is_last else "·"
        print(f"  [{idx:3d}] @0x{pos:04X}: {marker} "
              f"note={note:3d} ({name:5s}) dur={dur:3d}"
              f"{'  ← PHRASE END' if is_last else ''}")
        
        pos += 2
        idx += 1
        
        if is_last:
            phrases += 1
            # Next byte would be a new channel/command byte for next phrase
            # (read by the NEXT call to XT_GameSTART)
    
    print(f"\n  Total: {idx} notes in {phrases} phrase(s)")
    print()
    
    # Summarize the melody (just the varying notes, skip the pedal)
    print("=== MELODY SUMMARY (non-pedal notes only) ===")
    pos = note_off
    prev_note = -1
    melody = []
    while pos + 1 < len(data):
        nb = data[pos]
        if nb >= 0xFA:
            break
        dur = data[pos + 1]
        note = nb & 0x7F
        is_last = (nb & 0x80) != 0
        
        if note != 43:  # Skip the G3 pedal drone
            melody.append((note, dur, is_last, pos))
        
        pos += 2
    
    print(f"  Pedal tone: G3 (note 43), appears as constant drone")
    print(f"  Melodic notes ({len(melody)} total):")
    for note, dur, is_last, off in melody:
        name = note_name(note)
        print(f"    @0x{off:04X}: {name:5s} (note {note:3d}) dur={dur:3d}"
              f"{'  LAST' if is_last else ''}")


if __name__ == '__main__':
    main()
