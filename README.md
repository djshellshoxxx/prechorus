# PreChorus

32-Voice Swarm & Convergence Engine for vocals, instruments, and hits. Built for modern pop, EDM, future bass, dubstep, and breaks. VST3 + AU + Standalone, made with JUCE.

Captures notes, words, hits, or timed musical phrases from live audio input (or load any one-shot/vocal stem), and generates scalable swarms of up to 32 voices that start dispersed and progressively converge in pitch, time, stereo width, and tone directly into your chorus or drop.

---

## Features

### 1. Source & Capture Engine
- **Multiple Source Modes**:
  - **Live Capture**: Real-time recording from the track input.
  - **Loaded Sample**: Dedicated high-quality WAV/AIFF/FLAC/MP3 sample playback.
  - **Hybrid Layer**: Automatically layers live captured audio with loaded stems.
  - **Slice Scatter**: Transient-splits the source and distributes different slices across the ensemble voices.
- **Capture History & Locking**:
  - **8 Performance Memory Slots (1–8)**: Instant switching between recent vocal takes or phrase captures.
  - **Lock Toggle**: Protects your favorite take from being overwritten while continuing to audition.
- **Timed & Transient Capture**:
  - **Threshold Auto-Capture**: Auto-records when you sing or hit a note (customizable dB threshold) and cleanly auto-trims on silence release.
  - **BPM Sync Divisions**: Capture exact musical lengths: 1/16, 1/8, 1/4 (1 beat), 1/2 (2 beats), 1 Bar, and 2 Bars.
  - **Manual Arm**: One-click punch in/out.

### 2. 32-Voice Swarm Engine
- **Voice Count (1 to 32)**: Scalable from subtle analog doubling to massive, dense cinematic vocal swarms.
- **Progressive Reveal**: Early voices play short, percussive micro-chops while later voices progressively reveal more of the full vocal phrase.
- **Voice Age & Darkening**: Earlier voices in the swarm can be selectively darkened and saturated with vintage tape-style roll-off.
- **Voice Density Curve**: Shapes when and how quickly additional voices join the buildup (linear to explosive avalanche).
- **Voice Direction**: Forward, Reverse, Alternating, or Random per seed.

### 3. Convergence Engine
- **Target Convergence**: Voices start scattered and pull tightly together toward the focal climax:
  - **Timing Convergence**: Voices begin at wide staggered offsets and tighten toward the downbeat.
  - **Pitch Convergence**: Scattered interval offsets glide into unison at the drop.
  - **Musical Scale Lock**: Constrain scattered pitch intervals to Chromatic, Major, Minor, Pentatonic, or Octaves/5ths.
  - **Width Convergence**: Choose between collapsing from wide stereo to center mono punch, or blooming outward from center.
  - **Tone Convergence**: Scattered bright/dark voices progressively converge to the target spectrum.

### 4. Physics & Organic Modulation
- **Attraction**: Adjusts the gravitational pull strength snapping voices toward unison.
- **Turbulence**: Injects natural pitch, phase, and panning flutter for lifelike ensemble realism.
- **Overshoot**: Damped spring motion causing voices to briefly overshoot unison before locking in.
- **Orbit Mode**: Circulates voices through the 3D stereo field before resolving.
- **Deterministic Randomness (REGEN)**: Ensembles stay 100% bit-identical across playback and exports until you click REGEN for a new seed.

### 5. Swell Shaping, Mix & DAW Integration
- **Dry Replacement**: Blend or completely replace the original dry hit with the climax of the converged swarm.
- **Hit on Note (PDC)**: Plug-in Delay Compensation guarantees the drop lands exactly on the MIDI note or downbeat.
- **FL-Style Tension Curves**: Direct draggable tension envelopes over the interactive waveform for volume and pitch sweeps.
- **Drag-to-DAW**: Grab the "DRAG TO DAW" button to drop the rendered 24-bit WAV file straight onto your DAW arrangement playlist.
- **Space & Diffusion**: Ambient choral wash layered into the swell.

---

## Build (Windows)
1. Keep the path short (e.g. `C:\Users\new\desktop\antigravityprojects\prechorus`)
2. Right-click `build.ps1` > **Run with PowerShell** (as Administrator). First run installs CMake and downloads JUCE 8 automatically.
3. Plugin builds and installs to `C:\Program Files\Common Files\VST3\PreChorus.vst3`

If PowerShell blocks scripts: `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` once in an admin PowerShell.

## Build (macOS / Linux)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## DAWs & Usage
- **FL Studio**: Options > Manage plugins > Find plugins. Add `PreChorus` on your vocal/instrument track or channel rack.
- **Ableton / Logic / Reaper / Cubase**: Insert on any audio track for Live Capture and route MIDI to trigger the swarm.

## License
MIT