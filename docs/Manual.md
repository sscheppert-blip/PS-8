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
6. [Under the hood: the service board](#6-under-the-hood-the-service-board)
7. [Patchbay](#7-patchbay)
8. [Presets](#8-presets)
9. [MIDI](#9-midi)
10. [Sound recipes](#10-sound-recipes)
11. [Tips and tricks](#11-tips-and-tricks)
12. [Parameter list](#12-parameter-list)
13. [Specifications](#13-specifications)
14. [Troubleshooting](#14-troubleshooting)

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

### Unblocking plugins on macOS

Every file you download gets a hidden "quarantine" mark from macOS. For plugins that are not
notarized by Apple – like PS-8 and many free or open-source plugins – this can mean:

- a message that the plugin "cannot be opened" or "is damaged",
- or your DAW silently skips it during the scan.

**The general way – works for any plugin:** remove the mark once in the **Terminal**
(Applications → Utilities → Terminal), then restart your DAW:

```bash
xattr -dr com.apple.quarantine "/path/to/the/plugin"
```

Tip: type `xattr -dr com.apple.quarantine ` (with the space at the end), drag the plugin file from the
Finder into the Terminal window – the path is filled in – and press Return. `-d` deletes the mark,
`-r` includes everything inside the plugin bundle. `xattr -l "/path/to/the/plugin"` shows whether a
mark is still there.

**For PS-8 – all three formats at once:**

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/PS-8.component ~/Library/Audio/Plug-Ins/VST3/PS-8.vst3 ~/Library/Audio/Plug-Ins/CLAP/PS-8.clap
```

If you only installed some of the formats, the command reports the missing ones – that's fine, the
others are unblocked anyway.

Only do this for plugins from sources you trust: the mark is macOS's way of asking "are you sure?".

---

## 2. Getting started

1. Load PS-8 on a **MIDI/instrument track**.
2. Flip **RUN** up (the sequencer starts switched off) and press **Play** in your DAW. PS-8 follows
   the song – **HOST** is on by default.
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
| Drag the window corner | Resize (the aspect ratio is kept; PS-8 remembers the size) |

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
 PATCHBAY ─► cables re-route all of the above (section 7)
```

Every sequencer step (or matching MIDI note) **fires all three envelopes at once**. The envelopes
jump to full and then decay – how long is set by the three **DECAY** knobs, how strongly they act by
the **EG AMOUNT** knobs.

---

## 4. The front panel

### Top bar

| Control | Function |
|---|---|
| **PROGRAM** | Preset name, ◀ ▶ to step through the presets, SAVE, MENU – see [8. Presets](#8-presets) |
| **OUTPUT** (meter) | Output level: left −20 dB, centre 0 dB, right +3 dB |
| **LOAD** (meter) | CPU load of the plugin in percent |
| **TEMPO BPM** (nixies) | Current tempo – PS-8's own or the DAW's |
| **RUN** (green lamp) | Lit while the sequencer runs |
| **STEP** (nixies) | Last step played |
| **SCRAM** | Emergency stop: the sequencer stops (RUN switches off), the sound is cut at once, the step lamps flash red briefly |
| **SERVICE** (key) | Opens and closes the cover over the module row – see [6. Under the hood](#6-under-the-hood-the-service-board) |

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

### PATCHBAY

The jacks at the bottom re-route envelopes, oscillators and the sequencer with cables – see
[7. Patchbay](#7-patchbay).

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
| **1-8 / 9-16** (tabs in the title bar) | Which eight steps are shown. A dot lights on the page that is playing |

**HOST mode in detail:** the steps follow the position in the song, counted from the song start. With
8 steps of 1/16, step 1 falls on every half note; with 16 steps on every bar. When you jump in the DAW
or a loop wraps, the sequencer follows. **RESET** moves the start to the next step.

**Velocity and level:** a step's velocity is the peak of the VCA envelope. Velocity 0 is silent – just
like a step switched off. As on the original, softer steps also decay a little faster: at 100 % the
three envelopes take exactly the DECAY knob's time, at 0 % they are 30 % shorter.

**Why two equal steps can sound different:** the oscillators run freely, like analog ones – a hit
starts wherever the wave happens to be. With low pitches (kicks, toms) that changes the click and punch
from step to step. For identical hits, patch **TRIG → VCO RST** (see chapter 7).

---

## 6. Under the hood: the service board

![PS-8 service board](images/service.png)

The front panel only carries what belongs to the original instrument. The extras sit on a circuit board
under the module row – like the service hatch of a control room.

- **Open:** turn the **SERVICE** key (top right, next to SCRAM) or click one of the four screws at the
  corners of the module row. The screws turn out and the cover flips up.
- **Close:** turn the key back or click one of the four screw holes on the board.
- The hatch stays open or closed with your project. Presets don't change it.

By default everything here is neutral: 8 steps, no scale, no glide, LFO depth 0. The blue trimmers
work like the knobs (drag, double-click resets); a small display under each one shows its value.

### SEQ OPTIONS

| Control | Function |
|---|---|
| **RATE** | Note value per step: 1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32 (T = triplet) |
| **LENGTH** | Number of steps, 1–16. Odd lengths (5, 7, 11 …) run against the bar |
| **SCALE** | Snaps the PITCH knobs: **OFF** (continuous), **CHROMATIC** (semitones), **MAJOR**, **MINOR**, **PENTATONIC** (minor pentatonic). The root is always the set FREQUENCY |
| **GLIDE** | The pitch slides from step to step instead of jumping, up to 1 s. Fully left = off |

### LFO

| Control | Function |
|---|---|
| **RATE** | Speed, 0.05–40 Hz. With **SYNC** the same trimmer picks a note value: 4 bars, 2 bars, 1 bar, 1/2, 1/4, 1/8, 1/16, 1/32 |
| **SHAPE** | Triangle, square, falling saw, rising saw, sample & hold (random steps) |
| **DEPTH** | Amount. 0 = off |
| **TARGET** | **PITCH** (both VCOs, up to ±2 octaves), **VCO 2** (VCO 2 only), **CUTOFF** (up to ±4 octaves), **FM** (FM amount), **VOLUME** (tremolo) |
| **LFO SYNC** (jumper) | **FREE** or **SYNC**: click the jumper to move it. With SYNC, HOST and the DAW playing, the LFO is locked to the song position |

### STATUS (LEDs)

| LED | Function |
|---|---|
| **HOST LOCK** | Lit while PS-8 is locked to the DAW |
| **MIDI IN** | Flashes on every incoming note |
| **LFO** | Flashes with the LFO (when DEPTH > 0) |

---

## 7. Patchbay

The patchbay at the bottom re-routes signals with cables. Without cables PS-8 sounds and works exactly
as before: everything is connected internally. A cable only changes the input it is plugged into.

| Action | Result |
|---|---|
| Drag from one jack to another | Patch a cable (always from an output to an input, either direction) |
| Drag a plug out of an input | Move the cable to another input – or drop it anywhere else to remove it |
| Double-click or right-click a jack | Remove all cables on that jack |

One output can feed several inputs. Several cables into one input are added up. Cables are saved with
the project and presets, but they cannot be automated.

**Levels:** envelopes and VELOCITY run from 0 to 5 V, the oscillators and VCA swing ±5 V, and TRIG is a
5 V pulse of 5 ms. TRIG, RUN, ADV/CLK and RESET switch at 2.5 V. At inputs that add to a knob, 5 V
covers the knob's whole range.

### Outputs (black)

| Jack | Signal |
|---|---|
| **VCO 1**, **VCO 2** | The oscillators' waveforms |
| **VCO EG** | Pitch envelope |
| **VCF EG** | Filter envelope |
| **VCA EG** | VCA envelope (including the step velocity) |
| **TRIG** | Short pulse on every sequencer step (also on MIDI notes and the TRIG/ADV buttons) |
| **VELOCITY** | Velocity of the current step |
| **PITCH** | Pitch of the current step, 1 V per octave |
| **VCA** | The finished sound after the VCA, before VOLUME |

### Inputs

| Jack | With a cable | Without a cable |
|---|---|---|
| **VCO 1 CV**, **VCO 2 CV** | Pitch, 1 V per octave. PITCH into VCO 1 CV moves VCO 1 by exactly the step pitch | – |
| **VCO RST** *(extra)* | A rising edge restarts both VCOs at the same point of the wave. TRIG → VCO RST makes every hit sound the same | Free-running VCOs |
| **1>2 FM** | Adds to FM 1>2 AMOUNT | – |
| **VCO DEC** | Adds to the PITCH decay | – |
| **NOISE** | Adds to the noise LEVEL | – |
| **EXT IN** | Replaces the noise in the mixer (level: noise LEVEL) | Noise, or the DAW's side-chain audio |
| **VCF MOD** | Modulates the cutoff; noise VCF MOD sets the depth | Noise |
| **VCF DEC** | Adds to the FILTER decay | – |
| **TRIG** | A rising edge fires all three envelopes (the sequencer keeps running) | – |
| **VELOCITY** | Adds to the velocity of every hit | – |
| **VCA CV** | Opens the VCA directly, instead of the VCA envelope | VCA EG |
| **VCA DEC** | Adds to the VCA decay | – |
| **TEMPO** | Speeds the clock up: +1 V doubles the tempo (only with **INT**) | – |
| **RUN** | The sequencer runs only while the signal is high (and RUN is on) | Always running |
| **ADV/CLK** | External clock: every rising edge plays the next step, the own clock pauses. RUN must be on | Internal or DAW clock |
| **RESET** | A rising edge: the next step is step 1 | – |

**Audio from the DAW:** PS-8 has a side-chain input. Route a track to it in your DAW's side-chain
routing. While it is active and no cable is in EXT IN, its audio takes the place of the noise – it
runs through the filter, the VCA and the envelopes like the noise would.

### Patch ideas

- **Rolls:** VCA EG → TEMPO. Each hit speeds up the clock for a moment – bursts and rolls that follow
  the VCA decay.
- **Filter follows the melody:** PITCH → VCF MOD, noise VCF MOD up. Higher steps sound brighter.
- **Growl:** VCO 2 → VCF MOD. The filter is modulated at audio rate – rough, metallic edges.
- **Drone:** VELOCITY → VCA CV. The VCA stays open; every step changes the pitch, the velocity sets
  the level.
- **Accents shape the tone:** VELOCITY → VCF DEC. Loud steps get a longer filter sweep.
- **Feedback:** VCA → EXT IN, noise LEVEL carefully up. The sound feeds back into itself – gritty and
  unstable.
- **Identical hits:** TRIG → VCO RST. Every kick starts with the same click and punch, like a classic
  drum machine. (Deep Kick and Tom Melody come patched like this.)
- **VCO as clock:** VCO 1 FREQUENCY fully left, VCO 1 → ADV/CLK, VCO 1 LEVEL down. VCO 1 now clocks
  the sequencer; with VCO 1 EG AMOUNT up, every hit bends the tempo.

---

## 8. Presets

The **PROGRAM** display in the name plate shows the current preset. A `*` after the name means you
have changed something since loading it. Top right it says whether this is a **FACTORY** or a **USER**
preset.

| Control | Function |
|---|---|
| **◀ ▶** | Previous / next preset: Init, then the factory presets, then your own (wraps around) |
| **SAVE** | Save the current sound as your own preset. If the name exists already, PS-8 asks before overwriting |
| **MENU** or a click on the name | All presets at a glance, plus **Save as…**, **Rename…**, **Delete** and **Open preset folder** |

**What a preset contains:** all knobs and switches, the whole step pattern (pitch, velocity, on/off,
length, rate, scale, glide), the LFO and the patch cables. **Not** included are **RUN/STOP** and
**INT/HOST** – so loading a preset never starts or stops the sequencer.

**Factory presets** are built in and read-only. To keep a changed version, press **SAVE** and give it a
name. They are:

| Preset | |
|---|---|
| Deep Kick, Tom Melody, Snare, Closed Hat, Open Hat, Laser Zap, Acid Line | The recipes from chapter 10 (kick and tom with TRIG → VCO RST for even hits) |
| Moving Groove, Random Pitches | LFO on cutoff / random step pitches, in time with the DAW |
| Rolls (INT) | VCA EG → TEMPO. Switch the clock to **INT** – the TEMPO input only works with the own clock |
| Growl, Drone, Feedback, Melodic Filter | The patch ideas from chapter 7 |

**Where your presets live** – one file per preset (`.ps8preset`, plain XML), so you can back them up,
copy them to another computer or share them:

| System | Folder |
|---|---|
| Windows | `%APPDATA%\PS-8\Presets\` |
| macOS | `~/Library/Application Support/PS-8/Presets/` |
| Linux | `~/.config/PS-8/Presets/` |

Preset files dropped into that folder show up the next time you open the menu. Deleted presets go to
the trash.

**Projects and new instances:** a new PS-8 always starts with **Init** – also the Standalone app. A
saved project brings back exactly the sound it was saved with, including the preset name. PS-8 also
remembers the **window size** for all instances (`settings.xml` next to the `Presets` folder).

---

## 9. MIDI

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

## 10. Sound recipes

Start from a freshly loaded plugin each time (or pick **Init** in the PROGRAM menu). Most recipes
are also built in as factory presets. Clock positions refer to the knob position
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
- Service board: SCALE MINOR, GLIDE 9 o'clock, LENGTH 16; then spread the PITCH knobs

**Moving groove**
- Any sound, plus the LFO on the service board: TARGET CUTOFF, SHAPE TRIANGLE, jumper on SYNC, RATE 1 BAR, DEPTH 12 o'clock
- Or TARGET PITCH, SHAPE S&H, RATE 1/16, DEPTH 9 o'clock for random pitches

---

## 11. Tips and tricks

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

## 12. Parameter list

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
| Run | Stop / Run | Stop |
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

The "Steps 1-8 / 9-16" view, the open/closed service hatch and the patch cables are saved with the project but cannot be automated.

---

## 13. Specifications

| | |
|---|---|
| Formats | AU (macOS), VST3, CLAP; Standalone for testing |
| Systems | macOS 11+ (Apple Silicon and Intel), Windows x64, Linux x64 |
| Voices | 1 per instance |
| Oversampling | 4× up to 48 kHz, 2× up to 96 kHz |
| Latency | a few samples (reported to the DAW) |
| Output | mono signal on a stereo output |
| Input | optional side-chain (mono or stereo) for EXT IN |
| Patchbay | 17 inputs, 9 outputs |
| Filter | 4-pole ladder, low and high pass, self-oscillating |
| CPU | below 1 % of one core (measured on a current Mac) |

---

## 14. Troubleshooting

**PS-8 doesn't show up in the DAW** – check the plugin folder (see Installation) and rescan in your DAW.
On the Mac, unblock the plugin (see [Unblocking plugins on macOS](#unblocking-plugins-on-macos)), then
restart the DAW.

**No sound** – Is the DAW playing (with HOST)? Is RUN on? Are the LEVEL knobs and VOLUME up? Are the
steps green? Is VELOCITY above 0? Was SCRAM pressed (then switch RUN back on)? Is a cable in RUN,
ADV/CLK or VCA CV holding things up (double-click the jack to pull it)?

**The sequencer isn't in time** – set the switch to **HOST**. The Standalone has no DAW clock, so PS-8
always runs on its own tempo there.

**It whistles** – turn RESONANCE down. Fully right, the filter self-oscillates on purpose.

**CLIP is lit** – turn VOLUME or DRIVE down.

---

*Made with ideas and ears by a human, and with code, DSP and this manual by Claude Code
(Claude Opus 5.5, medium effort). Most of the flowers belong to the robot.* 🤖💐
