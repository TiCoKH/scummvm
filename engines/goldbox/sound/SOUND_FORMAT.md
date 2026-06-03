# Gold Box Sound Driver — Technical Reference

## Overview

The Gold Box games (Pool of Radiance, Curse of the Azure Bonds, etc.) embed
their sound data directly in the game executable, within the Turbo Pascal
overlay segment that contains the sound driver code. There are no external
sound files.

The driver operates at ~237 Hz via the PC's Programmable Interval Timer (PIT)
and supports two output modes:
- **PC Speaker**: 1 square-wave channel via PIT channel 2
- **Tandy 1000**: 4 voices (3 tone + 1 noise), each with independent
  frequency, volume, vibrato, and envelope control

---

## Memory Map (Pool of Radiance v1.3, segment 18AE)

```
Offset  Size    Description
------  ------  --------------------------------------------------------
0x01D7  1 B     BIOS tick divider (counts 13→0, chains INT 08h at 0)
0x01D8  1 B     Sound busy flag (reentrance guard)
0x01D9  1 B     Channel loop counter (4→0)
0x01DA  1 B     Channel struct size constant (0x30 = 48)
0x01DB  2 B     Watchdog counter (copy-protection, not used in reimpl)
0x01DD  34 B    Protection error message (encoded, not used in reimpl)
0x01FF  2 B     Current sound ID
0x0201  2 B     Current channel pointer (during script exec)
0x0203  2 B     Channel array base pointer (speaker or tandy)
0x0205  2 B     Current frequency table pointer
0x0207  2 B     Script subroutine return address
0x0209  4 B     Per-channel sound IDs (which sound owns each channel)
0x020D  1 B     Speaker enabled flag
0x020E  19 B    Note duration multiplier table (v1.0 only)
0x0221  12 B    Jump table for script commands 0xFA–0xFF (code ptrs)
0x022D  192 B   Speaker channel structs (4 × 48 bytes)
0x02ED  ...     (end of speaker channels)
0x03AD  24 B    Speaker frequency table (12 × uint16 LE)
0x03C5  2 B     Best speaker channel pointer (for output selection)
0x03C7  192 B   Tandy channel structs (4 × 48 bytes)
0x0487  ...     (end of tandy channels)
0x0547  24 B    Tandy frequency table (12 × uint16 LE)
0x055F  168 B   Speaker song pointer table (21 songs × 4 ch × uint16)
0x0607  168 B   Tandy song pointer table (21 songs × 4 ch × uint16)
0x06AF  256 B   Sine vibrato waveform (signed 8-bit, one full cycle)
0x07AF  16 B    Zero waveform (silence/no modulation)
0x07BF  16 B    DC offset waveform (constant 0x78)
0x07CF  256 B   Noise waveform A (pseudo-random, used by sound scripts)
0x08CF  256 B   Noise waveform B (pseudo-random, appears unused)
0x09CF  ~2.5KB  Command stream data (all songs and sound effects)
```

---

## Frequency Tables

Two separate tables exist — one for each output mode.

### Tandy Frequency Table (12 entries at 0x0547)

SN76489 divisor values for octave 0 (pre-shifted left by 2):

```
Index  Note  Value     Hz (approx)
-----  ----  ------    -----------
  0     C    0xFFC0    109
  1     C#   0xF140    115
  2     D    0xE3C0    122
  3     D#   0xD700    130
  4     E    0xCB40    137
  5     F    0xBF80    146
  6     F#   0xB4C0    154
  7     G    0xAA80    163
  8     G#   0xA100    173
  9     A    0x9800    184
 10     A#   0x8F80    195
 11     B    0x8740    206
```

Hardware output: `SN76489_divisor = (value >> 2) & 0x3FF`
Conversion: `Hz = 111860 / (value >> 2)`

### Speaker Frequency Table (12 entries at 0x03AD)

PIT channel 2 divisor values for octave 0:

```
Index  Note  Value     Hz (approx)
-----  ----  ------    -----------
  0     C    0x8E84    33
  1     C#   0x8684    35
  2     D    0x7EF7    37
  3     D#   0x77D7    39
  4     E    0x714F    41
  5     F    0x6AC4    44
  6     F#   0x64C6    46
  7     G    0x5F1E    49
  8     G#   0x59C7    52
  9     A    0x54BD    55
 10     A#   0x4FFC    58
 11     B    0x4B7E    62
```

Hardware output: `PIT_divisor = value >> 8`
Conversion: `Hz = 1193182 / (value >> 8)`

### Octave Shifting (both tables)

Higher octaves computed by right-shifting:
```
frequency = freqTable[note % 12] >> (note / 12)
```

The note-to-frequency lookup in XT_SoundStart:
```asm
; AX = note number (after adding transpose)
; Divide into octave + semitone:
LAB_octave_loop:
    INC CL           ; octave counter (starts at 0xFF, so first INC → 0)
    SUB AX, 12
    JNC LAB_octave_loop
    ADD AX, 12       ; AX = semitone (0-11)
; Lookup:
    BX = AX * 2      ; word index
    BX += freqTablePtr
    AX = [BX]        ; base divisor
    SHR AX, CL       ; shift right by octave count
```

---

## Song Pointer Tables

Two parallel tables of 21 songs x 4 channels:
- **Speaker** at 0x055F (only ch0 used — single voice hardware)
- **Tandy** at 0x0607 (up to 4 channels — 4 voice hardware)

