# PreChorus Product Specification

**Status:** Active implementation specification  
**Version:** 1.4 (post-v0.0.1 beta: all roadmap features implemented in this branch)  
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
| Offline renderer/exporter | Implemented | Stereo WAV 16/24/32f or FLAC 24-bit, selectable rate, normalise, tail trim, stems |
| Instrument plug-in | Not applicable | Does not synthesize independent note voices |
| MIDI effect | Not applicable | Does not transform MIDI output |

Formats: VST3, CLAP, Standalone (Windows x64 and Linux x64 release builds); AU is built on macOS only and is not part of the v0.0.1 beta.  
Primary build architecture: host-native architecture provided by JUCE/CMake toolchain.  
Main output: stereo.  
Main input: disabled, mono, or stereo.  
Optional buses (disabled until the host enables them): stereo/mono **Sidechain** input (detector for ducking, threshold capture and input-transient triggering) and three stereo stem outputs **Swarm**, **Hit** and **Dry** (main output = Swarm + Hit + Dry).  
MIDI input: accepted (notes, CC, channel pressure, pitch bend for MIDI learn). MIDI output: none.

## 3. Source and capture requirements

PreChorus MUST support Live Capture, Loaded Sample, Hybrid Layer, and Slice Scatter source modes.

Loaded audio MUST support WAV, AIFF/AIF, FLAC, MP3, and OGG when the JUCE build provides the corresponding decoder. Mono files MUST be converted to safe stereo internally before the render engine reads them. The maximum source length is 12, 30 (default), 60 or 120 s; longer files are cropped and the status bar says so.

**Slice Scatter** splits the loaded source at detected transients (up to 8 slices, ≥ 60 ms apart, each starting a few ms before its attack); a source without clear transients falls back to 8 equal slices. Voice *v* plays slice *v mod slice count*; reverse playback stays inside the slice.

Loading a file while Live Capture is selected MUST switch to Loaded Sample so the user's load action has an audible result.

Capture history MUST provide eight slots, an active slot, and a lock state. Locking a slot MUST prevent accidental overwrite of the protected take.

A finished capture switches Loaded Sample to Live Capture so the new take is audible (documented dependency, mirrors the load rule above). Capture recording runs on the audio thread; committing a take to a history slot happens on the message thread.

Capture modes MUST include Threshold, 1/16, 1/8, 1/4, 1/2, 1 Bar, 2 Bars, and Manual.

**Smart capture** (on by default, OPTIONS): a finished take has leading and trailing silence below −50 dBFS removed with 10 ms cosine fades, never shortening it below 50 ms; an all-silent take is kept unchanged. Optionally the take is peak-normalised to −1 dBFS. The status bar reports the length and trimmed time, and RETAKE re-arms into the same slot (the capture lock is honoured). The capture strip shows the live input level and the capture threshold so the user can see what will trigger a take. The detector for threshold capture can be the main input or the sidechain bus (`scSource`).

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

Voice character models MUST include Clean Digital, Analog Ensemble, Bucket-Brigade, Tape Choir, Dimension, String Ensemble, Granular Cloud, and Lo-Fi Choral, and additionally provide Vowel Choir (parallel formant filters gliding a-e-i-o-u), Stadium Crowd (per-voice pre-delay, detune and noise bed) and Glitch Scatter (scrambled, reversed and muted fragments). New characters are only ever appended to the `character` choice. Per-voice random streams used by these three are derived from the seed and the voice index, so they never disturb the sequences of the older characters.

**Formant** moves the spectral envelope of the swarm's source (STFT with a cepstrally smoothed envelope, ±12 semitones) while the harmonics (pitch) stay unchanged; the target hit stays natural.

## 5. Tone, mix, and dynamics requirements

Required shaping controls are Length, Shape, Tone, Bass Cut, Resonance, Tilt EQ, Presence, Air, Space, Drive, Transients, Formant, Mono Bass, Dry, Wet, Replace, Ducking, Pitch, volume start/end/tension, trim start/end, and pitch tension.

Controls that can create audible discontinuities SHOULD be smoothed or applied in pre-rendered processing rather than producing zippering in the realtime callback.

Output samples MUST remain finite for valid parameter states.

