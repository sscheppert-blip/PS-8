# PS-8 – Development

## Releases

Builds run only on demand (pushes don't trigger them, to save GitHub Actions minutes). To publish a
new version, bump `VERSION` in `CMakeLists.txt`, commit and push, then:

- **On a Mac:** `tools/release-mac.sh 0.2.0` builds, tests and validates the macOS plugins locally as
  universal binaries, creates a draft release with them, then lets GitHub build only Linux and
  Windows and publish. `--dry-run` stops after packaging.
- **On a PC:** `gh workflow run build -f version=0.2.0` (or Actions → build → Run workflow) builds
  all three platforms on GitHub.

## Building

Needs CMake ≥ 3.22, a C++20 compiler and Ninja (optional). JUCE and clap-juce-extensions are
fetched by CMake.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/PS8Tests_artefacts/Release/PS8Tests
```

The plugins are copied into the user plugin folders after building, and on macOS the Standalone app
into `~/Applications` (`-DPS8_COPY_PLUGIN=OFF` to skip).
The test runner also writes a demo pattern (`--render demo.wav`; add a factory preset name, `hit` for a
single hit of step 1 instead of the sequence, and `paramId=value` overrides, e.g.
`--render kick.wav "Deep Kick" hit cutoff=0.3`; a hit plays after 0.2 s of silence so the parameter
smoothing has settled), a panel screenshot
(`--snapshot panel.png`, add `patched` for a few demo cables and/or `service` for the open service hatch),
frames of the hatch animation (`--hatch-frames <folder>`), the app icon (`--icon design/icon.png`,
1024 px, used for the Standalone app) and measures the CPU load (`--bench`).

Manual images: `docs/images/panel.png` (`--snapshot … patched`) and `docs/images/service.png`
(`--snapshot … patched service`), both downscaled to 1400 px.

The tests play every factory preset for 3 s and check that it sounds without clipping; with the
environment variable `PS8_PRESET_LEVELS=1` they print the peak level of each preset.

## Matching the sound to reference recordings

`tools/sound-match/` (Python 3 with numpy and scipy) compares PS-8 with recordings of single hits or
loops. Keep the recordings out of the repository – `SAMPLES/` is ignored for that.

```bash
python tools/sound-match/slice.py SAMPLES SAMPLES/_hits          # loops -> single hits + catalogue.csv
python tools/sound-match/fit.py build/PS8Tests_artefacts/Release/PS8Tests SAMPLES/_hits 5 350
```

`slice.py` cuts the loops into hits and sorts them roughly (kick, tonal, noise, metal). `fit.py` picks
representative hits per class, searches the PS-8 settings that come closest (level curve, pitch curve,
spectrum) and prints what is left over on average – a difference that stays for every hit points at the
engine, one that goes away points at a preset. It renders through `PS8Tests --render-stdin`, which keeps
one process running and renders one hit per input line (`<wav>|<option>|…`, options as for `--render`,
plus `cables=a>b,c>d` and `seconds=<n>`).

## Validation (macOS)

Before a release, validate the installed plugins:

```bash
auval -v aumu Ps08 Sbli
pluginval --strictness-level 10 --validate-in-process --validate ~/Library/Audio/Plug-Ins/VST3/PS-8.vst3
clap-validator validate ~/Library/Audio/Plug-Ins/CLAP/PS-8.clap
```

`tools/check-sensitive-data.sh` (needs ripgrep) scans the tracked files for credentials and local paths.