Each entry is a uint16 segment-relative offset pointing directly to a
command stream. The pointer is stored into the channel's `songPtr` field.
When `ticksLeft` reaches 0, `XT_GameSTART` reads commands from `songPtr`.

- Value `0x0000` = channel not used for this song
- The "silence" stream: 0x0D2B (Speaker) / 0x11BF (Tandy)

### How to find a song's data

```
table_offset = 0x055F (Speaker) or 0x0607 (Tandy)
song_entry   = table_offset + songIndex * 8
ch0_ptr      = read_uint16(song_entry + 0)
ch1_ptr      = read_uint16(song_entry + 2)
ch2_ptr      = read_uint16(song_entry + 4)
ch3_ptr      = read_uint16(song_entry + 6)
```

Each pointer IS the start address of the command stream for that channel.
Go to that segment offset and start reading variable-length instructions.

### Song ID mapping

The game calls `SOUND_Play(s_id)`:
- s_id = 0: Stop all, disable speaker
- s_id = 1: Enable speaker (unmute)
- s_id = 0xFF: Stop songs, keep speaker enabled
- s_id = 2+: Load song at table index `(s_id - 2)`

`SOUND_LoadTandySong` / `SOUND_LoadSpeakerSong` receive `(s_id - 1)` as the
table index (0-based).

---

## Command Stream Format

The command stream is a sequence of **variable-length instructions**.
The first byte determines the instruction type and length:

| First Byte | Length | Format | Meaning |
|------------|--------|--------|---------|
| `0xFF` | 4 bytes | `FF reg val_lo val_hi` | Register write |
| `0xFE` | 3 bytes | `FE target_lo target_hi` | Loop back |
| `0xFD` | 3 bytes | `FD param_lo param_hi` | Conditional / duration marker |
| `0xFC` | 3 bytes | `FC target_lo target_hi` | Unconditional jump |
| `0xFB` | 1 byte  | `FB` | End of stream |

### 0xFF — Register Write (4 bytes)

```
FF [register] [value_lo] [value_hi]
```

Writes `value = (value_hi << 8) | value_lo` to the specified channel register.
Execution continues immediately to the next instruction (no delay), UNLESS
the register is DURATION (0x00) — which pauses execution for that many ticks.

### 0xFE — Loop Back (3 bytes)

```
FE [target_lo] [target_hi]
```

Decrements the loop counter. If counter > 0, jumps to segment offset
`target = (target_hi << 8) | target_lo`. Otherwise continues past.

### 0xFD — Conditional / Section Marker (3 bytes)

```
FD [param_lo] [param_hi]
```

Used for multi-section songs. The parameter `(param_hi << 8) | param_lo`
acts as a duration or state marker for the section that follows.
Exact semantics depend on context (may set duration, or act as a
conditional based on tempo/counter state).

### 0xFC — Unconditional Jump (3 bytes)

```
FC [target_lo] [target_hi]
```

Jumps to segment offset `target = (target_hi << 8) | target_lo`.
Used for song restart/loop-to-beginning, or to skip initialization
on repeated plays.

### 0xFB — End of Stream (1 byte)

```
FB
```

Terminates the stream. Channel becomes inactive.

---

## Register Map

| ID   | Name       | Channel Offset | Description |
|------|------------|----------------|-------------|
| 0x00 | DURATION   | +0x00 | Ticks until next command block loads |
| 0x04 | FREQ       | +0x04 | Base frequency (16-bit PIT divisor) |
| 0x06 | FREQ_DELTA | +0x06 | Frequency change per tick (signed, portamento) |
| 0x0A | VOLUME     | +0x0A | Current volume level |
| 0x0C | VOL_DELTA  | +0x0C | Volume change per tick (signed, envelope slope) |
| 0x0E | DELAY_TMR  | +0x0E | Delay timer for envelope reset |
| 0x10 | TEMPO      | +0x10 | Duration multiplier |
| 0x12 | TRANSPOSE  | +0x12 | Note offset for frequency lookup |
| 0x16 | ENV_BASE   | +0x16 | Pointer to envelope sub-stream |
| 0x18 | ENV_OFFSET | +0x18 | Current offset into envelope |
| 0x1A | ENV_TICKS  | +0x1A | Ticks until next envelope entry |
| 0x1C | VIB_TABLE  | +0x1C | Pointer to 256-byte waveform LUT |
| 0x1E | VIB_POS    | +0x1E | Vibrato phase position |
| 0x20 | VIB_SPEED  | +0x20 | Vibrato phase advance per tick |
| 0x22 | VIB_DEPTH  | +0x22 | Vibrato amplitude multiplier |
| 0x24 | VIB_WRAP   | +0x24 | Vibrato cycle wrap point |
| 0x26 | LOOP_CTR   | +0x26 | Primary loop counter |
| 0x28 | LOOP_CTR2  | +0x28 | Secondary loop counter |

### Key register: DURATION (0x00)

When a DURATION command is executed, the channel **pauses** command
processing for that many ticks (~237 Hz). All other registers are
"immediate" — they execute and the stream continues without waiting.

A typical sound is:
1. Set parameters (FREQ, VOLUME, VIB_*, etc.) — all immediate
2. Set DURATION — pauses here while the sound plays
3. After ticks expire, execution resumes

---

## Decoded Example: Song 7 (Tandy, ptr = 0x0C03)

A short sound effect — warbling tone:

