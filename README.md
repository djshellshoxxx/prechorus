# PreChorus

32-Voice Swarm & Convergence Engine for vocals, instruments, and hits. Built for modern pop, EDM, future bass, dubstep, and breaks. VST3 + AU + Standalone, made with JUCE.

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
- **Keyboard Workflow**: Space previews, Esc stops, R randomizes sound-design controls, G regenerates the deterministic seed, and H/F1 opens help.
- **Action Status Bar**: Loading, export, preview, preset, reset, randomize, and regeneration actions report success/state without interrupting audio.
- **Control Tooltips**: Primary buttons, selectors, toggles, and rotary controls provide inline usage guidance.
- **Drag-to-DAW**: Drag the "DRAG TO DAW" pad directly onto your DAW playlist as a 24-bit WAV file.

---

## Build (Windows)
1. Open PowerShell **as Administrator**:
   ```powershell
   cd C:\Users\new\desktop\antigravityprojects\prechorus
   .\build.ps1
   ```
2. Automatically builds VST3 and Standalone targets and installs `PreChorus.vst3` to `C:\Program Files\Common Files\VST3\PreChorus.vst3`.

## Build (macOS / Linux)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## License
PreChorus is currently distributed under the repository's proprietary license. See [LICENSE](LICENSE) and [COPYRIGHT-TRADEMARK.md](COPYRIGHT-TRADEMARK.md). Earlier versions that were published under MIT remain under the terms that accompanied those versions.

## Specifications

The product-specific source of truth is [docs/PRECHORUS_SPEC.md](docs/PRECHORUS_SPEC.md).

This project also follows the [Circuit Drift Labs Shared Audio Plugin Standard](docs/standards/CDL_PLUGIN_BASELINE.md). The product specification supplements the shared standard and records profiles, compliance status, product-specific behavior, and planned work.