The Dry level, Swarm Wet level and the ducking gain are ramped linearly across each audio block (from the previous block's value), so host automation of these controls does not zipper. All other sound-design controls are applied in the pre-rendered swarm (re-rendered on a background thread after a change) and never in the audio callback. The combined swarm + hit + layer render is limited to a −0.17 dBFS peak ceiling so a preview or export never starts clipped. The **volume envelope** (Start / End / Tension) runs from the start of the swell to the hit and then holds its End level, so the default 0 → 1 fade builds into the hit rather than attenuating it.

**Build Stutter grids:** `stutterGrid` Straight (1/8 → 1/16 → 1/32), Triplet, Dotted, or Custom (a 16-step pattern in `stutterPattern`, 16 sixteenth-note steps per bar anchored so the right-most step is the one before the hit); `stutterSwing` (0–60 %) lengthens the first and shortens the second step of every pair. Straight with no swing is identical to the v0.0.1 stutter.

**Layers** (offline-synthesised into the render, zero audio-thread cost): Riser (noise sweep into the hit), Reverse Cymbal (reversed noise swell ending on the hit), Sub Drop (62 → 30 Hz at the hit) and Impact (noise + sub thump), with a Layer Tone control. With all layers at 0 the render is unchanged.

## 6. Transport, triggering, and preview

Any MIDI note-on MUST be able to trigger the swarm at the note's sample offset. Velocity SHOULD influence playback gain of both the swarm and the target hit.

When **Keytrack** (`keytrack`) is on, the note number transposes playback by resampling (C4/60 = original); the time scale changes with the pitch, so PDC alignment applies to untransposed playback only.

When **Hit on note (PDC)** (`align`) is on, the plug-in reports the hit position as latency (max 20 s) and delays the dry input by the same amount. Keytrack keeps the hit on the reported latency for every transposition (faster notes start later, slower notes start further into the swarm).

**Target Source** (`tgtSource`): *MIDI Note* (default); *Next Bar Line* starts the swarm `hitIndex` samples before every qualifying bar line (honouring `sequence` and the host time signature) so the hit lands exactly on the bar line, starting part-way into the swarm if the transport begins inside the swell; *Loop End* does the same for the host loop end; *Input Transient* starts the swarm on the first sample whose level passes `tgtThresh` (250 ms hold-off). The automatic modes require a playing host with tempo/ppq information (confidence falls otherwise), never report latency, and are sample-accurate within a block. A loop wrap in Loop End mode does not cut a ringing tail.

**Build Stutter** (`stutter`, 0–1, default 0) gates the second half of the swell at 1/8, then 1/16 (from 50%), then 1/32 (from 80%) notes at host tempo with 2 ms fades; the amount sets gate depth.

The PLAY action and Spacebar shortcut MUST trigger the same preview path.

**MIDI learn / CC mapping:** right-click any knob → MIDI learn, then move a CC, aftertouch or the pitch wheel. The learning knob shows `LEARNING...` and a mapped knob shows its source in its label; Esc cancels. One source maps to one parameter and one parameter has one source (the newest wins). Values travel from the audio thread to the message thread through atomics (no allocation or locking); mappings are saved with the project. CC 0 and 120–127 cannot be mapped. Because sound controls re-render the swarm, mapped changes are applied at the 30 Hz UI timer rate and are best used on Macro, levels, filters and morph.

The editor MUST also provide:
- Escape to stop active preview playback.
- R to randomize the current sound-design parameters.
- G to regenerate only the deterministic seed.
- H or F1 to open the help overlay (Esc closes it).
- B to switch the A/B comparison slot.

Keyboard shortcuts MUST be listed in the help overlay and in control tooltips where relevant.

**Performance variations:** four sound-design snapshots V1–V4 (stored with the project) for live switching. Clicking an empty slot stores the current sound; clicking a stored slot recalls it; right-click stores, clears, or enables *MIDI note switching*, where notes C1–D#1 (MIDI 24–27, base note stored in state) select V1–V4 instead of triggering the swarm (off by default). A recall re-renders in the background and the previous render keeps playing until the new one is ready.

**Randomize themes and locks:** Subtle, Wide, Dark, Wild, Tight and Cinematic. A theme result is a pure function of the current seed, the theme and the lock state (the same inputs always give the same sound); the classic `R` randomize keeps its original ranges. Six lock groups (Swarm, Convergence, Physics & 3D, Tone & Colour, Mix & Stutter, Character & Layers) keep their controls out of every randomize; locks are stored with the project, never in presets.

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

### 7.5 Preset browser, swarm view and accessibility

The preset browser (PRESETS button, with < > stepping) offers search over names and tags, tag filters (Vocal, EDM, Dubstep, Pop, FX, Subtle, Extreme, User), favourites (★), sort by name or recently used, an INIT entry, user presets in sub-folders with rename/delete, and `.pcbank` banks. Favourites and recents are stored in `Documents/Circuit Drift Labs/PreChorus/settings.xml`, never in presets.

A **preset bank** (`.pcbank`) is a zip containing `manifest.xml` (`PRECHORUS_BANK`, `product="PreChorus"`, `schema="1"`) and `presets/*.pcpreset`. Import validates every entry: it rejects path traversal (`..`, absolute paths, drive letters), duplicate names, entries over 256 KB, non-preset XML, other products and future schema versions, never extracts anything but validated presets, installs into `Presets/Banks/<bank name>/` using sanitised file names, and never reads or executes audio or code.

The **swarm view** replaces the waveform on demand and shows one row per voice (ordered by pan, top = left), a bar from its entry to the hit coloured by pitch offset, the convergence curve and the playhead; hovering a row shows voice number, entry time, pitch and pan. It reads render metadata only.

**Accessibility:** every control has an accessible title and description; knobs take keyboard focus (Tab) with a visible focus ring and respond to Up/Down (Shift = fine), PageUp/PageDown and Home/End; the 16-step stutter editor is keyboard operable; the status line announces changes to screen readers; High contrast (OPTIONS) switches to a high-contrast palette (≥ 7:1 text); Reduced Motion and the tooltip switch remain. Screen-reader behaviour has not been verified with Narrator or VoiceOver yet.

### 7.6 Discoverability

The help overlay MUST explain:
- the product workflow;
- source modes;
- character modes;
- keyboard shortcuts;
- factory presets;
- export and drag-to-DAW behavior;
- target sources, MIDI learn, layers, stutter grids, stems and sidechain, and the preset browser.

## 8. State and preset behavior

Host state is authoritative for project recall. APVTS parameters MUST be restored from saved state.

The state additionally stores the active capture slot and capture lock, the selected source file path (display only when audio is embedded), the Simple/Full view, the maximum source length, export settings, smart-capture options, randomize lock groups, the MIDI mapping table, the high-contrast flag and — by default (OPTIONS → *Store audio inside project*) — the source audio itself: the loaded file and all filled history slots as 24-bit FLAC, with a total budget of about 30 MB (items that do not fit are skipped and reported in the diagnostics). Demo sources are regenerated rather than stored. If embedded audio is corrupt or missing, PreChorus MUST remain stable, retain parameter state and fall back to the referenced file when it exists.

There are 24 factory presets, each with tags (see `getFactoryPresetTags`). Factory presets MUST alter sound-design parameters only. They MUST NOT replace loaded audio or overwrite capture history. Each factory preset first restores sound-design defaults (excluding seed, mix levels and the pitch/volume envelopes) so no value leaks from a previous preset.

State carries `stateVersion` (currently 2); all additions since v0.0.1 are optional properties with safe defaults, so older projects open unchanged (they open in the Full view). State with a different root type or a newer version is ignored and the current state is kept. Restore never changes the saved Source Mode. The editor size is stored as a view preference.

**A/B comparison:** two in-memory snapshots of the sound-design parameters. Switching stores the current slot and recalls the other; the first switch copies the current sound. Snapshots are not saved with the session.

**A/B morph:** `morph` (0–1) interpolates every numeric sound-design parameter between the A and B snapshots (choices, toggles, the seed and the stutter pattern switch at 0.5). It is applied from the message thread at most about 8 times a second because each change re-renders the swarm; the first use stores the live sound in the current slot and, if the other slot is unused, copies it there.

**User presets:** `.pcpreset` XML (`PRECHORUS_PRESET`, `schema` = 1, one `SNAPSHOT` child of parameter values) in `Documents/Circuit Drift Labs/PreChorus/Presets`. Loading validates root type, schema, size (≤ 256 KB) and that values are finite numbers, then clamps them to range; on failure the current sound is kept and the status bar explains why.

## 9. File export

Export MUST produce a stereo file from the same rendered swarm state used for preview, with Swarm Wet / Hit Dry applied. Settings (persisted with the project): format WAV 16-bit (TPDF dither, deterministic), WAV 24-bit (default), WAV 32-bit float or FLAC 24-bit; sample rate = render rate (default), 44.1, 48 or 96 kHz (Lagrange resampling); optional normalise to −1 dBFS; optional trailing-silence trim (below −70 dBFS, 20 ms kept). *Export stems* writes `_Swarm` (build-up only) and `_Hit` (impact and tail) files. DRAG TO DAW uses the same settings. MP3 export is not offered (JUCE cannot encode MP3).

Export failure MUST be surfaced in the non-modal editor status area.

## 10. Realtime safety

The audio callback MUST not perform file I/O, network I/O, modal UI work, or unbounded rendering work.

Rendering and file loading may occur outside the realtime callback. Shared rendered-state access MUST remain synchronized.

## 11. Baseline compliance record

| Shared baseline area | Status | Notes |
|---|---|---|
| Product profile declaration | Implemented | This document |
| Parameter reference (IDs, ranges, defaults) | Implemented | [PARAMETERS.md](PARAMETERS.md), generated from the code and checked in CI |
| Stable parameter IDs | Implemented | IDs namespace/APVTS; new parameters are only appended |
| Host automation attachments | Implemented | JUCE APVTS attachments |
| Parameter smoothing | Implemented | Dry / Swarm Wet / Ducking ramped per block; all other sound design is pre-rendered |
| Tooltips | Implemented | Per-control audible-result text; global on/off in OPTIONS |
| Keyboard discoverability | Implemented | Space, Esc, R, G, B, Ctrl/Cmd+Z/Y, H/F1; Tab focus with ring, arrow/PageUp/PageDown/Home/End on knobs |
| Reduced motion option | Implemented | Decorative orbit/flash can be disabled |
| High contrast / accessibility metadata | Implemented | Titles/descriptions on all controls, high-contrast palette, status announcements; screen-reader pass pending |
| State restore | Implemented | APVTS + embedded source audio + view/export/lock/MIDI-map preferences |
| Presets | Implemented | 24 tagged factory presets, versioned user presets, searchable browser, favourites, `.pcbank` banks |
| A/B comparison and morph | Implemented | Sound-design snapshots; morph interpolates them |
| MIDI learn | Implemented | Right-click knob; CC / aftertouch / pitch bend; persisted; Esc cancels |
| Undo / redo | Implemented | Sound-design edits, Ctrl/Cmd+Z, grouped bursts, depth 100 |
| Resizable UI | Implemented | 60–200 %, fixed aspect ratio |
| Latency reporting | Implemented | Hit position when PDC on (MIDI target); 0 for the automatic target sources; dry path delayed to match |
| Offline export | Implemented | WAV 16/24/32f, FLAC 24, rates, normalise, tail trim, stems |
| Bus layouts | Implemented | Mono/stereo/disabled main in, stereo out; optional sidechain and Swarm/Hit/Dry stems; others rejected |
| Automated unit tests | Implemented | `Tests/PreChorusTests.cpp` (core, engine, features, library, banks, buses, editor); run in all CI jobs. pluginval strictness 8 (VST3) and clap-validator 0.3.2 pass |
| Output peak/clip metering | Implemented | Stereo post-processing peak in dBFS with CLIP indicator |
| CI build matrix | Implemented | Windows (VST3/CLAP/Standalone, tests, installer + portable artifacts), macOS (AU/VST3/CLAP/Standalone, tests), Linux (tests, pluginval, clap-validator, ASan/UBSan, TSan) |
| Documentation | Implemented | README, this specification, MANUAL, PARAMETERS, release notes, in-app help |
| Signed installers / notarisation | Planned | Needs a Windows code-signing certificate and an Apple Developer ID (owner supplied) |
| Real-DAW test matrix | Planned | Needs testers; the Windows build has been exercised by CI and validators only |

## 12. Future work

Items that need an owner decision, certificates or testers rather than code:
1. Code signing (Windows) and signing + notarisation (macOS); AU validation with `auval` on a macOS runner.
2. Hand-tested DAW matrix (Reaper, FL Studio, Ableton Live, Bitwig, Cubase, Logic) including PDC, loop recovery, sidechain and stem routing.
3. Licensing / trial (V6) and a Lite edition (V7): both depend on business decisions.
4. Screen-reader verification of the accessible UI with Narrator and VoiceOver.
5. More factory presets from a sound designer (the browser and tags are ready for 60+), and demo audio for the product page.