```
Addr   Bytes        Decoded
-----  -----------  ------------------------------------------
0C03:  FF 0A 03 00  VOLUME = 3
0C07:  FF 04 B4 3F  FREQ = 0x3FB4
0C0B:  FF 26 1E 00  LOOP_CTR = 30
0C0F:  FF 06 3C FE  FREQ_DELTA = -452 (signed 0xFE3C)
0C13:  FF 00 0C 00  DURATION = 12 ticks ← pause
0C17:  FF 06 77 02  FREQ_DELTA = +631
0C1B:  FF 00 08 00  DURATION = 8 ticks ← pause
0C1F:  FE 0F 0C     LOOP back to 0x0C0F (if LOOP_CTR > 0)
0C22:  FF 0A 00 00  VOLUME = 0 (silence)
0C26:  FF 00 00 00  DURATION = 0
```

**Result:** Oscillating pitch (slide down 12 ticks, slide up 8 ticks)
repeated 30 times. Total duration: ~30 × 20 ticks = 600 ticks ≈ 2.5 sec.

---

## Decoded Example: Song 2 (Tandy, ptr = 0x0C2B)

A noise burst — combat/hit sound:

```
Addr   Bytes        Decoded
-----  -----------  ------------------------------------------
0C2B:  FF 0A 03 00  VOLUME = 3
0C2F:  FF 1C CF 07  VIB_TABLE = 0x07CF (noise waveform)
0C33:  FF 24 00 20  VIB_WRAP = 0x2000
0C37:  FF 22 40 06  VIB_DEPTH = 1600
0C3B:  FF 20 9B 01  VIB_SPEED = 411
0C3F:  FF 04 CC 10  FREQ = 0x10CC
0C43:  FF 06 0A 00  FREQ_DELTA = +10
0C47:  FF 00 1E 00  DURATION = 30 ticks ← noise burst plays
0C4B:  FF 22 84 03  VIB_DEPTH = 900 (reduce vibrato)
0C4F:  FF 04 14 05  FREQ = 0x0514 (higher pitch)
0C53:  FF 06 50 00  FREQ_DELTA = +80
0C57:  FF 00 05 00  DURATION = 5 ticks ← short chirp
0C5B:  FF 0A 00 00  VOLUME = 0
0C5F:  FF 00 00 00  DURATION = 0
```

**Result:** ~126ms noise burst followed by a short high chirp. The noise
waveform randomizes pitch each tick creating white-noise timbre.

---

## Decoded Example: Song 20 (Tandy, ptr = 0x0DA3)

A multi-section **music piece** with vibrato and envelope streams:

```
Addr   Bytes        Decoded
-----  -----------  ------------------------------------------
0DA3:  FC AA 0D     JUMP to 0x0DAA (skip padding)
0DA6:  FF 00 00 00  DURATION = 0 (skipped by jump)
0DAA:  FD 30 00     COND/SECTION param=48
0DAD:  FF 12 E8 FF  TRANSPOSE = -24 (down 2 octaves)
0DB1:  FF 16 37 0E  ENV_BASE = 0x0E37 (envelope stream)
0DB5:  FF 1C AF 06  VIB_TABLE = 0x06AF (sine waveform)
0DB9:  FF 24 00 10  VIB_WRAP = 0x1000
0DBD:  FF 22 28 00  VIB_DEPTH = 40
0DC1:  FF 20 2C 01  VIB_SPEED = 300
0DC5:  FD 60 00     COND/SECTION param=96
0DC8:  FF 12 E8 FF  TRANSPOSE = -24
0DCC:  FF 10 05 00  TEMPO = 5
0DD0:  FF 16 1F 0E  ENV_BASE = 0x0E1F (different envelope)
0DD4:  FF 1C AF 06  VIB_TABLE = sine
0DD8:  FF 24 00 10  VIB_WRAP = 0x1000
0DDC:  FF 22 14 00  VIB_DEPTH = 20 (gentler vibrato)
0DE0:  FF 20 78 00  VIB_SPEED = 120
0DE4:  FD 00 00     COND/SECTION param=0
0DE7:  FF 12 E8 FF  TRANSPOSE = -24
0DEB:  FF 16 4F 0E  ENV_BASE = 0x0E4F
0DEF:  FF 1C AF 06  VIB_TABLE = sine
0DF3:  FF 24 00 10  VIB_WRAP = 0x1000
0DF7:  FF 22 78 00  VIB_DEPTH = 120
0DFB:  FF 20 E4 00  VIB_SPEED = 228
0DFF:  FD 90 00     COND/SECTION param=144
0E02:  FF 12 E8 FF  TRANSPOSE = -24
0E06:  FF 16 67 0E  ENV_BASE = 0x0E67
0E0A:  FF 1C AF 06  VIB_TABLE = sine
0E0E:  FF 24 00 10  VIB_WRAP = 0x1000
0E12:  FF 22 78 00  VIB_DEPTH = 120
0E16:  FF 20 E4 00  VIB_SPEED = 228
0E1A:  FF 0E 01 00  DELAY_TMR = 1
0E1D:  FB           END
```

**Structure:** This is a multi-section musical piece. Each `FD` command
marks a new musical section with its own envelope and vibrato parameters.
The `FD` param likely controls the duration/timing of each section.
Uses sine vibrato for smooth pitch wobble (musical vibrato), multiple
envelope streams for different ADSR shapes per section, and transpose
-24 to shift all notes down 2 octaves.

---

## Envelope Sub-Stream Format

