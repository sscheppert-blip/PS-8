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

The plugins are copied into the user plugin folders after building (`-DPS8_COPY_PLUGIN=OFF` to skip).
The test runner also writes a demo pattern (`--render demo.wav`), a panel screenshot
(`--snapshot panel.png`, add `patched` for a few demo cables and/or `service` for the open service hatch),
frames of the hatch animation (`--hatch-frames <folder>`), the app icon (`--icon design/icon.png`,
1024 px, used for the Standalone app) and measures the CPU load (`--bench`).

Manual images: `docs/images/panel.png` (`--snapshot … patched`) and `docs/images/service.png`
(`--snapshot … patched service`), both downscaled to 1400 px.

## Validation (macOS)

Before a release, validate the installed plugins:

```bash
auval -v aumu Ps08 Sbli
pluginval --strictness-level 10 --validate-in-process --validate ~/Library/Audio/Plug-Ins/VST3/PS-8.vst3
clap-validator validate ~/Library/Audio/Plug-Ins/CLAP/PS-8.clap
```

`tools/check-sensitive-data.sh` (needs ripgrep) scans the tracked files for credentials and local paths.
