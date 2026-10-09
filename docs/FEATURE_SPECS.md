# PreChorus — Feature Specs & Implementation Plan (post-v0.0.1-beta)

Status: PLANNING. Baseline = `main` @ b38c2cd (JUCE 8.0.4, clap-juce-extensions pinned, state version 2, 65 parameters).
Audience: developers (human or AI agents). Each feature has a **Spec** (what/done-when) and a **Plan** (how/in what order).
Items marked **[VERIFY]** are assumptions about current code that the implementer must confirm in the first 15 minutes (cheap grep) before building.

---

## PART A — Ground rules (read first, applies to every feature)

### A1. Architecture facts that shape every feature
1. **Offline render model.** `PreChorusProcessor::render()` builds the whole swarm into a `RenderedSample` (audio + `gainLin`/`pitchSemi` envelopes at `envStep`, `hitIndex`). `processBlock` only plays that buffer back (swapped under a `SpinLock`, `renderLock`). Retired renders are kept in `retiredRendered` so nothing is freed on the audio thread.
   - Consequence: **any parameter that changes the sound triggers a re-render.** Features that move sound parameters continuously (morph, MIDI-CC mapped macro, per-note variants) must be **debounced/quantized**, never sample-accurate. **[VERIFY]** how `parameterChanged` (PluginProcessor.cpp ~l.250) schedules `render()` (thread, debounce, cancel).
2. **Audio-thread law (`processBlock`, l.~1196):** no allocation, no locks other than the existing `renderLock` SpinLock swap, no `juce::String`, no file I/O, no `MessageManager`. Currently clean (audited). Any new per-block code must keep that true; add it to the review checklist.
3. **Message-thread work:** capture commit, file load, render, export, presets.
4. **State:** `kStateVersion = 2`. Rule: **adding optional properties with safe defaults does NOT bump the version.** Bump only when meaning of existing data changes, and then add a migration step + test. Never let an older/newer-version state crash (tests exist; keep them green).
5. **Parameters:** 65 APVTS params. Rules: (a) **never rename or re-type an existing ID** (breaks host automation and saved projects); (b) **append-only for choice lists** (never reorder/insert — stored as indices); (c) new params use `juce::ParameterID { "id", 2 }` (version hint ≥ 2 — required for AU/VST3 stability of params added after first release); (d) new IDs are prefixed by feature (`ks_`, `emb_`, `mph_` …) and declared in one place per feature (see A3).
6. **Formats:** VST3 + CLAP + Standalone everywhere, AU on macOS. Plugin code `PrCh`, manufacturer `Shdv`.

### A2. Biggest merge-conflict hotspots (and the cure)
| Hotspot | Why | Cure |
|---|---|---|
| `Source/PluginProcessor.cpp` (1672 lines) and `.h` | Every feature edits it | **X1 refactor first** (split into modules, zero behavior change) |
| `Source/PluginEditor.cpp` (1487 lines) | Every UI feature edits it | X1 splits editor into panels |
| `Tests/PreChorusTests.cpp` | Every feature adds tests | X3: one test file per feature, shared tiny harness |
| `CMakeLists.txt` | New sources/defs | Glob-free but alphabetical `target_sources` list, one line per file; each PR adds lines only |
| `docs/RELEASE_NOTES.md`, README | Every PR adds a line | Each feature writes `docs/changelog.d/<feature>.md` fragment; release step concatenates |
| Parameter layout function | Every param feature adds lines | A3: per-feature `addParams_<feature>()` called from one list |

### A3. Target source layout after X1 (no behavior change)
```
Source/
  PluginProcessor.{h,cpp}        // thin: owns members, processBlock dispatch, state
  Params/ParamIDs.h              // all IDs (existing verbatim)
  Params/Params_<feature>.cpp    // addParams_<feature>(std::vector<...>&) one per feature
  Engine/Render.{h,cpp}          // render() and helpers (RenderedSample lives here)
  Engine/Capture.{h,cpp}         // live capture, slots, lock
  Engine/Source.{h,cpp}          // file load, hybrid, slicing
  Engine/Presets.{h,cpp}         // factory table + user presets
  Engine/Playback.{h,cpp}        // trigger/MIDI/PDC/ducking (audio thread)
  UI/Editor.{h,cpp}, UI/Panels/*, UI/LookAndFeel.*, UI/Help.*
Tests/
  TestMain.cpp (runner), TestUtil.h, Test_<feature>.cpp
```

### A4. Branching & merge protocol
- One short-lived branch per work package: `feat/<id>-<slug>` from latest `main`. PRs ≤ ~400 changed lines where possible; split otherwise.
- **Order of merging is defined in Part C.** Do not start a package before its dependencies are merged (or before its interface is agreed).
- Before opening a PR: `git merge origin/main` into the branch (not rebase on shared branches), run full local gate (A5), then push.
- One owner per file-group at a time (see Part C "ownership lanes"). Two agents never edit `Render.cpp` simultaneously.
- Draft PR first, mark ready only when CI is green on Windows + macOS + Linux.

### A5. Quality gate (every PR, automated in CI after X2)
1. Build VST3 + CLAP + Standalone + tests: **0 errors, 0 new warnings** (turn on `-Wall -Wextra` for our sources only; the 3 existing warning classes get fixed in X1).
2. Regression tests: all pass, **including golden-render test (X3)** — default presets render bit-identical unless the PR intentionally changes DSP (then the PR updates the golden hashes and says why).
3. `pluginval` strictness 8 (VST3): SUCCESS. `clap-validator`: 0 failed.
4. Sanitizers on the Linux test run: ASan+UBSan (and TSan for the capture/render threading tests).
5. State round-trip: save → load → save is byte-stable; newer-version and malformed state rejected.
6. Manual smoke list for UI PRs (screenshot attached).
7. Review checklist: audio-thread rules (A1.2), param rules (A1.5), state rules (A1.4), no new `new/delete`, no `==` on floats.

