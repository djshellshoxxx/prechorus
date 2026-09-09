# PreChorus

Multi-voice anticipation and choral swell engine for vocals, synths, and one-shots. Built for modern pop, EDM, future bass, dubstep, and breaks. VST3 + AU + Standalone, made with JUCE.

Captures notes, words, hits, or timed musical phrases from live audio input (or load any one-shot/vocal stem), and generates multiple sample-based voices that swell and build anticipatory energy directly into your chorus or drop.

## Features
- **Live Capture Engine**:
  - **Threshold Auto-Capture**: Automatically begins recording when you sing, play a note, or fire a hit (customizable dB threshold).
  - **Beat & Bar Sync**: Captures timed phrases (1 Beat, 2 Beats, 1 Bar, 2 Bars) locked to your DAW's transport BPM.
  - **Manual Arm**: One-click punch in/out live audio capture.
  - **Sample & Stem Loader**: Or browse local folders (`<` `>`) or drag-and-drop any WAV/AIFF/FLAC/MP3 directly into the plugin.
- **Voice Engine**:
  - **Voice Count**: Layer 2 to 12 simultaneous sample-based voices.
  - **Timing Spread & Stagger**: Voices cascade and enter progressively before the downbeat.
  - **Micro-Detune**: Sub-cent pitch detune for wide, lush choral unison.
  - **Harmonic Stacking**: Musical harmony modes: Unison, Octaves, Power 5ths, and Choral Chords.
  - **Stereo Pan Spread**: Fanning voices across the stereo soundstage.
  - **Space & Diffusion**: Ambient choral wash layered into the swell.
  - **Reverse Swell Blend**: Continuously morph between forward choral bloom and reversed vocal riser textures.
- **Interactive Waveform & Envelopes**:
  - **Waveform Color Reactivity**: Color shifts dynamically with Tone (LPF) and Bass Cut (HPF).
  - **FL-Style Tension Curve**: Draggable volume envelope and tension handle right on top of the waveform.
  - **Pitch Sweep & Tension**: 1 / 2 / 4 octave pitch sweep with dedicated tension curve box.
  - **Interactive Trim**: Drag handles to trim start and end points.
  - **Voice Orbit Constellation**: Real-time visualizer showing voice spread, stereo imaging, and phase.
- **Workflow & DAW Integration**:
  - **Hit on Note (PDC)**: Plug-in Delay Compensation aligns the climax hit / chorus drop dead-on the MIDI note.
  - **Host BPM Sync**: Lock swell length to 1/2 bar, 1 bar, 2 bars, or 4 bars.
  - **Drag-to-DAW**: Grab the "DRAG TO DAW" button to drop the rendered audio swell directly onto your DAW arrangement playlist.
  - **WAV Export**: One-click 24-bit WAV file export.
  - **Randomize**: Instant inspiration button for experimental choral riser textures.

## Build (Windows)
1. Keep the path short (e.g. `C:\Users\new\desktop\antigravityprojects\prechorus`)
2. Right-click `build.ps1` > **Run with PowerShell** (as Administrator). First run installs CMake and downloads JUCE 8 automatically.
3. Plugin builds and automatically installs to `C:\Program Files\Common Files\VST3\PreChorus.vst3`

If PowerShell blocks scripts: `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` once in an admin PowerShell.

## Build (macOS / Linux)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## DAWs & Usage
- **FL Studio**: Options > Manage plugins > Find plugins. Add `PreChorus` as an effect on your vocal/synth track, or in the Channel Rack as an instrument to trigger via MIDI piano roll.
- **Ableton / Logic / Reaper / Cubase**: Insert on any audio track for Live Capture, route MIDI to it to trigger swells, or use the standalone app.

## License
MIT