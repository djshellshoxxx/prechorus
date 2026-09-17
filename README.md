# PreChorus

32-Voice Swarm & Convergence Engine for vocals, instruments, and hits. Built for modern pop, EDM, future bass, dubstep, and breaks. VST3 + CLAP + AU + Standalone on Windows, macOS and Linux, made with JUCE.

**Version 1.4.0**

**The Signature Sound**: A cloud of related voices becoming progressively more recognizable, coherent, and intimate until they meet the original drop or event.

---

## Complete Systems Breakdown

### 1. Source & Capture Engine
- **Multiple Source Modes**:
  - **Live Capture**: Real-time audio recording directly from your vocal/instrument track.
  - **Loaded Sample**: Pristine WAV, AIFF, FLAC, or MP3 one-shot and stem playback.
  - **Hybrid Layer**: Seamlessly layers live recorded vocal takes with loaded audio stems.
  - **Slice Scatter**: Transient-splits the source and distributes different slices across the ensemble voices.
- **Capture History & Locking**:
  - **8 Performance Memory Slots (1–8)**: Instant switching between recent vocal takes or phrase captures.
  - **Lock Toggle**: Protects your chosen take from being overwritten while continuing to audition.
- **Timed & Transient Capture**:
  - **Threshold Auto-Capture**: Auto-records when you sing or hit a note (customizable dB threshold) and cleanly auto-trims on silence release.
  - **BPM Sync Divisions**: Capture exact musical lengths: 1/16, 1/8, 1/4 (1 beat), 1/2 (2 beats), 1 Bar, and 2 Bars.
  - **Manual Arm**: One-click punch in/out.

### 2. 32-Voice Swarm Engine
- **Voice Count (1 to 32)**: Scalable from natural vocal doubling to dense cinematic swarms.
- **Progressive Reveal**: Early voices play short fragment chops/staccato slices; later voices progressively reveal more of the full phrase as the swell builds.
- **Voice Age & Darkening**: Earlier voices in the swarm can be selectively darkened with vintage tape-style roll-off.
- **Voice Density Curve**: Shapes when and how quickly additional voices join the buildup (linear rise or exponential avalanche).
- **Voice Direction**: Forward, Reverse, Alternating (odd reverse, even forward), or Random per seed.
- **Grain Size (10ms–200ms)**: Variable grain size and Hann windowing for granular textures.
- **Humanize**: Injects organic micro-variations across voice start timing (±15ms), micro-pitch drift, and velocity weighting.

### 3. Voice Character Models
- **Clean Digital**: Pristine, transparent interpolation with zero coloration.
- **Analog Ensemble**: Warm tape-style saturation (`tanh`), subtle analog pitch drift, and low-mid bandwidth contouring.
- **Bucket-Brigade (BBD)**: Darker repeats, analog BBD clock roll-off (4.5 kHz), companding, and controllable BBD clock noise.
- **Tape Choir**: Wow and flutter modulation with warm tape head saturation.
- **Dimension**: Ultra-wide cross-coupled chorusing (Roland Dimension D style) designed to preserve a solid, phase-safe mono center.
- **String Ensemble**: Vintage Solina/ARP-inspired multi-rate dual-LFO modulation (0.6 Hz slow LFO + 6.0 Hz vibrato).
- **Granular Cloud**: Grains form an ethereal ambient texture while remaining tightly tied to the source.
- **Lo-Fi Choral**: Vintage bit-depth and sample-rate reduction for gritty, crushed vocal chops.

### 4. Convergence Engine & Musical Motion
- **Convergence Macro (`MACRO` knob)**: A master knob scaling all enabled convergence dimensions simultaneously.
- **Freeze**: Suspends convergence and holds the ensemble at its current spread, transforming the swell into an infinite ambient drone or vocal choir pad.
- **Reverse Convergence (REV CONV)**: Inverts trajectory: voices begin in unified unison and progressively scatter into chaos before the target downbeat.
- **3D Distance Approach**: Doppler-like distance staging: voices begin far away in a deep wet cavern and rush forward into upfront, dry, in-your-face closeness right at the impact.
- **Focus**: Accelerates convergence near the drop, pulling pitch detune and stereo fanning into a laser-focused point.
- **Post-Target Release**:
  - **Cut at Impact**: The swarm abruptly ends at the climax hit, maximizing drop punchiness.
  - **Sustain Chorus**: Voices bloom and sustain past the impact as a rich, atmospheric choral pad with smooth cosine release.
  - **Scatter Out**: Converged energy explodes back outward into wide stereo panning and pitch dispersion after the drop.