Referenced by `ENV_BASE` register (0x16). The envelope runs in parallel
with the main command stream, controlling volume slope independently.
Located at addresses like 0x0E1F, 0x0E37, 0x0E4F, 0x0E67.

### Format: 4-byte entries (word pairs)

```
[int16 value (LE)]  [int16 duration (LE)]
```

| Duration | Meaning |
|----------|---------|
| > 0      | Set VOL_DELTA = value, wait this many ticks |
| -1 (0xFFFF) | Set VOLUME = value absolutely, continue to next entry |
| -1 + value=0 | Silence channel and stop envelope processing |

### Example (from 0x0E1F area)

```
Bytes          Decoded
-----------    ------------------------------------------
03 00 FF FF    Set VOLUME = 3 (duration=-1: immediate)
00 00 10 00    VOL_DELTA = 0, wait 16 ticks (sustain)
00 00 FF FF    Set VOLUME = 0 (duration=-1: immediate silence)
00 00 00 00    VOL_DELTA = 0, wait 0 (end)
```

### Envelope data regions

From Song 20 analysis, envelope streams are at:
- 0x0E1F — short sustain envelope
- 0x0E37 — envelope with attack
- 0x0E4F — another shape
- 0x0E67 — another shape

These are referenced by the main command stream's `FF 16 xx xx` commands.

---

## Vibrato Waveform Tables (800 bytes at 0x06AF)

The vibrato system reads from 256-byte LUTs to modulate frequency.
Total area: 800 bytes = 256 + 16 + 16 + 256 + 256.

### Available Waveforms

| Address | Size | ID | Type | Character |
|---------|------|----|------|-----------|
| 0x06AF  | 256 B | 0 | Sine | Smooth pitch wobble (musical vibrato) |
| 0x07AF  | 16 B  | 1 | Zero | No modulation (flat zeros) |
| 0x07BF  | 16 B  | 2 | DC   | Constant offset 0x78 (pitch shift without wobble) |
| 0x07CF  | 256 B | 3 | Noise A | Pseudo-random (harsh/percussive timbre) |
| 0x08CF  | 256 B | 4 | Noise B | Pseudo-random (appears unused) |

### Sine Waveform Detail

The 256-byte sine table at 0x06AF is a standard signed 8-bit sine:
- Samples 0–63: rise from 0 to +126 (0x7E)
- Samples 64–127: fall from +126 to 0
- Samples 128–191: fall from 0 to -126 (0x82)
- Samples 192–255: rise from -126 to 0

Peak amplitude: ±126 (not ±127, likely to avoid overflow in multiply).

### Zero and DC Waveforms

These are only 16 bytes each because `vibWrap` is set small when using
them (e.g. 0x0100), so `vibPos >> 4` never exceeds index 15.

- Zero: all 0x00 — effectively disables vibrato
- DC: all 0x78 (+120) — applies a constant positive pitch offset
  scaled by vibDepth. Used for detuning/pitch shift effects.

### Vibrato Calculation (per tick)

```
vibPos += vibSpeed
if (vibPos >= vibWrap) vibPos -= vibWrap
sample = (int8)vibTable[vibPos >> 4]       // signed byte from LUT
vibrato = (sample * 256 * vibDepth) >> 16  // fixed-point multiply
outFreq = freq + vibrato                   // final output
```

**Musical vibrato:** Sine table + small vibDepth (20-120) + moderate speed
**Noise/percussion:** Noise table + large vibDepth (1600+) + high speed
**Pitch shift:** DC table + vibDepth sets offset amount

---

## Playback Pipeline (per tick at ~237 Hz)

For each active channel:

```
1. volume += volDelta              ← smooth envelope
2. freq += freqDelta               ← portamento/pitch bend  
3. vibPos += vibSpeed              ← advance vibrato phase
   vibrato = table[vibPos>>4] * vibDepth >> 16
4. outFreq = freq + vibrato        ← final pitch
5. if (delayTimer-- == 0)          ← one-shot envelope reset
     envOffset = 0, envTicksLeft = 1
6. if (ticksLeft-- == 0)           ← main timer expired
     → read next instruction(s) from stream
     → stops at DURATION command (sets new ticksLeft)
7. if (envTicksLeft-- == 0)        ← envelope timer
     → consume next envelope entry
```

---

## Hardware Output

### PC Speaker
- PIT Channel 2 divisor = `outFreq >> 8`
- Port 0x61 bits 0-1 = speaker gate (from `volume & 3`)
- Only channel 0 is output

### Tandy SN76489 (port 0xC0)
- 4 channels (3 tone + 1 noise)
- Frequency: `(outFreq >> 2) & 0x3FF` (10-bit divisor)
- Attenuation: `(0xFFFF - volume) >> 12` (4-bit, 0=loud 15=silent)

---

## Complete Song Table (Pool of Radiance)

### Speaker (at 0x055F) — single channel only

| Song | Ch0 Ptr | Type |
|------|---------|------|
| 0 | 0x0D2B | Silence (null stream) |
| 1 | 0x0D33 | Sound effect |
| 2 | 0x0C2B | Noise burst (combat hit) |
| 3 | 0x0C9B | Sound effect |
| 4 | 0x0CC3 | Sound effect |
| 5 | 0x0C63 | Sound effect |
| 6 | 0x0CFB | Sound effect |
| 7 | 0x0C03 | Warbling tone (30 cycles) |
| 8 | 0x0BBF | Sound effect |
| 9 | 0x0BDB | Sound effect |
| 10 | 0x09CF | Sound effect |
| 11 | 0x0D7B | Sound effect |
| 12 | 0x0A0F | Sound effect |
| 13 | 0x0A4F | Sound effect |
| 14 | 0x0B57 | Sound effect |
| 15 | 0x0B7F | Sound effect |
| 16 | 0x0ACF | Sound effect |
| 17 | 0x0B13 | Sound effect |
| 18 | 0x0A8F | Sound effect |
| 19 | 0x0E7F | Sound effect (longer) |
| 20 | 0x0DA3 | Music (multi-section with vibrato) |