### A6. Definition of Done (per feature)
Spec acceptance criteria met · tests added · docs fragment written · tooltips/help text updated · changelog fragment · pluginval/clap-validator green · reviewed against A5.7 · no TODOs left without an issue number.

---

## PART B — Feature specs

Template per feature: **Goal · Behavior · State/Params · UI · Edge cases · Acceptance · Plan (files, steps, risks, deps, size)**.
Size: S ≤ 1 dev-day, M 2–3, L 4–7, XL 8+ (human days; an AI agent is typically 2–4× faster on code, not on review/DAW testing).

### FOUNDATION (do first — enables everything else)

#### X1 — Source refactor (no behavior change)
- **Goal:** remove merge hotspots (A3). **Behavior:** identical. **Acceptance:** golden renders (X3) bit-identical; all existing tests pass; warnings fixed (`-Wunused-parameter` in editor l.130, float `==` in editor l.351 & `PluginEditor.h:93`, `processBlock(double)` hidden-overload — add `using AudioProcessor::processBlock;`).
- **Plan:** (1) land X3 golden test **before** moving code. (2) Move code by whole functions, one module per commit, compile after each. (3) No renames, no logic edits. (4) Single PR, merged fast; everyone else rebases after. Risk: long-lived conflict with other work → freeze other PRs for the day. Size: M. Deps: X3.

