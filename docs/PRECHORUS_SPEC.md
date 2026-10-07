# PreChorus Product Specification

**Status:** Active implementation specification  
**Version:** 1.3 (product release v0.0.1 beta)  
**Product:** PreChorus  
**Shared baseline:** [Circuit Drift Labs Shared Audio Plugin Standard](standards/CDL_PLUGIN_BASELINE.md)

## 1. Product purpose

PreChorus is a source-driven 32-voice swarm and convergence audio effect for vocals, instruments, hits, risers, and transition material. It builds a dispersed ensemble from live-captured or loaded audio and progressively converges that material toward a target musical event.

The core design goal is fast sound design with deterministic repeatability: users should be able to load or capture a source, establish the convergence character, audition immediately, and export or trigger the result without needing external routing or destructive editing.

## 2. Product profiles

| Profile | Status | Notes |
|---|---|---|
| Effect plug-in | Implemented | Mono or stereo input, stereo output |
| MIDI-triggered effect | Implemented | MIDI note-on triggers rendered swarm playback |
| Standalone companion | Implemented | JUCE standalone target |
| Offline renderer/exporter | Implemented | 24-bit stereo WAV export |
| Instrument plug-in | Not applicable | Does not synthesize independent note voices |
| MIDI effect | Not applicable | Does not transform MIDI output |

Formats: VST3, CLAP, Standalone (Windows x64 release); AU is built on macOS only and is not part of the v0.0.1 beta.  
Primary build architecture: host-native architecture provided by JUCE/CMake toolchain.  
Main output: stereo.  
Main input: disabled, mono, or stereo.  
MIDI input: accepted. MIDI output: none.

## 3. Source and capture requirements

PreChorus MUST support Live Capture, Loaded Sample, Hybrid Layer, and Slice Scatter source modes.

Loaded audio MUST support WAV, AIFF/AIF, FLAC, MP3, and OGG when the JUCE build provides the corresponding decoder. Mono files MUST be converted to safe stereo internally before the render engine reads them.

Loading a file while Live Capture is selected MUST switch to Loaded Sample so the user's load action has an audible result.

Capture history MUST provide eight slots, an active slot, and a lock state. Locking a slot MUST prevent accidental overwrite of the protected take.

A finished capture switches Loaded Sample to Live Capture so the new take is audible (documented dependency, mirrors the load rule above). Capture recording runs on the audio thread; committing a take to a history slot happens on the message thread.

Capture modes MUST include Threshold, 1/16, 1/8, 1/4, 1/2, 1 Bar, 2 Bars, and Manual.

## 4. Swarm and convergence requirements

The engine MUST support 1 to 32 rendered ensemble voices and preserve deterministic random behavior until the user explicitly regenerates the seed.

Required swarm dimensions:
- voice count, density, age, reveal, direction, humanize, and grain size;
- time spread and convergence;
- pitch spread, detune, scale lock, and pitch convergence;
- pan spread and width convergence;
- tone convergence;
- attraction, turbulence, overshoot, orbit, distance, and focus;
- freeze and reverse convergence;
- Cut at Impact, Sustain Chorus, and Scatter Out post-target modes.

Voice character models MUST include Clean Digital, Analog Ensemble, Bucket-Brigade, Tape Choir, Dimension, String Ensemble, Granular Cloud, and Lo-Fi Choral.

## 5. Tone, mix, and dynamics requirements

Required shaping controls are Length, Shape, Tone, Bass Cut, Resonance, Tilt EQ, Presence, Air, Space, Drive, Transients, Formant, Mono Bass, Dry, Wet, Replace, Ducking, Pitch, volume start/end/tension, trim start/end, and pitch tension.

Controls that can create audible discontinuities SHOULD be smoothed or applied in pre-rendered processing rather than producing zippering in the realtime callback.

Output samples MUST remain finite for valid parameter states.

## 6. Transport, triggering, and preview

Any MIDI note-on MUST be able to trigger the swarm at the note's sample offset. Velocity SHOULD influence playback gain of both the swarm and the target hit.

When **Keytrack** (`keytrack`) is on, the note number transposes playback by resampling (C4/60 = original); the time scale changes with the pitch, so PDC alignment applies to untransposed playback only.

When **Hit on note (PDC)** (`align`) is on, the plug-in reports the hit position as latency (max 20 s) and delays the dry input by the same amount.

**Build Stutter** (`stutter`, 0–1, default 0) gates the second half of the swell at 1/8, then 1/16 (from 50%), then 1/32 (from 80%) notes at host tempo with 2 ms fades; the amount sets gate depth.

The PLAY action and Spacebar shortcut MUST trigger the same preview path.

The editor MUST also provide:
- Escape to stop active preview playback.
- R to randomize the current sound-design parameters.
- G to regenerate only the deterministic seed.
- H or F1 to open the help overlay (Esc closes it).
- B to switch the A/B comparison slot.

Keyboard shortcuts MUST be listed in the help overlay and in control tooltips where relevant.

## 7. Usability requirements