Speaker ch1-3 ALL point to 0x0D2B (silence) — PC Speaker hardware
only has 1 voice so only ch0 is ever output.

### Tandy (at 0x0607) — multi-channel (4 voices)

| Song | Ch0 | Ch1 | Ch2 | Ch3 |
|------|-----|-----|-----|-----|
| 0 | silence | silence | silence | silence |
| 1 | 0x11C7 | 0x11F3 | 0x121F | silence |
| 2 | silence | silence | silence | 0x11A3 |
| 3 | 0x110B | 0x1107 | 0x1103 | silence |
| 4 | 0x1127 | silence | 0x116B | 0x1183 |
| 5 | 0x10DB | silence | silence | silence |
| 6 | silence | silence | 0x10B3 | 0x10C3 |
| 7 | 0x1087 | 0x1083 | silence | silence |
| 8 | silence | silence | silence | 0x104B |
| 9 | silence | silence | silence | 0x106B |
| 10 | silence | silence | 0x0E97 | 0x0EAF |
| 11 | 0x124B | 0x1263 | 0x127B | silence |
| 12 | silence | silence | 0x0ECF | 0x0EE7 |
| 13 | 0x0F2F | silence | 0x0F07 | 0x0F17 |
| 14 | silence | silence | 0x0FF3 | 0x0FFF |
| 15 | silence | silence | 0x1013 | 0x102B |
| 16 | silence | silence | 0x0F8F | 0x0F9B |
| 17 | silence | silence | 0x0FBF | silence |
| 18 | 0x0F73 | silence | 0x0F4B | 0x0F5B |
| 19 | 0x135C | silence | silence | silence |
| 20 | 0x1293 | silence | silence | silence |

Tandy SN76489 has 4 hardware voices (3 tone + 1 noise). Songs use
2-4 channels for harmonies and layered sound effects.

---

## Stream Address Range

- **First stream address:** 0x09CF (Tandy Song 10 ch0)
- **Last stream address:** 0x135C (Speaker Song 19 ch0)
- **Total data region:** ~0x09CF to ~0x1378 (approx 2473 bytes)

---

## Extracting Song Data — Step by Step

1. **Find the song table pointer:**
   ```
   offset = table_base + songIndex * 8 + channelIndex * 2
   ptr = read_uint16_LE(segment, offset)
   ```

2. **Go to that segment offset.** First byte is an instruction opcode.

3. **Read variable-length instructions** until you hit `0xFB` (end):
   ```
   while not done:
     opcode = segment[ptr]
     
     switch opcode:
       case 0xFF:  // Register write
         reg = segment[ptr+1]
         val = segment[ptr+2] | (segment[ptr+3] << 8)
         ptr += 4
         process_register(reg, val)
         
       case 0xFE:  // Loop back
         target = segment[ptr+1] | (segment[ptr+2] << 8)
         ptr += 3
         if (loop_counter > 0) { loop_counter--; ptr = target; }
         
       case 0xFD:  // Conditional/section
         param = segment[ptr+1] | (segment[ptr+2] << 8)
         ptr += 3
         process_conditional(param)
         
       case 0xFC:  // Jump
         target = segment[ptr+1] | (segment[ptr+2] << 8)
         ptr += 3
         ptr = target
         
       case 0xFB:  // End
         done = true
   ```

---

---

## Music Note Stream Format (v1.0)

In Pool of Radiance v1.0, the music tracks (songs 12-19) use a different
format from the register command streams. Instead of `FF reg val` synthesis
commands, they use a **note-based format** processed by XT_GameSTART.

Both Speaker and Tandy music share the same basic structure but differ in
complexity (speaker = multi-channel interleaved, tandy = single channel).

### Header (3 bytes)

```
[FC] [param1] [param2]
```

- `FC` = music track marker
- `param1 param2` = format/loop parameters (differ by output mode):
  - Speaker music: `FB 13` (tempo multiplier = 0x13 = 19)
  - Tandy music: `89 20` (loop address 0x2089, or tempo = 0x20 = 32)

### Note Data Format

After the 3-byte header, the data consists of note **groups**.
Each group represents one time-slice of playback.

#### Speaker Music: Multi-channel Interleaved Groups

Speaker music interleaves multiple channel voices into a single stream.
Format:

```
[cmd1] [note1, dur1] [cmd2] [note2, dur2] ... [noteN|0x80, durN]
```

Byte-by-byte execution:

```c
bVar7 = *stream++;              // read command byte
while (true) {
    channel = bVar7 >> 5;       // target channel (0-3)
    note = *stream;             // note byte
    dur  = *(stream + 1);      // duration byte
    stream += 2;
    
    set_frequency(channel, note & 0x7F);
    caller.ticksLeft = dur;
    
    if (note & 0x80) break;    // LAST note in group
    bVar7 = *stream++;         // read next command byte
}
```

