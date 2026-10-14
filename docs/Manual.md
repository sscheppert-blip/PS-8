# PS-8 – Manual

*Deutsch: [Handbuch](Handbuch.md)*

![PS-8 front panel](images/panel.png)

PS-8 is an analog-sounding percussion synthesizer with a built-in step sequencer. It has no
drum samples: every sound – kick, tom, snare-like noise, metal, zaps, bass lines – is built from two
oscillators, noise, a filter and three envelopes. On top of that, the sequencer plays its own pitch
and level on every step.

This manual explains every control, gives sound recipes and lists all parameters.

---

## Contents

1. [Installation](#1-installation)
2. [Getting started](#2-getting-started)
3. [How the sound is made](#3-how-the-sound-is-made)
4. [The front panel](#4-the-front-panel)
5. [Sequencer](#5-sequencer)
6. [Sequence, LFO and Status](#6-sequence-lfo-and-status)
7. [MIDI](#7-midi)
8. [Sound recipes](#8-sound-recipes)
9. [Tips and tricks](#9-tips-and-tricks)
10. [Parameter list](#10-parameter-list)
11. [Specifications](#11-specifications)
12. [Troubleshooting](#12-troubleshooting)

---

## 1. Installation

PS-8 comes as **AU** (macOS only), **VST3** and **CLAP** for macOS, Windows and Linux.
Ready-made packages are under **Releases** in the repository.

| System | Folder |
|---|---|
| macOS AU | `~/Library/Audio/Plug-Ins/Components/` |
| macOS VST3 | `~/Library/Audio/Plug-Ins/VST3/` |
| macOS CLAP | `~/Library/Audio/Plug-Ins/CLAP/` |
| Windows VST3 | `C:\Program Files\Common Files\VST3\` |
| Windows CLAP | `C:\Program Files\Common Files\CLAP\` |
| Linux VST3 / CLAP | `~/.vst3/` / `~/.clap/` |

Then let your DAW rescan its plugins. PS-8 shows up as an instrument by the manufacturer
**sscheppert-blip**.

---

## 2. Getting started

1. Load PS-8 on a **MIDI/instrument track**.
2. Press **Play** in your DAW. PS-8 follows the song – **RUN** and **HOST** are on by default.
3. An even 8-step pattern plays. Now turn some knobs:
   - **OSCILLATOR 1 → FREQUENCY**: base pitch
   - **OSCILLATOR 1 → EG AMOUNT**: how far the pitch drops on every hit ("boom")
   - **FILTER → CUTOFF**: bright / dull
   - **DECAY → VCA**: short / long
4. The **PITCH** knobs in the sequencer give each step its own pitch, **VELOCITY** its own level.

**General operation**

| Action | Result |
|---|---|
| Drag a knob up/down | Change the value (a small readout shows the value while dragging) |
| **Double-click** or **Alt + click** a knob | Back to its default |
| Click a toggle switch | Flip it (three-way switch: upper half = up, lower half = down) |
| Drag the window corner | Resize (the aspect ratio is kept) |

---

## 3. How the sound is made

```
 VCO 1 ──┬──────────────┐
         │ FM           ├─► MIXER ─► FILTER ─► DRIVE ─► VCA ─► OUTPUT
 VCO 2 ◄─┘ (+ SYNC)     │            ▲                  ▲
 NOISE ─────────────────┘            │                  │
                                     │                  │
 PITCH DECAY ─► pitch of VCO 1/2     │                  │
 FILTER DECAY ───────────────────────┘                  │
 VCA DECAY ─────────────────────────────────────────────┘
 SEQUENCER ─► trigger + pitch + level per step
 LFO ─► pitch, VCO 2, cutoff, FM or volume
```

Every sequencer step (or matching MIDI note) **fires all three envelopes at once**. The envelopes
jump to full and then decay – how long is set by the three **DECAY** knobs, how strongly they act by
the **EG AMOUNT** knobs.

---

## 4. The front panel

### Top bar

| Control | Function |
|---|---|
| **OUTPUT** (meter) | Output level: left −20 dB, centre 0 dB, right +3 dB |
| **LOAD** (meter) | CPU load of the plugin in percent |
| **TEMPO BPM** (nixies) | Current tempo – PS-8's own or the DAW's |
| **RUN** (green lamp) | Lit while the sequencer runs |
| **STEP** (nixies) | Last step played |
| **SCRAM** | Emergency stop: the sequencer stops (RUN switches off), the sound is cut at once, the step lamps flash red briefly |

### OSCILLATOR 1 and OSCILLATOR 2

| Control | Function |
|---|---|
| **FREQUENCY** | Base pitch, about 8 Hz – 8 kHz (from a low rumble to a high whistle) |
| **EG AMOUNT** | Pitch envelope. **Centre = off**. Right: the pitch starts higher and falls (kick, tom). Left: it starts lower and rises (zaps). Up to ±5 octaves |
| **WAVE** SQR / TRI | Square (hollow, strong) or triangle (soft, round) |
| **LEVEL** | Level in the mixer |

How fast the pitch envelope decays is set by **DECAY → PITCH**. It acts on both oscillators, each with
its own EG AMOUNT.

### MODULATION

| Control | Function |
|---|---|
| **FM 1>2 AMOUNT** | VCO 1 modulates the frequency of VCO 2. A little: rougher, more overtones. A lot: metallic, bell-like, noise |
| **FM** ON / OFF | Switch FM on and off quickly without moving the knob |
| **HARD SYNC** ON / OFF | VCO 2 restarts with every cycle of VCO 1. With the pitch envelope on VCO 2 this sounds cutting and "talking" |

### NOISE

| Control | Function |
|---|---|
| **LEVEL** | Noise level in the mixer (snare, hi-hat, attack click) |
| **COLOR** WHITE / PINK | White noise (hissing) or pink noise (darker) |
| **VCF MOD** | Noise modulates the filter frequency – makes the sound grainy and dirty |

### FILTER

| Control | Function |
|---|---|
| **CUTOFF** | Filter frequency, about 20 Hz – 20 kHz |
| **RESONANCE** | Emphasises the region around the cutoff. Fully right, the filter whistles on its own (even without oscillators) |
| **EG AMOUNT** | Filter envelope. Centre = off, right opens the filter on every hit, left closes it. Up to ±8 octaves |
| **MODE** LOW / HIGH | Low pass (bass stays, highs go) or high pass (highs stay, bass goes) – both 24 dB/octave |

### DECAY

| Control | Function |
|---|---|
| **PITCH** | Decay time of the pitch envelope |
| **FILTER** | Decay time of the filter envelope |
| **VCA** | Decay time of the level – i.e. the length of the sound |
| **VCA ATTACK** FAST / SLOW | Hard attack or soft onset (about 12 ms) |
| **DRIVE** | Saturation/distortion after the filter |

All decays range from about 3 ms to 10 s.

### OUTPUT

| Control | Function |
|---|---|
| **VOLUME** | Output level |
| **CLIP** | Lights red when the output goes above 0 dB – turn VOLUME down |

### PATCH

The jacks have **no function yet** in this version (stamped "OFFLINE"). Patching follows in a later
version.

---

## 5. Sequencer

| Control | Function |
|---|---|
| **PITCH** (top row) | Pitch per step, ±24 semitones. Centre = 0 |
| **VELOCITY** (middle row) | Level per step |
| **Lamp buttons** (STEP ON) | Step on/off. **Green** = on, **dark** = off, **amber** = playing. Steps beyond the set length are dimmed |
| **TEMPO** | Own tempo 20–300 BPM (only with **INT**) |
| **RUN / STOP** | Sequencer on/off |
| **INT / HOST** | **INT**: own tempo, starts right away with RUN. **HOST**: runs only while the DAW plays and is locked to the song position |
| **PITCH TO** 1+2 / OFF / VCO 2 | Where the step pitch goes: both oscillators, nowhere, or VCO 2 only |
| **TRIG** | Play the current step again |
| **ADV** | Advance one step and play it |
| **RESET** | The next step is step 1 again |

**HOST mode in detail:** the steps follow the position in the song, counted from the song start. With
8 steps of 1/16, step 1 falls on every half note; with 16 steps on every bar. When you jump in the DAW
or a loop wraps, the sequencer follows. **RESET** moves the start to the next step.

**Velocity and level:** a step's velocity is the peak of the VCA envelope. Velocity 0 is silent – just
like a step switched off.

---

## 6. Sequence, LFO and Status

This row holds extras. By default everything is neutral: 8 steps, no scale, no glide, LFO depth 0.
A small readout under each knob shows its current value.

### SEQUENCE

| Control | Function |
|---|---|
| **RATE** | Note value per step: 1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32 (T = triplet) |
| **LENGTH** | Number of steps, 1–16. Odd lengths (5, 7, 11 …) run against the bar |
| **SCALE** | Snaps the PITCH knobs: **OFF** (continuous), **CHROMATIC** (semitones), **MAJOR**, **MINOR**, **PENTATONIC** (minor pentatonic). The root is always the set FREQUENCY |
| **GLIDE** | The pitch slides from step to step instead of jumping, up to 1 s. Fully left = off |

### LFO

| Control | Function |
|---|---|
| **RATE** | Speed, 0.05–40 Hz. With **SYNC** the same knob picks a note value: 4 bars, 2 bars, 1 bar, 1/2, 1/4, 1/8, 1/16, 1/32 |
| **SHAPE** | Triangle, square, falling saw, rising saw, sample & hold (random steps) |
| **DEPTH** | Amount. 0 = off |
| **TARGET** | **PITCH** (both VCOs, up to ±2 octaves), **VCO 2** (VCO 2 only), **CUTOFF** (up to ±4 octaves), **FM** (FM amount), **VOLUME** (tremolo) |
| **FREE / SYNC** | Free running or in time with the DAW (with HOST and the DAW playing, the LFO is locked to the song position) |

### STATUS

| Control | Function |
|---|---|
| **STEPS** 1-8 / 9-16 | Which eight steps the sequencer below shows |
| **HOST LOCK** | Lit while PS-8 is locked to the DAW |
| **MIDI IN** | Flashes on every incoming note |
| **LFO** | Flashes with the LFO (when DEPTH > 0) |
| **STEPS 9-16** | Lit while the sequencer plays steps 9–16 – even if you are looking at 1–8 |

---

## 7. MIDI

PS-8 listens on all MIDI channels.

| Note | Function |
|---|---|
| **C1 … D#2** (36–51) | Play step 1 … 16 directly. Level = note velocity × step velocity |
| **E2** (52) | Next step |
| **F2** (53) | Current step again |
| **F#2** (54) | Back to step 1 |

The notes also work while the sequencer runs – you then play on top of the pattern.

**Your own rhythms via MIDI:** switch **RUN** off and play the steps from a MIDI clip. Each step then
works like a drum pad with its own pitch – this way patterns can have any length.

**Automation:** every knob and switch can be automated in the DAW (names in the parameter list).

---

## 8. Sound recipes

Start from a freshly loaded plugin each time. Clock positions refer to the knob position
(7 o'clock = fully left, 12 o'clock = centre, 5 o'clock = fully right).

**Deep kick**
- VCO 1: FREQUENCY 9 o'clock, EG AMOUNT 2 o'clock, WAVE TRI, LEVEL full
- VCO 2 LEVEL and NOISE LEVEL fully left
- FILTER: CUTOFF 1 o'clock, RESONANCE 9 o'clock, EG AMOUNT centre
- DECAY: PITCH 9 o'clock, VCA 12 o'clock

**Tom melody**
- Like the kick, but VCO 1 FREQUENCY 11 o'clock, EG AMOUNT 1 o'clock
- SCALE to MINOR, set the steps' PITCH knobs to different positions

**Snare-like**
- VCO 1: FREQUENCY 11 o'clock, EG AMOUNT 1 o'clock, LEVEL half
- NOISE: LEVEL 2 o'clock, COLOR WHITE
- FILTER: MODE HIGH, CUTOFF 11 o'clock
- DECAY: PITCH 8 o'clock, VCA 10 o'clock

**Hi-hat / metal**
- VCO 1 and VCO 2 on SQR, FREQUENCY both around 3 o'clock (slightly apart)
- FM 1>2 AMOUNT 2 o'clock, NOISE LEVEL 12 o'clock
- FILTER: MODE HIGH, CUTOFF 2 o'clock
- DECAY: VCA 8 o'clock (short) – 11 o'clock for open hats

**Laser zap**
- VCO 1: FREQUENCY 1 o'clock, EG AMOUNT fully right
- DECAY: PITCH 10 o'clock, VCA 10 o'clock
- RESONANCE 2 o'clock

**Acid line**
- VCO 1 on SQR, FREQUENCY 10 o'clock, EG AMOUNT centre
- FILTER: CUTOFF 9 o'clock, RESONANCE 3 o'clock, EG AMOUNT 2 o'clock
- DECAY: FILTER 10 o'clock, VCA 1 o'clock
- SCALE MINOR, GLIDE 9 o'clock, spread the PITCH knobs, LENGTH 16

**Moving groove**
- Any sound, plus LFO: TARGET CUTOFF, SHAPE TRIANGLE, SYNC, RATE 1 BAR, DEPTH 12 o'clock
- Or TARGET PITCH, SHAPE S&H, RATE 1/16, DEPTH 9 o'clock for random pitches

---

## 9. Tips and tricks

- **Polyrhythms:** set LENGTH to 5 or 7 – the pattern shifts against the bar and only repeats after
  many bars.
- **Velocity as tone colour:** short VCA decays plus different velocities give ghost notes and accents.
- **Filter as oscillator:** VCO and noise LEVEL to 0, RESONANCE fully right – the filter whistles on
  its own and is played by the filter envelope.
- **Two sounds in one:** PITCH TO on **VCO 2**: VCO 1 stays put as the kick foundation, VCO 2 plays
  the melody on top.
- **Several instances:** load one PS-8 each for kick, snare and hats – every instance is one voice,
  just like the hardware.
- **Panic:** **SCRAM** stops everything at once when the resonance gets out of hand. **RUN** starts
  it again.

---

## 10. Parameter list

These are the parameter names in your DAW's automation.

| Name | Range | Default |
|---|---|---|
| VCO 1 Frequency | 8 Hz – 8.2 kHz | 64 Hz |
| VCO 1 EG Amount | −100 … +100 % | +45 % |
| VCO 1 Wave | Triangle / Square | Triangle |
| VCO 1 Level | 0–100 % | 85 % |
| VCO 2 Frequency | 8 Hz – 8.2 kHz | 181 Hz |
| VCO 2 EG Amount | −100 … +100 % | +30 % |
| VCO 2 Wave | Triangle / Square | Square |
| VCO 2 Level | 0–100 % | 35 % |
| FM 1-2 Amount | 0–100 % | 0 % |
| FM | Off / On | On |
| Hard Sync | Off / On | Off |
| Noise Level | 0–100 % | 10 % |
| Noise Color | White / Pink | White |
| Noise VCF Mod | 0–100 % | 0 % |
| Cutoff | 20 Hz – 20.5 kHz | about 905 Hz |
| Resonance | 0–100 % | 30 % |
| VCF EG Amount | −100 … +100 % | +40 % |
| Filter Mode | Low Pass / High Pass | Low Pass |
| Pitch Decay | 3 ms – 10 s | about 34 ms |
| VCF Decay | 3 ms – 10 s | about 77 ms |
| VCA Decay | 3 ms – 10 s | about 173 ms |
| VCA Attack | Fast / Slow | Fast |
| Drive | 0–100 % | 0 % |
| Volume | 0–100 % | 70 % |
| Tempo | 20–300 BPM | 120 BPM |
| Run | Stop / Run | Run |
| Clock | Internal / Host | Host |
| Rate | 1/4 … 1/32 | 1/16 |
| Seq Pitch To | VCO 1+2 / Off / VCO 2 | VCO 1+2 |
| Seq Length | 1–16 | 8 |
| Seq Pitch Scale | Off / Chromatic / Major / Minor / Pentatonic | Off |
| Glide | Off – 1 s | Off |
| LFO Rate | 0.05–40 Hz | about 1 Hz |
| LFO Sync Rate | 4 Bars … 1/32 | 1 Bar |
| LFO Sync | Free / Sync | Free |
| LFO Shape | Triangle / Square / Saw / Ramp / Sample & Hold | Triangle |
| LFO Depth | 0–100 % | 0 % |
| LFO Target | Pitch / VCO 2 / Cutoff / FM / Volume | Cutoff |
| Step 1 … 16 Pitch | −24 … +24 semitones | 0 |
| Step 1 … 16 Velocity | 0–100 % | 100 % |
| Step 1 … 16 On | Off / On | On |

The "Steps 1-8 / 9-16" view is saved with the project but cannot be automated.

---

## 11. Specifications

| | |
|---|---|
| Formats | AU (macOS), VST3, CLAP; Standalone for testing |
| Systems | macOS 11+ (Apple Silicon and Intel), Windows x64, Linux x64 |
| Voices | 1 per instance |
| Oversampling | 4× up to 48 kHz, 2× up to 96 kHz |
| Latency | a few samples (reported to the DAW) |
| Output | mono signal on a stereo output |
| Filter | 4-pole ladder, low and high pass, self-oscillating |
| CPU | below 1 % of one core (measured on a current Mac) |

---

## 12. Troubleshooting

**PS-8 doesn't show up in the DAW** – check the plugin folder (see Installation) and rescan in your DAW.
For AU on the Mac, restarting the DAW sometimes helps.

**No sound** – Is the DAW playing (with HOST)? Is RUN on? Are the LEVEL knobs and VOLUME up? Are the
steps green? Is VELOCITY above 0? Was SCRAM pressed (then switch RUN back on)?

**The sequencer isn't in time** – set the switch to **HOST**. The Standalone has no DAW clock, so PS-8
always runs on its own tempo there.

**It whistles** – turn RESONANCE down. Fully right, the filter self-oscillates on purpose.

**CLIP is lit** – turn VOLUME or DRIVE down.
