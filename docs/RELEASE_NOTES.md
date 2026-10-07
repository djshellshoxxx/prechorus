# PreChorus v0.0.1 beta — Release Notes

**Status:** BETA. Windows x64. Proprietary — see LICENSE.

## Downloads
| File | What it is | Install |
|---|---|---|
| `PreChorus-v0.0.1-beta-Windows-Installer.exe` | Installs VST3, CLAP and the Standalone app (choose components) | Run it (admin) |
| `PreChorus-v0.0.1-beta-Windows-Portable.zip` | Standalone app, no install | Unzip anywhere, run `PreChorus.exe` |
| `PreChorus-v0.0.1-beta-Windows-VST3.zip` | VST3 plug-in only | Copy `PreChorus.vst3` to `C:\Program Files\Common Files\VST3` |
| `PreChorus-v0.0.1-beta-Windows-CLAP.zip` | CLAP plug-in only | Copy `PreChorus.clap` to `C:\Program Files\Common Files\CLAP` |

All binaries use the static MSVC runtime (no Visual C++ redistributable needed). PreChorus makes no network connections and has no telemetry or licence checks.

## New in v0.0.1
- **CLAP format** alongside VST3 and Standalone.
- **A/B compare** (usability): `A`/`B` header button (shortcut **B**) swaps two sound-design snapshots; `A>B` copies the current one across.
- **User presets** (value): Save/Load `.pcpreset` files from the preset menu; saved presets appear in the menu. Stored in `Documents\Circuit Drift Labs\PreChorus\Presets`. Versioned, validated XML; never touches your source audio.
- **Keytrack** (fun): MIDI notes transpose the swarm (C4 = original) so you can play it like an instrument.
- **Build Stutter** (effect): accelerating 1/8 → 1/16 → 1/32 gate across the second half of the swell, locked to host tempo.
- **Resizable window** (60–200%, aspect-locked, remembered per session) plus a Window size menu in OPTIONS.
- **Trim In / Trim Out** knobs (the trim parameters previously had no control).

## Fixes
- Editor failed to compile (output meter referenced from the wrong class).
- Waveform never refreshed after a parameter change re-rendered the swarm.
- Live capture allocated memory and took a lock on the audio thread; capture is now committed on the message thread.
- PDC "Hit on note" delayed the swarm but not the dry input; the dry path is now delayed by the reported latency.
- MIDI triggers ignored the note's sample offset; triggers are now sample-accurate. Velocity now scales the target hit as well as the swarm.
- Restoring a session with a sample file switched Live Capture to Loaded Sample.
- Host automation of History Slot / Capture Lock had no effect.
- Factory presets leaked settings (e.g. Freeze, Reverse Convergence) from the previously chosen preset.
- Hybrid Layer misaligned layers recorded at a different sample rate.
- Dimension character used an already-modified pan value; Lo-Fi Choral lacked the specified sample-rate reduction; Analog Ensemble lacked the specified pitch drift.
- Target Sequence used the block start and assumed 4/4; it now uses each note's position and the host time signature.
- Mono input left the right channel undefined; non-finite input/output samples are now sanitized.
- Input meter / ducking / threshold capture now read both input channels; renders are never freed on the audio thread.
- Malformed or future-version session state is rejected safely; state now carries a version.
- Tooltips now describe each knob's audible result; help documents source modes, export, drag-to-DAW, and version/site.

## Validation (recorded)
- Regression suite `Tests/PreChorusTests.cpp`: 15 tests, all passing (Linux); also run in Windows CI.
- pluginval (Linux build, strictness 8, in-process): **SUCCESS** for VST3.
- clap-validator 0.3.2 (Linux build): 21 tests — 18 passed, 0 failed, 3 skipped.
- Windows VST3/CLAP/Standalone: built by GitHub Actions (`release.yml`). Not yet hand-tested in a Windows DAW — please report issues.

## Known limitations
- Keytrack transposition also changes playback speed, so transposed notes move the hit away from the PDC-aligned position.
- Loaded files are limited to the first 12 s; captures to 8 s (manual) / 12 s buffer.
- macOS (AU/VST3/CLAP) builds are not part of this beta.