Byte layout:
- **Command byte** (between note pairs): `bits 7-5 = target channel`
- **Note byte**: `bit 7 = last flag`, `bits 6-0 = note number (0-126, 127=rest)`
- **Duration byte**: raw tick count (0-255)
- **0xFF as note byte**: REST + LAST (note=127, bit7=1)

#### Tandy Music: Single-channel Pure Note Pairs

Tandy music tracks only use channel 0, so there are NO command bytes
between notes. The data is pure `[note, duration]` pairs:

```
[note1|0x80, dur1] [note2|0x80, dur2] ... 
```

Since it's monophonic, almost every note has bit 7 set (LAST = only note
in its group). Occasional chords appear when bit 7 is clear:

```
[noteA, durA] [noteB|0x80, durB]   ← 2-note chord (A not last, B last)
```

### Note Number to Frequency

```c
noteNum = (note & 0x7F) + channel.transpose;
octave = 0;
while (noteNum > 11) { noteNum -= 12; octave++; }
frequency = freqTable[noteNum] >> octave;
```

### Decoded Example: Tandy Song 13, v1.0 (at 127F:18DC)

Header: `FC 89 20`

```
Addr   Bytes    Note        Dur  Last?
-----  ------   ----        ---  -----
+003:  B7 06    G4  (55)     6   LAST
+005:  B5 06    F4  (53)     6   LAST
+007:  B7 06    G4  (55)     6   LAST
+009:  B5 06    F4  (53)     6   LAST
+00B:  B7 06    G4  (55)     6   LAST
+00D:  B5 06    F4  (53)     6   LAST
+00F:  B8 06    G#4 (56)     6   LAST
+011:  BA 06    A#4 (58)     6   LAST
+013:  BC 06    C5  (60)     6   LAST
+015:  BE 06    D5  (62)     6   LAST
+017:  BF 06    D#5 (63)     6   LAST
+019:  C1 06    F5  (65)     6   LAST
+01B:  C3 06    G5  (67)     6   LAST
+01D:  C1 06    F5  (65)     6   LAST
+01F:  C3 06    G5  (67)     6   LAST
+021:  C1 06    F5  (65)     6   LAST
+023:  C3 06    G5  (67)     6   LAST
+025:  C1 06    F5  (65)     6   LAST
+027:  C4 06    G#5 (68)     6   LAST
+029:  C6 06    A#5 (70)     6   LAST
+02B:  C8 06    C6  (72)     6   LAST
+02D:  CA 06    D6  (74)     6   LAST
+02F:  CB 06    D#6 (75)     6   LAST
+031:  CD 51    F6  (77)    81   LAST   ← long note
+033:  4A 31    D5  (74)    49   not last ← chord start
+035:  48 06    C5  (72)     6   not last
+037:  B7 06    G4  (55)     6   LAST   ← chord end (3 notes)
```

**Musical content:** G4/F4 trill → ascending chromatic scale → F5/G5 trill
→ another ascending run → long F6 → D5+C5+G4 chord. Classic RPG fanfare.

### Decoded Example: Speaker Song 16, v1.0 (at 127F:1021)

Header: `FC FB 13` (tempo=19)

```
Addr   Bytes         Ch  Note        Dur  Last?
-----  -----------   --  ----        ---  -----
+003:  09            ch0
+004:  2C 29         ch0  G#3 (44)   41   no
+006:  48            ch2
+007:  49 CB         ch2  C#6 (73)  203   no
+009:  0C            ch0
+00A:  2C 4B         ch0  G#3 (44)   75   no
+00C:  4C            ch2
+00D:  2B C9         ch2  G3  (43)  201   no
+00F:  46            ch2
+010:  CE 09         ch2  F#6 (78)    9   LAST ←

+012:  2C            ch1
+013:  29 48         ch1  F3  (41)   72   no
+015:  49            ch2
+016:  CB 0C         ch2  D#6 (75)   12   LAST ←

+018:  2C            ch1
+019:  4B 4C         ch1  D#6 (75)   76   no
+01B:  2B            ch1
+01C:  C9 46         ch1  C#6 (73)   70   LAST ←
```

