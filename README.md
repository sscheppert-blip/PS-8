# PS-8

Analog-style semi-modular percussion synthesizer with a step sequencer, in a 1970s
control-room look. Built with JUCE as AU, VST3, CLAP and Standalone for macOS, Windows and Linux.

- Two VCOs (triangle/square) with pitch envelope, linear FM 1→2 and hard sync, white/pink noise
- 4-pole ladder filter (low/high pass) with envelope and noise modulation
- Three decay envelopes (pitch, filter, VCA) with fast/slow VCA attack, drive
- Step sequencer (1–16 steps) with pitch and velocity per step, steps switchable on/off
- Pitch quantiser (chromatic, major, minor, pentatonic) and glide
- LFO (5 shapes, free or host-synced) on pitch, VCO 2, cutoff, FM or volume
- Internal tempo or locked to the host's bar position, 1/4 … 1/32 incl. triplets
- MIDI: play single steps, advance, retrigger and reset by note
- SCRAM button: stops the sequencer and silences the voice
- Oversampled (4× up to 48 kHz, 2× up to 96 kHz)

**Coming in a future version:** the patchbay. The jacks are already on the panel (stamped
"OFFLINE"); patch cables to re-route envelopes, sequencer and modulation will follow in an update.

**Manual:** [English](docs/Manual.md) · [Deutsch](docs/Handbuch.md) · **Schnellstart (German quick start):** [docs/Schnellstart.md](docs/Schnellstart.md)

![PS-8 front panel](docs/images/panel.png)

## Download

Released versions are under **Releases**: AU (macOS), VST3 and CLAP for macOS, Windows and Linux.
Installation is explained in the manual ([English](docs/Manual.md#1-installation) ·
[Deutsch](docs/Handbuch.md#1-installation)).

## Credits

Idea, direction, ears and the "nah, take the red caps off" calls: [sscheppert-blip](https://github.com/sscheppert-blip).
Every line of code, DSP and documentation was typed by [Claude Code](https://claude.com/claude-code)
running **Claude Opus 5.5** on **medium effort**. So if you like it, send a good part of the flowers to
the robot – the human only pointed and said "make it go boom". 🤖💐

## License

Public domain ([Unlicense](LICENSE)) – do whatever you like with it, no credit needed.
The plugins are built with [JUCE](https://juce.com), which has its own license terms.

<sub>Building from source: [docs/Development.md](docs/Development.md)</sub>