### 7.1 Tooltips

Every primary actionable control MUST have a concise tooltip describing the result of using it. Knob tooltips MUST explain the parameter's audible purpose rather than merely repeating its label.

Tooltips MUST cover, at minimum:
- source, character, scale, direction, post-release, sequence, sync and preset selectors;
- load, previous/next sample, play, export, reset, randomize, regenerate, help, arm, capture and lock actions;
- all rotary parameter controls;
- freeze, reverse convergence, PDC alignment and sync toggles.

### 7.2 Operation status

The editor MUST expose a persistent status text area for user actions that otherwise fail silently.

Status messages MUST cover at least:
- successful or failed sample loading;
- successful or failed WAV export;
- randomize, reset and regenerate actions;
- preview start and stop;
- preset selection.

A status message MUST not interrupt audio processing or require a modal dialog for normal operation.

### 7.3 Safe parameter reset

Rotary controls MUST support double-click reset through JUCE's parameter attachment/default-value behavior. Reset actions MUST preserve source audio and capture history unless explicitly documented otherwise.

### 7.4 Reduced motion and output metering

The editor MUST provide a Reduced Motion control that stops decorative orbital and transient-flash animation without disabling functional waveform or meter updates.

The editor MUST show a labeled post-processing stereo peak readout in dBFS. The readout MUST visibly indicate CLIP when the measured peak reaches or exceeds 0 dBFS.

### 7.5 Discoverability

The help overlay MUST explain:
- the product workflow;
- source modes;
- character modes;
- keyboard shortcuts;
- factory presets;
- export and drag-to-DAW behavior.

## 8. State and preset behavior

Host state is authoritative for project recall. APVTS parameters MUST be restored from saved state.

The state additionally stores the selected external source path, active capture slot, and capture lock. If the external source file is missing at restore time, PreChorus MUST remain stable and retain parameter state.

Factory presets MUST alter sound-design parameters only. They MUST NOT replace loaded audio or overwrite capture history. Each factory preset first restores sound-design defaults (excluding seed, mix levels and the pitch/volume envelopes) so no value leaks from a previous preset.

State carries `stateVersion` (currently 2). State with a different root type or a newer version is ignored and the current state is kept. Restore never changes the saved Source Mode. The editor size is stored as a view preference.

**A/B comparison:** two in-memory snapshots of the sound-design parameters. Switching stores the current slot and recalls the other; the first switch copies the current sound. Snapshots are not saved with the session.

**User presets:** `.pcpreset` XML (`PRECHORUS_PRESET`, `schema` = 1, one `SNAPSHOT` child of parameter values) in `Documents/Circuit Drift Labs/PreChorus/Presets`. Loading validates root type, schema, size (≤ 256 KB) and that values are finite numbers, then clamps them to range; on failure the current sound is kept and the status bar explains why.

## 9. File export

WAV export MUST produce stereo 24-bit PCM at the rendered sample rate and use the same rendered swarm state used for preview.

Export failure MUST be surfaced in the non-modal editor status area.

## 10. Realtime safety

The audio callback MUST not perform file I/O, network I/O, modal UI work, or unbounded rendering work.

Rendering and file loading may occur outside the realtime callback. Shared rendered-state access MUST remain synchronized.

## 11. Baseline compliance record

| Shared baseline area | Status | Notes |
|---|---|---|
| Product profile declaration | Implemented | This document |
| Stable parameter IDs | Implemented | IDs namespace/APVTS |
| Host automation attachments | Implemented | JUCE APVTS attachments |
| Tooltips | Implemented in v1.3 | Per-control audible-result text; global on/off in OPTIONS |
| Keyboard discoverability | Implemented in v1.3 | Space, Esc, R, G, B, H/F1 |
| Reduced motion option | Implemented in v1.2 | Decorative orbit/flash can be disabled |
| State restore | Implemented | APVTS + source path/slot/lock |
| Presets | Implemented | 10 factory presets + versioned user presets |
| A/B comparison | Implemented in v1.3 | Sound-design snapshots |
| Resizable UI | Implemented in v1.3 | 60–200%, fixed aspect ratio |
| Latency reporting | Implemented | Hit position when PDC on; dry path delayed to match |
| Offline WAV export | Implemented | 24-bit stereo WAV |
| Unsupported bus rejection | Implemented | Stereo output; disabled/mono/stereo input |
| Automated unit tests | Partial | pluginval strictness 8 (VST3) and clap-validator pass on Linux builds; product unit tests planned |
| Output peak/clip metering | Implemented in v1.2 | Stereo post-processing peak in dBFS |
| CI build matrix | Partial | Windows VST3/CLAP/Standalone build gate + tagged release packaging; macOS remains planned |
| Accessibility audit | Planned | Focus order and reduced-motion pass remains |

## 12. Future usability work

Prioritized follow-up items:
1. Searchable preset browser and favorites.
2. macOS AU/Standalone CI validation.
3. Automated JUCE unit tests for parameter ranges, state restore, file-loading edge cases, and deterministic rendering.