**Musical content:** Interleaved multi-channel melody. Ch0 provides bass
notes (G#3, G3), ch1 mid-range (F3, D#6, C#6), ch2 high melody (C#6, F#6).
The speaker rapidly alternates between voices to simulate polyphony.

---

## v1.0 Memory Map (segment 127F)

```
Offset  Size    Description
------  ------  --------------------------------------------------------
0x00A9  1 B     Channel loop counter
0x00AA  1 B     Channel struct size constant (0x30)
0x00AD  2 B     Current channel pointer
0x00AF  2 B     Channel array base pointer
0x00B1  2 B     Current frequency table pointer
0x00B3  2 B     Script subroutine return address
0x00B5  4 B     Per-channel sound IDs
0x00B9  1 B     Speaker enabled flag
0x00BA  19 B    Note duration multiplier table
0x00CD  12 B    Jump table for script commands 0xFA–0xFF
0x040B  160 B   Speaker song pointer table (20 songs × 4 ch × uint16)
0x04AB  160 B   Tandy song pointer table (20 songs × 4 ch × uint16)
0x????  24 B    Speaker frequency table (12 × uint16 LE)
0x????  24 B    Tandy frequency table (12 × uint16 LE)
0x????  256 B   Sine vibrato waveform
0x????  16 B    Zero waveform
0x????  16 B    DC waveform
0x????  256 B   Noise waveform A
0x????  256 B   Noise waveform B
0x086B  ~1.7KB  Speaker command/note stream data
0x149D  ~2.8KB  Tandy command/note stream data
```

### v1.0 Song Organization

| Songs | Type | Speaker Format | Tandy Format |
|-------|------|----------------|---------------|
| 0-11 | Sound effects | Register commands (`FF reg val`) | Multi-ch register commands |
| 12-19 | Music tracks | Note stream (multi-ch interleaved) | Note stream (single-ch) |

---

## Version Comparison

| | v1.0 (segment 127F) | v1.3 (segment 18AE) |
|---|---|---|
| Songs | 20 | 21 |
| Speaker table | 0x040B (80 words) | 0x055F (84 words) |
| Tandy table | 0x04AB (80 words) | 0x0607 (84 words) |
| Speaker silence | 0x0A17 | 0x0D2B |
| Tandy silence | 0x1649 | 0x11BF |
| SFX format | Register commands | Register commands |
| Speaker music | Note stream (`FC FB` header, multi-ch) | Register commands (FD sections) |
| Tandy music | Note stream (`FC 89` header, single-ch) | Register commands |
| Music songs | 12-19 (8 tracks) | Song 20 only (1 track?) |

### Key Differences

1. **v1.0 has 8 dedicated music tracks** (songs 12-19) separate from SFX
2. **v1.0 music uses note-based format** — simpler, more like traditional
   tracker/sequencer data with note numbers and durations
3. **v1.3 consolidated to register command format** — music uses the same
   synthesis command language as SFX, with FD section markers for structure
4. **v1.0 Tandy music is monophonic** (only ch0) despite having 4 hardware
   voices available; v1.3 Tandy SFX uses multiple channels
5. **v1.0 uses a note duration table** (19 bytes at 0x00BA) that maps 5-bit
   note indices to duration multipliers; v1.3 sets all durations explicitly
   via `FF 00 xx xx` register commands
6. **Both versions have separate speaker/tandy frequency tables** — the
   divisor values differ due to different clock rates (PIT vs SN76489)

---

## Two Stream Formats Summary

### Format A: Register Command Stream (SFX + v1.3 music)

```
Identified by: First byte is 0xFF (register write) or 0xFC (jump)
Used for: Sound effects (all versions), music (v1.3 only)
Structure: Variable-length opcodes (FF/FE/FD/FC/FB)
Processed by: Command interpreter in XT_GameSTART
```

### Format B: Note Stream (v1.0 music only)

```
Identified by: Starts with FC FB xx (speaker) or FC 89 xx (tandy)
Used for: Music tracks in v1.0 (songs 12-19)
Structure: 3-byte header + [cmd] [note, dur] groups (speaker)
           3-byte header + [note, dur] pairs (tandy)
Processed by: Note decoder in XT_GameSTART (different code path)
```

### How to Distinguish at Runtime

```c
first_byte = *songPtr;
if (first_byte == 0xFC) {
    second_byte = *(songPtr + 1);
    if (second_byte == 0xFB || second_byte == 0x89) {
        // Format B: Note stream music
        tempo = *(songPtr + 2);
        noteData = songPtr + 3;
    } else {
        // Format A: FC is a jump command
        jumpTarget = read_uint16(songPtr + 1);
    }
} else {
    // Format A: Register command stream (starts with FF usually)
}
```

---

## Note Duration Table (v1.0, 19 bytes at 0x00BA)

Used in v1.0's `XT_SoundStart` for computing note duration from the
compact note encoding. The low 5 bits of the note byte index into this
table, and the result is multiplied by `volumeScale` (field +0x0E).

```
Index  Value  Rhythm (approx)
-----  -----  ---------------
  0    0x00   rest / unused
  1    0x00   unused
  2    0x00   unused
  3    0x02   1/32 note
  4    0x00   unused
  5    0x03   dotted 1/32
  6    0x04   1/16 note
  7    0x00   unused
  8    0x06   dotted 1/16
  9    0x08   1/8 note
 10    0x00   unused
 11    0x0C   dotted 1/8
 12    0x10   1/4 note
 13    0x00   unused
 14    0x18   dotted 1/4
 15    0x20   1/2 note
 16    0x00   unused
 17    0x30   dotted 1/2
 18    0x40   whole note
```

The pattern: powers of 2 (2,4,8,16,32,64) with dotted values (×1.5)
interleaved. Zero entries are unused/reserved rhythm slots.

Usage in v1.0:
```asm
AND  BX, 0x1F              ; low 5 bits of note byte
MUL  byte ptr [BX + 0xBA]  ; multiply volumeScale × durationTable[index]
MOV  [SI], AX              ; store as channel duration
```

Not used in v1.3 — all durations are explicit `FF 00 xx xx` commands.

---

## Channel Struct Layout (48 bytes, 0x30)

Each of the 8 channel slots (4 speaker + 4 tandy) uses this layout:

```
Offset  Size  Field           Description
------  ----  -----------     ----------------------------------------
+0x00   2     duration        Ticks remaining for current note
+0x02   2     scriptPtr       Pointer to sound script bytecode
+0x04   2     frequency       Current base frequency (PIT/PSG divisor)
+0x06   2     freqDelta       Frequency increment per tick
+0x08   2     outputFreq      Final output frequency (freq + vibrato)
+0x0A   2     volume          Current volume level
+0x0C   2     volumeDelta     Volume change per tick (envelope slope)
+0x0E   2     volumeScale     Volume/duration scaling factor
+0x10   2     field10         Tempo multiplier (v1.0) / reserved
+0x12   2     noteOffset      Transpose (semitones added to note)
+0x14   2     sustainCount    Sustain counter / delay timer
+0x16   2     envelopeBase    Base pointer to envelope table
+0x18   2     envelopeOfs     Current offset into envelope table
+0x1A   2     envelopeTicks   Ticks until next envelope step
+0x1C   2     vibratoBase     Vibrato waveform base address
+0x1E   2     vibratoPos      Current position (fixed-point phase)
+0x20   2     vibratoSpeed    Phase advance per tick
+0x22   2     vibratoDepth    Amplitude multiplier
+0x24   2     vibratoPeriod   Phase wrap-around length
+0x26   2     loopCounter     Primary loop counter
+0x28   2     loopCounter2    Secondary loop counter
+0x2A   6     reserved        Unused padding to 48 bytes
```

Channel arrays in memory:
- Speaker: 4 channels at 0x022D (v1.3) — only ch0 output to hardware
- Tandy: 4 channels at 0x03C7 (v1.3) — all 4 output to SN76489

---

## Timer Architecture

The driver hooks INT 08h (PIT timer) with a custom ISR running at ~237 Hz.

### Timer Divisor

PIT channel 0 reprogrammed with divisor `0x13B1` (5041):
```
Frequency = 1193182 / 5041 ≈ 236.7 Hz
Period ≈ 4.22 ms per tick
```

### BIOS Tick Chaining

The ISR maintains a divider counting down from 13 (0x0D). Every 13th
custom tick, it chains to the original INT 08h handler to maintain the
normal 18.2 Hz BIOS tick rate:
```
237 Hz / 13 ≈ 18.2 Hz (matches standard BIOS timer)
```

### ISR Execution Order

```
1. Check/decrement watchdog counter (copy protection — ignore)
2. Check reentrance guard; if busy, skip sound update
3. Set busy flag
4. Call TIMER_SetSpeakerDMA (advance all speaker channels)
5. Call TIMER_SetTandyDMA (advance all tandy channels)
6. Clear busy flag
7. Decrement BIOS divider; if zero:
   - Reset divider to 13
   - Chain to original INT 08h (JMP FAR)
8. Otherwise: send EOI (0x20 to port 0x20) and IRET
```

---

## PC Speaker Output Selection

Since the PC Speaker has only one voice, `TIMER_SetSpeakerDMA` must
select the "best" channel when multiple are active:

```
For each of 4 speaker channels:
    Call SOUND_AdvanceChannel()
    If channel has non-zero volume AND non-zero frequency:
        Remember as bestChannel (last one wins)

If bestChannel exists:
    Write outputFreq to PIT channel 2 (ports 0x42)
    Set port 0x61 bits 0-1 from (volume & 3)
Else:
    Clear port 0x61 bits 0-1 (speaker off)
```

---

## Waveform Pointer Resolution

In the original binary, sound scripts set the vibrato waveform via:
```
FF 1C xx xx    ; REG_VIB_WAVE = near pointer into segment
```

These are raw segment-relative addresses:
| Pointer | Waveform |
|---------|----------|
| 0x06AF  | Sine     |
| 0x07AF  | Zero     |
| 0x07BF  | DC bias  |
| 0x07CF  | Noise A  |
| 0x08CF  | Noise B  |

For the ScummVM reimplementation, these raw pointers must be converted
to waveform indices (0–4). Two approaches:

### Approach A: Patch at extraction time

When extracting channel data from the binary, replace raw pointer values
with waveform IDs:
```python
WAVE_MAP = {0x06AF: 0, 0x07AF: 1, 0x07BF: 2, 0x07CF: 3, 0x08CF: 4}
if reg == 0x1C:
    value = WAVE_MAP[value]
```

### Approach B: Map on-the-fly in the driver

Keep channel data as raw bytes. When the driver processes a `REG_VIB_WAVE`
command, map the original pointer to an index at runtime:
```cpp
case SoundCommand::REG_VIB_WAVE: {
    static const uint16 kWaveOffsets[] = {
        0x06AF, 0x07AF, 0x07BF, 0x07CF, 0x08CF
    };
    ch.vibWaveId = 0;
    for (int i = 0; i < 5; ++i) {
        if (uval == kWaveOffsets[i]) {
            ch.vibWaveId = i;
            break;
        }
    }
    break;
}
```

Approach B is simpler for porting — dump channel data verbatim from the
binary and let the driver handle the pointer→index translation. No need
to patch extracted data. Same principle applies to `REG_ENV_STREAM` (0x16)
which stores envelope stream pointers that need similar mapping.

---

## Verification Checklist

To verify correct extraction, check that:
- SFX streams begin with `FF 0A 03 00` (VOLUME = 3) or `FF 0A` variant
- Music streams (v1.0) begin with `FC FB xx` or `FC 89 xx`
- Silence streams are very short
- All loop/jump targets point to valid addresses within the data region
- In register streams: FREQ values are in range 0x0100-0xFFFF
- In register streams: VIB_TABLE values are known waveform addresses
- In register streams: ENV_BASE values point into envelope data area
- SFX streams end with `FB`
- In note streams: note values (& 0x7F) are in range 0-78 (reasonable pitches)
- In note streams: duration values are typically < 0x80 (0-127)
- In note streams: bytes with bit 7 set in note position are "last" markers
- In speaker note streams: command bytes between notes have (byte>>5) in 0-3