- **Target Convergence Dimensions**:
  - **Timing Convergence**: Voices begin at wide staggered offsets and tighten toward the downbeat.
  - **Pitch Convergence**: Divergent intervals glide into unison at the focal climax.
  - **Musical Scale Lock**: Constrains scattered intervals to Chromatic, Major, Minor, Pentatonic, or Octaves/5ths.
  - **Width Convergence**: Collapse wide stereo down to center mono punch, or bloom outward from center.
  - **Tone Convergence**: Scattered bright/dark voices progressively match the target filter spectrum.

### 5. Global Shaping, Acoustics & Color
- **High-Pass & Low-Pass Filters with Resonance**: Sculpt the frequency range with variable Q resonance.
- **Tilt EQ**: One-knob spectral balance pivot (dark warm low-end to bright airy highs).
- **Presence**: Dedicated 10 kHz high-shelf presence lift for modern pop/EDM vocal clarity.
- **Air Sheen**: Filtered high-frequency exciter (8.5 kHz+ saturator) applied to both the ensemble swell and target climax hit for repeatable, shimmering harmonic top-end air.
- **Drive & Soft Clipping**: Analog saturation for harmonics and grit.
- **Transient Softening / Preservation**: Softens harsh sibilance or preserves sharp transient punch.
- **Formant Shift**: Transposes vocal formants independent of pitch (±12 semitones).
- **Mono Bass**: High-passes the Side channel below cutoff (20 Hz–300 Hz) to keep club sub-bass 100% pure mono.
- **Sidechain Ducking**: Automatically ducks the swell whenever live input vocals or drum hits strike.
- **Click Protection & Reverse Fades**: 5ms cosine windowing on grain edges and reverse turnarounds for 100% click-free playback.

### 6. Factory Presets
- **10 Curated Starting Points**: Pop Vocal Double, EDM Riser Swarm, Future Bass Shimmer, Dubstep Chaos Impact, Intimate Whisper Build, Cinematic Choir Pad, Lo-Fi Bedroom Vocal, Ambient Drone Freeze, Aggressive Distortion Drop, and Trap Vocal Stutter.
- Presets shape the swarm, convergence, and tone-color knobs only — your loaded/captured source audio, trims, and envelope shaping are left untouched, so a preset can be auditioned on any material.

### 7. DAW Integration & Performance
- **Target Confidence Readout**: Real-time visual badge showing downbeat alignment confidence (100% when locked).
- **Target Sequence**: Restricts automatic triggering to specific musical cycles (Every Note, Beat 1 Only, Every 2 Bars, Every 4 Bars).
- **Follow Tempo**: Automatically recalculates voice paths and swell lengths whenever the DAW BPM shifts.
- **Transport Recovery**: Detects DAW playhead jumps, loop restarts, or scrub discontinuities, immediately clearing voice buffers to prevent clicks or stale audio artifacts.
- **Hit on Note (PDC)**: Plug-in Delay Compensation guarantees the drop lands exactly on the MIDI note or downbeat.
- **MIDI Triggered**: Any MIDI note-on received by the plugin fires the swarm (velocity-sensitive), so a DAW MIDI track can trigger the drop exactly on the beat.
- **Drag-and-Drop Loading**: Drop a WAV/AIFF/FLAC/MP3/OGG file straight onto the plugin window to load it as the source.
- **Spacebar Preview**: Press Space anywhere in the plugin window to trigger playback, same as the PLAY button.
- **Drag-to-DAW**: Drag the "DRAG TO DAW" pad directly onto your DAW playlist as a 24-bit WAV file.

### 8. Workflow, Presets & Control
- **In-Plugin Manual**: The `?` button (or File > Manual) opens a full manual covering every control, the workflow, the GUI tour, keyboard shortcuts, and the version number.
- **File Menu**: Open Preset, Save Preset, Save Preset As, Export Audio to WAV, Reset to Defaults, Open Preset Folder, Options and Manual — all from one header dropdown.
- **User Presets**: Your own setups save as `.pcpreset` files (Documents/PreChorus/Presets by default) and carry the whole state, including MIDI mappings.
- **Options Page**: Turn hover tooltips on or off, review and clear MIDI mappings, open the audio / MIDI device selector (standalone), and jump to your preset folder.
- **Reset**: One button returns every setting and function to its factory default.
- **Random**: Generates a fresh sound each press. After the first press every further click resets to defaults first, so no two results build on each other.
- **A/B Compare**: Two full snapshots with a COPY button to duplicate one into the other.
- **Right-Click Any Control**: Reset to default, set a specific value, MIDI Learn (move a controller to bind it), or clear a mapping.
- **MIDI CC Learn**: Real CC mapping, saved with the plugin state and with presets, listed and clearable in Options.
- **Hover Tooltips**: Every control explains itself after a moment's hover; switchable in Options.
- **Knob Handling**: Double-click resets to default, vertical drag is fine, Shift is coarse and Ctrl is ultra-fine. Values animate over 80 ms and the readout appears above the knob on hover.

