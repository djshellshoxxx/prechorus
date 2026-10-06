# PreChorus Product Specification

**Status:** Active implementation specification  
**Version:** 1.1  
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

Formats: VST3, AU, Standalone.  
Primary build architecture: host-native architecture provided by JUCE/CMake toolchain.  
Main output: stereo.  
Main input: disabled, mono, or stereo.  
MIDI input: accepted. MIDI output: none.

## 3. Source and capture requirements

PreChorus MUST support Live Capture, Loaded Sample, Hybrid Layer, and Slice Scatter source modes.

Loaded audio MUST support WAV, AIFF/AIF, FLAC, MP3, and OGG when the JUCE build provides the corresponding decoder. Mono files MUST be converted to safe stereo internally before the render engine reads them.

Loading a file while Live Capture is selected MUST switch to Loaded Sample so the user's load action has an audible result.

Capture history MUST provide eight slots, an active slot, and a lock state. Locking a slot MUST prevent accidental overwrite of the protected take.

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

Any MIDI note-on MUST be able to trigger the swarm. Velocity SHOULD influence playback gain.

The PLAY action and Spacebar shortcut MUST trigger the same preview path.

The editor MUST also provide:
- Escape to stop active preview playback.
- R to randomize the current sound-design parameters.
- G to regenerate only the deterministic seed.
- H or F1 to open the help overlay.

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

### 7.4 Discoverability

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

Factory presets MUST alter sound-design parameters only. They MUST NOT replace loaded audio or overwrite capture history.

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
| Tooltips | Implemented in v1.1 | Product-specific help text |
| Keyboard discoverability | Implemented in v1.1 | Space, Esc, R, G, H/F1 |
| Reduced motion option | Planned | Visualizer animation currently always active |
| State restore | Implemented | APVTS + source path/slot/lock |
| Presets | Implemented | 10 factory presets |
| A/B comparison | Planned | Useful future workflow feature |
| Offline WAV export | Implemented | 24-bit stereo WAV |
| Unsupported bus rejection | Implemented | Stereo output; disabled/mono/stereo input |
| Automated unit tests | Planned | Add parameter/state/DSP regression target |
| CI build matrix | Planned | Windows/macOS validation recommended |
| Accessibility audit | Planned | Focus order and reduced-motion pass remains |

## 12. Future usability work

Prioritized follow-up items:
1. A/B state comparison that snapshots parameter state without duplicating source media.
2. User preset save/load with schema versioning and validation.
3. Reduced-motion preference for constellation and waveform animation.
4. Searchable preset browser and favorites.
5. Resizable/scalable UI with a compact laptop layout.
6. Dedicated output peak/clip meter with labeled signal point.
7. Automated JUCE unit tests for parameter ranges, state restore, file-loading edge cases, and deterministic rendering.
