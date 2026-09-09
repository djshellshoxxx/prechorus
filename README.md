# PreChorus

32-Voice Swarm & Convergence Engine for vocals, instruments, and hits. Built for modern pop, EDM, future bass, dubstep, and breaks. VST3 + AU + Standalone, made with JUCE.

**The Signature Sound**: A cloud of related voices becoming progressively more recognizable and coherent until they meet the original drop or event.

---

## Key Systems & Features

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
- **Voice Character**:
  - **Clean Digital**: Transparent, high-precision interpolation.
  - **Analog Ensemble**: Warm tape saturation (`tanh`), subtle pitch drift, and bandwidth contouring.
  - **Lo-Fi Choral**: Gritty vintage bit-depth and sample-rate reduction.
- **Humanize**: Injects organic micro-variations across start timing (+/- 15ms), pitch drift, and velocity.

### 3. Convergence & Modulation Engine
- **Convergence Macro**: A single dedicated macro knob scaling all enabled convergence dimensions together.
- **Freeze**: Suspends convergence and holds the ensemble at its current spread for an infinite ambient drone or vocal choir pad.
- **Reverse Convergence (REV CONV)**: Inverts trajectory: voices begin in unified unison and progressively scatter into chaos before the target downbeat.
- **Post-Target Release**:
  - **Cut at Impact**: Voices abruptly cut off right on the drop, maximizing transient impact.
  - **Sustain Chorus**: Voices sustain past the drop as a lush, blooming chorus pad.
  - **Scatter Outward**: The converged energy explodes back outward in pitch and stereo space after the downbeat.
- **Target Convergence Dimensions**:
  - **Timing Convergence**: Voices begin at wide staggered offsets and tighten toward the downbeat.
  - **Pitch Convergence**: Divergent intervals glide into unison at the focal climax.
  - **Musical Scale Lock**: Constrains scattered intervals to Chromatic, Major, Minor, Pentatonic, or Octaves/5ths.
  - **Width Convergence**: Collapse wide stereo down to center mono punch, or bloom outward from center.
  - **Tone Convergence**: Scattered bright/dark voices progressively match the target filter spectrum.

### 4. Physics & Organic Movement
- **Attraction**: Dial in the gravitational pull strength snapping voices toward unison.
- **Turbulence**: Injects natural pitch, phase, and panning flutter for realistic, organic movement.
- **Overshoot**: Damped spring motion causing voices to briefly swing past unison before settling.
- **Orbit**: Circulates voices through the 3D stereo field before resolving.
- **Deterministic Randomness (REGEN)**: Ensembles stay 100% bit-identical across playback and exports until you click REGEN for a new seed.

### 5. DAW Integration & Performance
- **Target Confidence Readout**: Real-time visual badge showing downbeat alignment confidence (100% when locked).
- **Target Sequence**: Restrict triggering to specific musical cycles (Every Note, Beat 1 Only, Every 2 Bars, Every 4 Bars).
- **Follow Tempo**: Automatically recalculates voice paths and swell lengths whenever DAW BPM changes.
- **Transport Recovery**: Detects DAW playhead jumps and loop restarts, immediately flushing buffers to prevent clicks or stale audio.
- **Hit on Note (PDC)**: Plug-in Delay Compensation guarantees the drop lands exactly on the MIDI note or downbeat.
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
MIT