#### X2 — CI hardening
- **Goal:** CI enforces A5. **Spec:** add to `build-linux.yml` (new): build, tests under `xvfb-run`, pluginval strictness 8 (download pinned release), clap-validator 0.3.2 (pinned), ASan/UBSan job, optional TSan job. Add `ccache` (actions/cache) to all builds — cuts rebuild from ~3 min to <1 min. Add `-Wall -Wextra -Werror` for `Source/` only. Release workflow gains Linux job (PR #4) + checksums file `SHA256SUMS.txt`.
- **Acceptance:** a deliberate RT violation (test branch) or data race fails CI. **Plan:** workflows only (no source overlap → can run in parallel with X1). Size: M. Risk: pinned tool URLs rot → vendor version + sha256 in the workflow.

#### X3 — Test harness + golden renders
- **Spec:** split `Tests/PreChorusTests.cpp` into `TestMain.cpp`, `TestUtil.h` (processor factory, offline block runner, synthetic vowel source, `hashBuffer()`), `Test_<area>.cpp` (move existing 15 tests verbatim). Golden test: for each of the 10 factory presets (and 3 characters × 2 source modes) with fixed seed, 44.1k and 48k, render via `render()` and compare an FNV-1a hash of quantized (24-bit) samples to `Tests/golden/*.txt`. Plus: finite-output fuzz (random params, 200 iterations), and an RT-safety smoke test that runs `processBlock` 10k times under a malloc-counting hook (`mallinfo2`/interposed `operator new`) asserting 0 allocations. **[VERIFY]** render is deterministic (seed, no uninitialized memory, no time-based RNG); if not, fix determinism first (it is a bug anyway).
- **Acceptance:** golden test passes twice on two machines (Linux/Windows may differ in float math → hash per platform, or compare with tolerance via RMS diff ≤ 1e-6 instead of hash; choose RMS-diff). Size: M. Deps: none.

---

### ESSENTIAL

#### E1 — Longer source & capture
- **Goal:** loaded files and captures are no longer silently truncated at 12 s / 8 s.
- **Behavior:** `Max source length` option (Options menu, stored in editor settings, default 30 s, choices 12/30/60/120 s). Loading a longer file: if over limit, show non-blocking status "Cropped to 30 s (file is 74 s)" and offer **"Use region…"** (start offset = drag on waveform). Live capture buffer and manual capture length scale with the option; memory shown in tooltip (48 kHz stereo float ≈ 23 MB/min/slot × 8 slots).
- **State:** `srcMaxSeconds` stored as property (not a param; default keeps old behavior if absent = 12 for old sessions).
- **Performance gate:** render time for 32 voices × longest source must stay interactive. **[VERIFY]** measure `render()` at 12 s vs 60 s (Release, 48 kHz). If > ~1.5 s: render on a background thread with cancel-on-change (`juce::ThreadPool` job holding a copy of inputs; result swapped in under `renderLock` exactly as now) and show "Rendering…" status. Reuse existing swap path; do not add new locks on the audio thread.
- **Edge:** sample-rate mismatch uses existing resampling; very long + hybrid mode; slot memory with 8 × 120 s (cap total at e.g. 1.5 GB → refuse with message).
- **Acceptance:** 60 s file loads, plays, exports; no audio-thread allocation (X3 test); old sessions load unchanged; render never blocks UI > 100 ms.
- **Plan:** files `Engine/Source`, `Engine/Capture`, `Engine/Render`, editor Options + waveform region drag. Steps: (1) constant→member `maxSourceSamples`; (2) prepareToPlay buffer sizing (l.226 `12.0` and `dryDelay`) ; (3) loader l.1484; (4) background render if needed; (5) region UI; (6) tests (long-file load, memory cap, old-state default). Risk: M (threading). Size: L. Deps: X1,X3.

#### E2 — Keytrack that keeps the hit aligned
- **Goal:** transposing by MIDI note does not shift where the impact lands.
- **Current:** `rate = 2^((note-60)/12)` resamples the render (l.~1368): pitch and duration change together; hit time = `hitIndex / rate`, PDC reports `hitIndex` ⇒ misaligned for every note except C4.
- **Behavior (two phases):**
  - **Phase 1 (cheap, ships first):** compensate start offset: for rate ≥ 1 delay playback start by `hitIndex·(1 − 1/rate)` samples so the hit lands at the PDC point; for rate < 1 the hit would need to start *before* the note (impossible) → **clamp transposition to ≥ −0 st when PDC is on** or show a one-time warning "downward keytrack moves the hit late; use Time-Preserve (beta)". Add `keytrackMode` choice: `Speed (classic)`, `Time-Preserve`.
  - **Phase 2:** `Time-Preserve`: pitch-shift the *render* by ratio r keeping duration, via a time-preserving shifter (WSOLA or phase vocoder; **Opus-class** DSP task). Cache shifted renders per semitone offset (LRU of ≤ 6 entries, built on the message/background thread on first use; the first note of a new pitch plays the speed-based fallback with Phase-1 compensation until the cache entry is ready). Never compute on the audio thread.
- **State/params:** `keytrackMode` (choice, append-only), existing keytrack param unchanged.
- **Acceptance:** for notes C2…C6, onset of the hit within ±1 sample of expected (test: sine-impulse target, detect peak); no allocation in processBlock; golden unaffected when keytrack off.
- **Plan:** `Engine/Playback`, new `Engine/PitchShift.{h,cpp}`. Steps: Phase 1 + tests (S/M) → separate PR; Phase 2 (L). Risk: Phase 2 quality/artifacts on vocals (use window ≥ 40 ms, test on synthetic vowel + real vocal). Deps: X1,X3.

#### E3 — Embed source audio in the project
- **Goal:** sessions are portable; no "file missing".
- **Behavior:** Options → "Store audio inside project" (default ON for new projects). State stores the active source and (optionally) all 8 capture slots as **FLAC bytes in a base64 child node** (`juce::FlacAudioFormat` → `MemoryOutputStream`). Size cap per project 32 MB (otherwise fall back to path-only with status warning). On load: decode on a background thread (existing "deferred external sample loading" path — **[VERIFY]** reuse), so `setStateInformation` stays fast.
- **State:** `embeddedSources` ValueTree child: `{slot, sampleRate, channels, flac(base64)}`; absent = old behavior. No version bump (optional property).
- **Edge:** captures made after save; host with tiny state limits (some hosts choke > ~16 MB: default to active-source-only + a "Embed all slots" checkbox); corrupt FLAC → ignore + status message, never crash.
- **Acceptance:** save project, delete the audio file, reload → identical render (golden compare); malformed embed rejected (add to malformed-state test); state size within cap.
- **Plan:** `Engine/Source`, `PluginProcessor::get/setStateInformation`. Steps: encode/decode helpers + unit tests → state wiring → Options UI → size guard. Risk: S/M. Size: M. Deps: X1.

#### E4 — Signed Windows installer & macOS release
- **Owner tasks (cannot be coded):** buy/obtain a **Windows code-signing cert** (OV/EV, or Azure Trusted Signing if eligible) and an **Apple Developer Program** membership (Developer ID Application + Installer certs). Store as GitHub secrets (`WIN_CERT_PFX_BASE64`, `WIN_CERT_PASSWORD`, `APPLE_CERT_P12_BASE64`, `APPLE_CERT_PASSWORD`, `APPLE_ID`, `APPLE_TEAM_ID`, `APPLE_APP_PASSWORD`).
- **Windows:** in `release.yml`, after build run `signtool sign /fd SHA256 /tr <timestamp> /td SHA256` on `PreChorus.exe`, `.vst3`, `.clap`, then sign the Inno installer (`SignTool` directive in `.iss`).
- **macOS:** new `release-macos` job: build AU/VST3/CLAP/Standalone as **universal** (`-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`), `codesign --options runtime --timestamp`, `pkgbuild`/`productbuild` installer (installs to `/Library/Audio/Plug-Ins/{Components,VST3,CLAP}`), `xcrun notarytool submit --wait`, `xcrun stapler staple`. Add `SHA256SUMS.txt`.
- **Acceptance:** fresh Windows 11 VM: no SmartScreen "unknown publisher" (EV) / publisher shown (OV); fresh macOS: opens with no Gatekeeper block; `spctl -a -vvv` accepted; `auval -v aufx PrCh Shdv` passes.
- **Plan:** workflows + `installer/` only (no source overlap → run in parallel from day 1). Risk: notarization failures (hardened runtime entitlements) — iterate on a throwaway tag `v0.0.0-signtest`. Size: L. Deps: owner certs.

#### E5 — Real-DAW test pass
- **Matrix (min):** Windows: Reaper, FL Studio, Ableton Live, Bitwig, Cubase/Studio One; macOS: Logic (AU), Live, Reaper; Linux: Reaper, Bitwig. Formats: VST3, CLAP (Reaper/Bitwig/FL), AU.
- **Checklist per host (record pass/fail + notes in `docs/DAW_TEST_MATRIX.md`):** instantiate/remove 20×; project save/recall incl. embedded audio (E3); automation of ≥ 5 params; PDC: drop lands on the grid with latency compensation on/off; loop/scrub/jump recovery; MIDI trigger sample accuracy; sample-rate change 44.1/48/96; buffer size 32…2048; offline render/bounce equals realtime; multiple instances; resize window; 100% CPU headroom test at 32 voices; close project while rendering (no crash).
- **Plan:** owner/tester task + a bug template (`.github/ISSUE_TEMPLATE/daw_bug.yml`: host, version, OS, format, steps, diagnostics from E8). Size: L (calendar time dominated by testers). Deps: a build to test; re-run on each release candidate.

#### E6 — Undo / redo
- **Goal:** Ctrl/Cmd+Z / Shift+Z for parameter edits, randomize, reset, preset load, A/B copy.
- **Behavior:** pass a `juce::UndoManager` to the APVTS constructor **[VERIFY]** current constructor args; wrap multi-parameter actions (randomize, preset, reset) in `beginNewTransaction("Randomize")`. Non-parameter state (source audio, slots) is **not** undoable (documented). Header shows ◀ ▶ buttons + tooltip with action name. Depth 100. Keys handled in editor `keyPressed` (hosts that swallow keys: also buttons).
- **Edge:** undo during active render (just sets params; render debounce handles it); don't record automation-driven changes (use `ScopedValueSetter`/only record from UI gestures: begin/endGesture hooks exist).
- **Acceptance:** randomize → undo restores exact values (test via snapshot compare); 100-step cap; no growth when host automates.
- **Plan:** `PluginProcessor` ctor, `Engine/Presets`, editor header. Size: M. Risk: undo recording host-driven changes → test. Deps: X1.

#### E7 — Manual & quick-start
- **Deliver:** `docs/MANUAL.md` (quick start in 5 steps with screenshots; every control; workflows: vocal build, EDM riser, MIDI-triggered, export/drag-to-DAW; troubleshooting; FAQ; glossary), a 1-page PDF quick-start (generated in CI from markdown), and an in-plugin **first-run overlay** (3 cards: load/capture → press Space → drag to DAW; "Don't show again" stored in editor settings). Help menu gets "Open manual" (opens URL; app stays offline otherwise — clicking is user-initiated).
- **Plan:** writing task (Haiku/Sonnet-class OK); overlay is a small editor component (S). Needs screenshots after UI settles → do after U1. Size: M.

#### E8 — Diagnostics ("Copy report")
- **Behavior:** Help → "Copy diagnostics" puts text on the clipboard: version, build date/commit, format (VST3/CLAP/AU/Standalone), OS, host name/version (`PluginHostType`), sample rate, block size, channel layout, latency, voices/params summary (non-sensitive numeric values), last 50 status-bar messages, last error codes. **No audio, no file paths beyond file *name*, no network.** Standalone also writes a rotating log (`Documents/Circuit Drift Labs/PreChorus/Logs`, 1 MB × 3) for crash triage.
- **Acceptance:** report < 8 KB; contains no user paths (test regex); works in all formats.
- **Plan:** small `Diagnostics.{h,cpp}` + editor menu item. Size: S. Deps: none (parallel).

---

### EASIER TO USE

#### U1 — Simple view (one-knob mode)
- **Behavior:** header toggle `SIMPLE | ADVANCED` (stored per-instance in editor state, default SIMPLE for first run). Simple shows: source section, preset menu, big **BUILD** knob (maps to existing `MACRO`), **Length**, **Character**, **Width**, **Brightness** (tilt), Preview/Drag-to-DAW. Advanced = current UI. **No new DSP, no new params, no new automation IDs** (zero state risk).
- **Acceptance:** all Simple controls bind to existing parameters; switching views never re-renders; keyboard focus order sane (U7).
- **Plan:** `UI/Panels/SimplePanel`, layout logic only. Size: M. Deps: X1.

#### U2 — Preset browser & factory expansion
- **Behavior:** popup browser with search, **tags** (Vocal/EDM/Dubstep/Pop/FX/Subtle/Extreme), ★ favorites (stored in user settings file, not in presets), sort (name/recent), prev/next arrows, "Init" entry, show user presets in their own folder tree, rename/delete user presets, import `.pcpreset`/`.pcbank` (V5).
- **Factory presets → data:** convert the hard-coded `loadFactoryPreset` switch (l.~310–430) to a **table/XML (BinaryData)** of `{name, tags[], author, values{id: value}}` applied on top of defaults (keeps the "no leak" guarantee). Grow 10 → 40–60 (needs sound-design time: owner/sound designer supplies; dev provides tooling: "Save as factory entry" dev-only menu that dumps the XML).
- **Tests:** every factory preset loads, renders finite, within level (peak ≤ 0 dBFS), hash in golden (X3); unknown IDs in a preset ignored safely; preset schema versioned.
- **Plan:** `Engine/Presets` + `UI/PresetBrowser`. Steps: data conversion with golden check proving the existing 10 render identically → browser UI → tags → favorites → new presets. Size: L. Deps: X1,X3. Risk: golden mismatch on conversion → fix before UI work.

#### U3 — Smarter targeting (what the host allows)
- **Reality check:** plugins cannot see future timeline markers or clip ends. Available: tempo, time signature, bar/beat position, loop points (`AudioPlayHead::getPosition()->getLoopPoints()`), transport state, incoming MIDI/audio.
- **Behavior:** new `Target Source` choice (append-only): `MIDI note (current)`, `Next bar line (auto)`, `Loop end`, `Input transient (sidechain/input onset)`. `Next bar line`: swarm length snaps to the Sync Length so the hit lands on bar N; `Loop end`: length = distance to loop end when loop enabled. `Input transient` reuses ducking envelope detector with a threshold param. Confidence badge reflects the chosen source (exists).
- **State/params:** `tgt_source` (choice), `tgt_thresholdDb` (float, only for transient). **[VERIFY]** how Sync Length/Follow Tempo currently compute length (l.~1130–1160 area and processBlock trigger section).
- **Acceptance:** with host mock (test PlayHead) the hit lands within 1 sample of bar line for 3 tempos and 3 time signatures; loop-end mode degrades gracefully when no loop.
- **Plan:** `Engine/Playback`, `Engine/Render` (length calc). Size: L. Risk: hosts report loop points inconsistently (test in E5). Deps: X1,X3.

#### U4 — Smart capture
- **Behavior:** after a capture finishes: auto-trim leading/trailing silence (−50 dBFS, 10 ms fade), optional normalize to −1 dBFS (option), detect phrase boundaries (energy gate) and show **Accept / Retake / Extend** in the status bar; `Retake` reuses slot. A capture-ready indicator before arm shows input level.
- **Acceptance:** golden captures from synthetic phrase with 1 s silence both ends trim to ±5 ms; never trims below 50 ms; lock honored.
- **Plan:** `Engine/Capture` (post-process on message thread in `commitFinishedCapture`) + small UI. Size: M. Deps: X1.

#### U5 — Visual feedback (swarm view)
- **Behavior:** a new "SWARM" tab/overlay drawn from render metadata: horizontal timeline of voice entry (one row per voice, bar length = audible span, color = pitch offset, vertical position = pan), convergence curve overlay (spread vs time), playhead. Hover shows voice #, start, pitch, pan.
- **Data:** extend `RenderedSample` with `std::vector<VoiceInfo>{start,end,pitchSemi,pan,gain}` filled in `render()` (message-thread side; read-only in UI via the existing shared_ptr). 
- **Acceptance:** 60 fps at 32 voices; no audio-thread access to UI data; resizes with window.
- **Plan:** `Engine/Render` (metadata only; must not alter audio → golden unchanged) + `UI/SwarmView` (Component with cached `Path`s, repaint on render swap). Size: L. Deps: X1,X3.

#### U6 — MIDI learn & CC mapping
- **Behavior:** right-click any knob → "MIDI Learn", move CC → bound; "Clear". Mappings stored in state (`midiMap: {cc, channel, paramID}`); processBlock converts CC → param via atomic setter (`setValueNotifyingHost` is **not** audio-thread-safe for all hosts → push to a lock-free FIFO drained on a timer/message thread). Mod wheel / aftertouch targets allowed.
- **Warning:** CC-mapped *sound* params re-render (A1.1) → mapped changes are debounced 100–200 ms; document "best for Macro/level/filter, not for live scrubbing". Params that don't trigger render (levels, ducking, mix) feel instant. **[VERIFY]** which params re-render.
- **Edge:** many hosts don't deliver CC to effects (document); learn mode times out; conflicts (one CC → one param, last wins).
- **Acceptance:** learn/clear/persist round-trip; no allocation on audio thread; FIFO overflow safe.
- **Plan:** `Engine/Playback` (CC intake), `Params/MidiMap`, editor context menu. Size: M. Deps: X1.

#### U7 — Accessibility & UI scale
- **Behavior:** JUCE 8 accessibility enabled: every control gets `setTitle`/`setDescription`, tab-order, focus ring, keyboard knob control (↑↓ ±1 step, Shift fine, Home/End), screen-reader-friendly status bar (live region). High-contrast theme toggle, font scale 100–150%, "Reduce motion" option (stops animations/visual meters easing).
- **Acceptance:** Windows Narrator / macOS VoiceOver read control names+values; full UI operable by keyboard; contrast ≥ 4.5:1 for text in both themes (check with script).
- **Plan:** `UI/*` sweep after U1/U5 stabilize (otherwise churn). Size: M. Deps: U1,U5.

---

### MORE VALUABLE

#### V1 — Multi-output / stems
- **Behavior:** optional extra stereo output buses (disabled by default): `Wet`, `Dry`, `Swarm-only` (pre-FX) — enabled in the host's routing. Main out unchanged (so projects behave the same). Offline export: "Export stems" writes the same set as WAV.
- **JUCE:** `BusesProperties().withOutput("Out", stereo, true).withOutput("Wet", stereo, false)…`; `isBusesLayoutSupported` accepts main stereo + any subset of aux; `processBlock` writes to `getBusBuffer`. VST3 and AU handle this; **CLAP**: via clap-juce-extensions audio-ports (verify with clap-validator); Standalone ignores extras.
- **Risk:** host quirks (FL/Logic routing), channel-count-change handling — default aux OFF mitigates. Must not break mono-in/stereo-out layouts.
- **Acceptance:** with aux enabled, sum(Wet+Dry stems) equals main out within −120 dB; pluginval/clap-validator green; main output byte-identical to before when aux disabled.
- **Plan:** `PluginProcessor` bus setup + `Engine/Playback`. Size: L. Deps: X1,X3.

#### V2 — Export options
- **Corrections to earlier idea:** JUCE **cannot encode MP3**; skip MP3. Offer **WAV 16/24/32f, FLAC (24-bit), OGG optional (needs `JUCE_USE_OGGVORBIS=1`)**, sample-rate choice (host/44.1/48/96), **dither** (TPDF) when going to 16-bit, normalize option, tail trim, and "Bake into host" = drag-to-DAW pad uses the same settings.
- **Acceptance:** exported WAV matches in-plugin render (null test < −120 dB at 32f); 16-bit dither deterministic with seed; file names sanitized.
- **Plan:** `Engine/Export` (move `exportWav`), editor Export dialog. Size: M. Deps: X1.

#### V3 — Sidechain input
- **Behavior:** optional sidechain input bus (stereo, off by default) selectable as source for: Ducking, Threshold capture, and `Input transient` target (U3). Param `sc_source` choice (`Main in`, `Sidechain`, append-only).
- **Risk:** plugin gets an extra input bus → hosts expose sidechain routing; layout negotiation bugs; AU needs `aufx` bus config. CLAP audio-port in-place flags.
- **Acceptance:** with sidechain silent+main loud and `sc_source=Sidechain` → no duck; reverse → duck; layouts with/without sidechain pass pluginval.
- **Plan:** `PluginProcessor` buses + detectors. Size: M. Deps: X1, U3 (shares detector). Do V1 and V3 in one bus-layout PR to avoid two rounds of layout bugs.

#### V4 — Key detection & scale snapping
- **Behavior:** "Detect key" button analyses the active source (chroma via FFT, 12-bin profile correlation with Krumhansl–Schmuckler major/minor) → sets `scaleRoot` and `Scale Lock` (Major/Minor). Existing Scale Lock stays; add `scaleRoot` (0–11) param. **[VERIFY]** whether a root param already exists (grep `scaleLock` use at l.~15–30: `noteInOct` suggests quantization relative to C).
- **Acceptance:** ≥ 90% key accuracy on a 20-clip synthetic set (diatonic melodies in all 24 keys), reports confidence; no effect when Scale Lock = Chromatic.
- **Plan:** `Engine/KeyDetect.{h,cpp}` (pure, unit-testable) + button. Size: M. Deps: none for the algorithm; UI after U1.

#### V5 — Preset banks (shareable)
- **Behavior:** Export selected user presets to `.pcbank` (a zip of `.pcpreset` + `manifest.xml` with name/author/version/tags via `juce::ZipFile`); Import validates every entry (schema, product id, version, size ≤ 256 KB each, path traversal safe — reject `..`, absolute paths, duplicates), previews list, installs into a bank folder. Never executes or reads audio from banks.
- **Acceptance:** malicious zip tests (zip-slip, huge entry, bad XML) rejected; round-trip equal.
- **Plan:** `Engine/Presets` + dialog. Size: M. Deps: U2.

#### V6 — Licensing & trial (offline)
- **Owner decisions needed first:** business model (paid? price? per-machine or per-user? how many activations?), refund/transfer policy, legal text (EULA is in `LICENSE`).
- **Behavior:** license file `PreChorus.lic` (JSON payload + **Ed25519 signature**, public key compiled in; private key kept offline by the owner) containing name/email-hash/edition/expiry-optional. Import via drag-drop onto the plugin or menu. No network, no machine lock by default (document trade-off); optional soft machine hint. Unlicensed = **demo**: after N minutes of audio or per-render, add a short noise burst/mute every 20 s (pick one, owner decides) and a visible "DEMO" badge.
- **Security note:** offline licensing is deterrence not DRM; don't spend effort on anti-crack. Use a small vendored public-domain Ed25519 implementation (e.g. orlp/ed25519) **[owner must approve the dependency]**; **Opus-class review of verify code and constant-time compare**.
- **Acceptance:** valid → full; tampered/expired → demo; garbage file never crashes; license check never on audio thread (verified at load, flag read atomically); signature tool `tools/make_license.py` (private key arg).
- **Plan:** `Licensing/*`, editor badge, tools script, installer drops no key. Size: L. Deps: owner decisions, X1.

#### V7 — Lite edition
- **Behavior:** CMake option `-DPRECHORUS_EDITION=LITE` defines `PRECHORUS_LITE=1`: max 8 voices, no export/drag (or WAV 16-bit only), fewer characters, no Simple→Advanced toggle (owner to define), different **plugin code `PrCL`**, product name `PreChorus Lite` so both can coexist. Parameter IDs identical where features exist (so presets cross-load; missing ones ignored).
- **Plan:** feature flags centralised in `Edition.h` (`kMaxVoices`, `kHasExport`…) — retrofit while doing X1 so later features consult it. CI matrix builds both. Size: M. Deps: X1, V6 (upgrade path). Risk: untested flag combinations → CI builds & runs tests for both editions.

---

### MORE FUN

#### F1 — Themed randomize + locks
- **Behavior:** Randomize menu: `Subtle / Wide / Dark / Wild / Tight / Cinematic` (each = ranges per parameter group in a table), + **lock icons** per control group (lock = excluded from randomize/preset-load optional). History: last 10 randomizations in A/B-like strip (works with E6 undo). Locks stored in editor state (not presets).
- **Acceptance:** same seed+theme → same result; locked params never change; output finite (fuzz).
- **Plan:** `Engine/Presets` (randomizer table), UI lock icons. Size: M. Deps: X1, E6.

#### F2 — A↔B Morph
- **Behavior:** `Morph` slider (0–1) interpolating numeric params between A and B snapshots (choices/toggles switch at 0.5). **Re-render model constraint (A1.1):** morph is *committed* (debounced 150 ms, or on mouse-up) → not for fast automation; document and name it "Morph (renders on release)". Automation allowed but re-render rate limited to ≤ 4/s; renders cancel-and-replace.
- **State/params:** `mph_amount` (float). 
- **Acceptance:** morph 0 == A exactly, 1 == B exactly (bit-for-bit golden); no stutter in audio during render; CPU spike bounded.
- **Plan:** requires background render + cancel (shared with E1). Do after E1. Size: M. Deps: E1, X1.

#### F3 — Performance variations
- **Behavior:** 4 **variation slots** (V1–V4) = sound-design snapshots (reuse `soundDesignSnapshot`). MIDI note ranges (configurable base note, default C1–D#1 select V1–V4; ignored by triggering) or buttons. Selected variation re-renders in background; the previous render stays active until ready (no silence). Optionally pre-render all 4 (memory ×4, option).
- **Acceptance:** selecting during playback never glitches audio; state persists; 4 variations round-trip.
- **Plan:** `Engine/Presets` + `Playback`; shares background render with E1/F2. Size: M. Deps: E1.

#### F4 — Impact & riser layers
- **Behavior:** `Layers` section: **Riser** (filtered noise sweep, 0–100%, rising pitch), **Reverse cymbal** (noise burst reversed), **Sub drop** (sine 60→30 Hz at the hit), **Impact** (short noise+sub thump); each: level, tone, length-to-hit. Synthesized in `render()` into the output buffer ending/starting at `hitIndex` (offline → zero audio-thread cost). Ducked/filtered like the swarm? Default: pass through shaping filters (toggle).
- **State/params:** `lay_riser`, `lay_rev`, `lay_sub`, `lay_impact` (levels), `lay_tone`, `lay_follow` (bool). 
- **Acceptance:** layers off ⇒ golden unchanged; hit alignment unchanged (`hitIndex` same); no DC offset (< −80 dBFS); finite.
- **Plan:** `Engine/Layers.{h,cpp}` pure functions + call in `render()`; new editor panel. Size: M. Deps: X1,X3. Good parallel candidate (disjoint new files).

#### F5 — New voice characters
- **Behavior:** append to `character` choice (never reorder): `Vowel Choir` (parallel bandpass formant bank morphing a–e–i–o–u), `Stadium Crowd` (diffuse, noisy, detuned, short pre-delays), `Glitch Scatter` (slice scramble + gate + reverse bits). Each ≤ ~150 lines in the existing character switch. 
- **Acceptance:** new IDs don't change existing characters' golden; each finite and level-safe; presets using them.
- **Plan:** `Engine/Render` character section → conflicts with E2/F4 if edited together; **ownership lane: Render.cpp, one agent at a time** (see C). Size: L. Deps: X1,X3.

#### F6 — Stutter variations
- **Behavior:** `stutterGrid` choice (append): `Straight`, `Triplet`, `Dotted`, `Custom`; **Custom 16-step pattern** editor (click steps), `stutterSwing` 0–60%, `stutterShape` (gate depth). Pattern stored in state ValueTree. Render-time like existing Build Stutter.
- **Acceptance:** Straight output identical to current (golden); pattern persists; triplet timing exact to host tempo within 1 sample.
- **Plan:** `Engine/Render` (stutter section) + small UI component. Size: M. Deps: X1,X3.

#### F7 — Built-in demo source
- **Behavior:** Source menu: "Demo: Ahh / Hey / Chord" generated procedurally (additive vowel with formants + vibrato; 'Hey' with noise onset) at host rate → new users hear PreChorus instantly with zero files. Fully synthetic = no sample licensing. Also used in tests (X3).
- **Plan:** `Engine/DemoSource.{h,cpp}` (pure, deterministic) + menu. Size: S. Deps: none (also feeds X3 — build first, tiny).

---

### MORE DESIRABLE (mostly non-code; needs owner)

#### D1 — Presentation & UI polish
- Brand pass: logo, consistent palette, typography, knob style, animated swarm background tied to render meta (U5), loading states, 100/150/200% asset sharpness (vector `Path`s, no bitmaps), light/dark. Provide a design brief + Figma/mock from a designer; devs implement in `UI/LookAndFeel`. **Owner/designer task.** Size: L–XL. Do after U1/U5 (structure stable).

#### D2 — Positioning & site
- One-sentence pitch, 3 audio demos (before/after, 15–30 s), a 60 s video, product page with download buttons (links to GitHub Releases), short FAQ, press kit (logo, screenshots). Owner task; dev supports with release URL JSON and `SHA256SUMS`.

#### D3 — Trust signals
- `CHANGELOG.md` (generated from `docs/changelog.d/*`), public `ROADMAP.md`, issue templates (bug/DAW/feature), `SECURITY.md`, response SLA statement, "Known issues" in release notes, semantic version tags. Size: S.

#### D4 — Cross-platform parity
- macOS universal (E4), **Linux aarch64** build (GitHub `ubuntu-24.04-arm` runner; **[VERIFY]** availability for this repo's visibility/plan), AppImage/`.deb`/`.tar.gz` packaging + install script for Linux (`~/.vst3`, `~/.clap`), optional Windows ARM64 later. Size: M. Deps: X2.

---

## PART C — Implementation plan (sequencing, lanes, parallelism)

### C1. Waves
| Wave | Contents | Parallel lanes | Exit criteria |
|---|---|---|---|
| **0 Foundation** (≈ 1 week) | F7 (demo source, tiny) → X3 → X1 ; X2 in parallel; E4 certs/pipelines in parallel; D3 trust docs | Lane P (code: F7→X3→X1), Lane CI (X2), Lane REL (E4 pipelines) | Golden test green; refactor merged; CI enforces A5 |
| **1 Reliability** (≈ 2 weeks) | E1, E3, E2-phase1, E6, E8; E5 starts on first RC | Lane Source (E1→E3), Lane Play (E2), Lane UI (E6,E8) | DAW matrix pass on 3 hosts; no known crashers |
| **2 Usability** | U1, U2, U4, E7 (after U1), U3, U6, U5, U7 | Lane UI (U1→U2→U5→U7), Lane Engine (U4→U3), Lane Play (U6) | Quick-start works for a new user in < 2 min |
| **3 Value & fun** | V1+V3 (one PR), V2, V4, V5, F4, F1, F6, F7→done, F5, E2-phase2, F2, F3 | Lane Render (F4→F5→F6 sequential!), Lane Bus (V1/V3), Lane Util (V2,V4,V5) | Beta 2 |
| **4 Commercial** | V6, V7, D1, D2, D4 | Lane REL + owner | Release 1.0 RC |

### C2. Ownership lanes (prevents merge conflicts)
- **Render.cpp lane** (one agent): E1(render part), F4, F5, F6, F2/F3 render wiring, U5 metadata. Order: U5-meta → F4 → F6 → F5. Never parallel.
- **Playback.cpp lane:** E2, U3, U6, V1/V3 (processBlock changes). Sequential.
- **Editor lane:** U1 → U2 → U5 → U7; E6/E8 menus slot in via small PRs *between* these.
- **New-file lanes (safe parallel):** F7, V4, V2(Export), V5, Licensing, X2, E4, docs.

### C3. Merge-day mechanics
1. Merge order = wave order; X1 merges alone.
2. Each PR touches shared files only to add one line (param list entry, `target_sources` line, menu item). Reviewer checks the diff of shared files first.
3. Daily `main` → branch merge for branches older than 1 day.
4. If two PRs conflict in a hotspot: the later author resolves, re-runs the full gate. Never "fix CI" by skipping tests.
5. Release cut: tag `vX.Y.Z-beta.N` from `main` after the DAW matrix delta passes.

### C4. Risk register (top)
| Risk | Mitigation |
|---|---|
| Re-render storms from continuous control (morph/MIDI CC) | Debounce + cancel/replace background render (E1) before F2/F3/U6 |
| Bus layout bugs (V1/V3) across hosts | Default aux OFF; single PR; pluginval + E5 matrix |
| DSP quality of time-preserving shift (E2.2) | Opus-class owner, listening tests, ship Phase 1 first |
| Notarization/signing delays (E4) | Start day 1, throwaway tags |
| State growth/host limits (E3) | Size caps, default active-only |
| Float determinism across platforms (X3) | RMS-diff tolerance not hashes |
| Scope creep | Only items in this doc enter waves; new ideas go to ROADMAP.md |

### C5. Estimates (human dev-days, excluding calendar waits)
X1 3 · X2 3 · X3 3 · F7 0.5 · E1 5 · E2 (1+6) · E3 3 · E4 5 · E5 (testers) · E6 2 · E7 3 · E8 1 · U1 3 · U2 6 · U3 5 · U4 3 · U5 5 · U6 3 · U7 3 · V1+V3 6 · V2 3 · V4 3 · V5 3 · V6 5 · V7 3 · F1 2 · F2 2 · F3 3 · F4 3 · F5 5 · F6 3 · D1 8 · D2/D3 3 · D4 3. Total ≈ 115 dev-days; 3–4 parallel lanes ≈ 6–8 calendar weeks.

---

## PART D — Notes for finishing quickly with maximum quality and low cost

### D1. Model/agent routing (cheapest capable)
- **Haiku-class:** docs/manual text, changelog fragments, README, issue templates, mechanical file moves in X1 (with compile-after-each-step check), test boilerplate, preset XML entry from a sound designer's list, tooltips.
- **Sonnet-class (default for implementation):** all UI work, state/serialization (E3, U2, V5), capture/source (E1, U4), CI workflows, key detection, export, layers/characters (with listening checks), licensing plumbing.
- **Opus/top-tier only for:** time-preserving pitch shift (E2.2), threading/background render design (E1/F2/F3), RT-safety review of any `processBlock` diff, Ed25519 verification review (V6), bus-layout negotiation (V1/V3). Use it as a **reviewer** on small diffs rather than as the author where possible.
- Never hand an agent the whole repo: give **this spec section + the exact files + the A5 gate**. Tell it to grep, not read 1,600-line files whole (after X1, files are small, which itself saves tokens).

### D2. Token/time savers
- Warm build dir + `ccache` + Ninja: full Linux build ≈ 3 min cold; incremental typically seconds. Keep `/tmp/.../build` between runs; run only the targets touched (`ninja PreChorusTests` for logic work).
- Automated gate script `tools/gate.sh` (build, tests, pluginval, clap-validator) so every agent runs one command and pastes only the summary line.
- Golden + fuzz tests catch regressions without listening; reserve human listening for new DSP only (E2.2, F4, F5).
- Keep PRs small; ask reviewers for blocking issues only.

### D3. Things only the owner can supply (start now; they gate release)
1. Code-signing certs / Apple Developer account + GitHub secrets (E4).
2. Licensing/business decisions and approval of the Ed25519 dependency (V6, V7).
3. Sound design: 30–50 new factory presets with tags (U2); listening sign-off for new characters/layers (F4, F5).
4. A designer for UI polish and brand assets; demo audio/video and site copy (D1, D2).
5. Test machines or beta testers across the DAW matrix (E5) — recruit 5–10 producers; provide the bug template + diagnostics (E8).
6. Decide privacy stance for opt-in features (none currently collect data; keep "no network" promise — the manual link opening a URL is user-initiated).
7. Legal: trademark/copyright statements exist (`COPYRIGHT-TRADEMARK.md`); confirm third-party notices for JUCE (licence terms for commercial closed-source use: JUCE 8 requires a commercial or AGPL licence — **confirm which licence applies before selling**), clap-juce-extensions, any Ed25519 code; ship a `THIRD_PARTY_NOTICES.txt`.

### D4. Per-feature kickoff prompt template (for agents)
```
Implement <ID> from docs/FEATURE_SPECS.md. Read only: Part A, the <ID> section, and these files: <list>.
First do the [VERIFY] items and report findings in 5 lines. Then implement exactly the spec; do not
touch other files except one-line additions to shared lists. Add tests (Tests/Test_<feature>.cpp)
and docs fragment (docs/changelog.d/<id>.md). Run tools/gate.sh; report only the summary.
Do not change existing parameter IDs/choice order; new params use ParameterID{id,2}.
```

### D5. Release readiness checklist (before calling it 1.0)
Gate green on 3 OS · DAW matrix ≥ 90% pass, no crashers · signed/notarized installers · manual + quick-start · licence/third-party notices · SHA256SUMS · rollback plan (keep previous release) · support email/issue templates live · pluginval/auval/clap-validator logs attached to the release.