### 9. The Colony
The constellation is a living simulation, and every orb is one audio element - so what happens to
the orbs happens to the sound.
- **Click gestures**: left-click x4 splits three orbs, right-click x3 breeds a red one,
  middle-click x2 starts a clutch of eggs, and clicking a yellow orb lights a fuse.
- **Drag an orb**: slowly to mutate the sound and throw off fractals, fast to destroy.
- **Colony buttons**: Add Gravity, Release Gravity, Add Enzyme, Radiate and Add Water - and
  leaning on any of them has consequences of its own.
- **Colour rules**: green hunts red and detonates on contact; green brushing a plain orb twice
  turns time around; blue crashing into pink kills the blue one.
- **Reversal**: audio and animation wind down, stop, and run backwards - never instantly.
- **Breeding**: eggs hatch into coloured orbs that whistle, live 5-30 minutes and leave an
  oscillation behind. Clutch size decides what happens to the pace of everything.
- **Things that happen on their own**: visitors, spontaneous orbs, slow resizes, and the
  occasional asteroid that shatters the colony into a hundred pieces.
- **Score**: points for every change you make. It means nothing. Switchable in Options.

### 10. The Voxbox
A formant-synthesis voice that builds every utterance from scratch, so it always sounds like
someone saying something and never twice the same way. Left alone it calls for help; it also
speaks for blown fuses, dead blue orbs, arriving visitors, explosions, and long fuses lit by the
RANDOM button, a preset load, a dropped sample or a right-click - in English, Spanish or French.

### 11. Visual Identity
Built to the shared visual identity spec used across the plugin range — same palette, typography,
knob and button shapes, 8px layout grid, 32px header strip, meter behaviour, and the signature
accent notch in the top-left corner with the version stamped bottom-right. See `theme.md`.

---

## Formats

| Format | Windows | macOS | Linux |
|---|---|---|---|
| VST3 | yes | yes | yes |
| CLAP | yes | yes | yes |
| AU | - | yes | - |
| Standalone | yes | yes | yes |

The CLAP build is produced by [clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions)
wrapping the same plugin target, so it shares every source file, parameter and saved state with the
VST3 — presets and `.pcpreset` files move between the two freely.

CLAP can be turned off with `-DPRECHORUS_BUILD_CLAP=OFF` if you only want VST3 and Standalone.

## Build (Windows)
1. Open PowerShell **as Administrator**:
   ```powershell
   cd C:\Users\new\desktop\antigravityprojects\prechorus
   .\build.ps1
   ```
2. Builds the VST3, CLAP and Standalone targets and installs `PreChorus.vst3` to
   `C:\Program Files\Common Files\VST3\PreChorus.vst3`. The CLAP lands in
   `C:\Program Files\Common Files\CLAP\PreChorus.clap`.

## Build (Linux)
```bash
./build-linux.sh
```
Installs the build dependencies it is missing, then configures and builds VST3, CLAP and
Standalone into `build-linux/`. Written against Ubuntu 26.04 (gcc 15, Ninja) and usable under
WSL. The Linux CI job builds and links this same configuration on every push.

To install for the current user:
```bash
mkdir -p ~/.vst3 ~/.clap
cp -r build-linux/PreChorus_artefacts/Release/VST3/PreChorus.vst3 ~/.vst3/
cp    build-linux/PreChorus_artefacts/Release/CLAP/PreChorus.clap ~/.clap/
```

## Build (macOS / any platform, by hand)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## CI
`.github/workflows/build.yml` builds Linux and Windows on every push to `main` or a `feat/**`
branch and uploads the VST3, CLAP and Standalone artefacts. Both platforms are green as of
v1.4.0; grab a build from the run's **Artifacts** section rather than compiling locally if you
just want to install it.

## License
MIT