# PreChorus — Project Progress

**Current version:** 1.4.0
**Last updated:** 2026-09-16
**Build:** VST3 + CLAP + AU + Standalone, JUCE 8.0.4.
CMake configures clean on Windows (MSVC 2022 x64, CLAP 1.2.7) and all seven project sources
compile with no errors or warnings. The full link has **not** been completed on this dev box:
it has 3.88 GB of RAM shared with a running BOINC client, and the JUCE module translation
units exhaust physical memory before linking. CI (`.github/workflows/build.yml`) builds
Windows and Linux on runners that have the headroom.

Tracks the project against `include.md` (the checklist every VST in the range must satisfy),
`theme.md` (the shared visual identity spec), and the colony/voxbox work layered on top.

---

## 1. `include.md` checklist — all done

| # | Requirement | Where |
|---|---|---|
| 1 | Help section: every feature, workflow, GUI tour, version number | `HelpPage::manualText()` in `Source/Pages.cpp` — `?` or File ▸ Manual |
| 2 | Custom icon unique to the project | `Assets/icon_1024.png` / `icon_128.png` |
| 3 | Preset bank with descriptive names | 10 factory presets + user `.pcpreset` files |
| 4 | Reset button — everything back to defaults | `resetAllToDefaults()` |
| 5 | Save / Save As / Open / Options dropdown | `FILE` menu in the header |
| 6 | Options page: tooltips, MIDI and audio card | `OptionsPage` (gear icon) |
| 7 | Export audio to WAV | `EXPORT WAV`, File ▸ Export, drag-to-DAW |
| 8 | Right-click any control: MIDI map, reset, set value | `showParamContextMenu()` in `Source/Controls.cpp` |
| 9 | Hover tooltips | `Source/Tooltips.h`, 400 ms, switchable |
| 10 | Random button — resets first after the first press | `randomizePreChorus()` |
| 11 | Progress file | this file |

## 2. `theme.md` — fully applied

Palette, typography (Inter / JetBrains Mono with fallbacks, 11px tracked uppercase labels),
knob and fader shapes, button and toggle states with the 100 ms press flash, the smooth-gradient
meter with 1.5 s peak hold and 20 dB/s fall, the 8px grid and 16px window padding, 1px section
separators, the 32px header with gear / preset / A-B, 80 ms ease-out on value changes, hover
brighten, Shift/Ctrl drag modifiers, the corner notch and the version stamp.

Plus the ANIMATIONS addition: the output LED in the header, dark grey at −∞, brightening to
white toward 0 dB, latching red while over.

**Accent:** the spec's stock `#E8532A` / `#4FB6C4` rather than a re-tint.

---

## 3. The Colony (`Source/Colony.h/.cpp`)

The constellation is a simulation. Every orb is one audio element, so what happens to the orbs
happens to the sound. It lives on the message thread; the processor's 30 Hz timer steps it and
re-renders the swell whenever the element count moves.

**Click gestures** — left ×4 splits three orbs; right ×3 breeds a red orb; middle ×2 makes an
orb pregnant; clicking a yellow orb lights a 40 s–4 min fuse that fires, then repeats 3–9 min
later. Progress pips are drawn along the bottom of the field.

**Dragging** — slowly mutates the sound fast and throws off fractals; fast destroys other orbs
and strips oscillators. The held orb's halo reddens with speed.

**Buttons** — Add Gravity / Release Gravity / Add Enzyme / Radiate / Add Water, with a live
census and the score.

**Overdose (more than 5 presses in 10 s)** — water goes watered-down and washed-out; Radiate
time-stretches and granulates (0.5×–3× slower, wearing off over 30 min); Enzyme does the
opposite and compresses time.

**Colour rules** — green hunts red; green+red detonates (half the colony and half the added
oscillations); green brushing a plain orb twice reverses time; blue crashing into pink kills the
blue one.

**Reversal** — every 20th destruction and every second green-on-plain contact. It slows, stops,
hangs, then winds back up to the same speed backwards. Audio and animation both.

**Breeding** — clutch is 10 (1 in 10), 30 (1 in 4), 20 (1 in 5), else 2–88. Exactly 20 halves
the pace, exactly 30 doubles it, exactly 10 rolls between 3× slower and 4× faster, all glided in
over ~6 s. Eggs hatch at random into coloured orbs that whistle at random pitches, live 5–30
minutes, and leave a 40 Hz–10 kHz oscillation when they die. Each colour has its own pop on
arrival and the inverse of that pop on death.

**Autonomous events** — spontaneous coloured orb (5 min, 1 in 4), visitor that stays 4–5 min and
whizzes out (4 min, 1 in 4; every 5th begs for food and shelter), babble (4 min, 1 in 30),
asteroid shattering the colony into 100 orbs at 1×–100× (20 min, 1 in 100), slow resize between
0.1× and 4× with a rising tone (40 min, 1 in 40).

**Score** — points per sound change; docked for fast destruction; fractioned for very fast
damage. It means nothing.

## 4. The Voxbox (`Source/Talkbox.h/.cpp`)

Formant synthesis: a glottal pulse through three two-pole resonant bands, with a fresh throat,
pitch contour and pace every time. One invocation in five drops a full octave. Free-form modes
(plea, urgent babble, random outburst) plus fixed phrase templates spoken in a new voice each
time: *food and shelter*, *are you having fun / I'm hungry*, *help me tie my shoes*, Spanish
*ayuda / socorro por favor*, French *au secours / aidez-moi*, *ranch and mayo in my hair*, and
three daft one-liners.

`WhistleBank` handles the short sounds — whistles, death oscillations, water sparkles, falling
tones, colour pops and their inverses, rising resize tones, and exit whizzes — through a
single-producer queue, allocation-free on the audio thread.

## 5. Audio plumbing added for all this

- Granular time-stretch playback (two half-overlapped Hann grains) so the swell drags without
  dropping an octave; plain interpolated read when not stretching.
- Reversible playback: read position runs either way, and a voice started while reversed begins
  at the impact.
- Output reverb for the washout and the babble tail; a 330 ms delay line for urgent outbursts.
- Washout low-pass, water high-pass thinning, colony oscillator bank and gamma pitch offsets
  folded into `render()`.

## 6. Switches

Options has **Distress Call** (the idle 10-minute caller) and **Colony Life** (everything the
colony does unprompted). Both default to on. With Colony Life off, the buttons and the click
gestures still work and nothing fires by itself.

---

## 7. Formats — CLAP and Linux

**CLAP** comes from [clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions),
fetched by CMake and pointed at the existing `PreChorus` target. It is a wrapper, not a port:
the CLAP and the VST3 compile from the same sources and share the same parameter tree and
`getStateInformation` payload, so a preset saved in one opens in the other. CLAP 1.2.7.
`clap_juce_extensions_plugin()` is declared with `CLAP_FEATURES audio-effect chorus stereo`
and the CLAP id `com.sheldondavidson.prechorus`. Turn it off with `-DPRECHORUS_BUILD_CLAP=OFF`.

**Linux** needs no source changes: JUCE covers the platform, and `Source/` contains no
`windows.h`, `_WIN32`, `__declspec` or `#pragma comment` — checked by grep, not yet confirmed by
a completed Linux compile. What it needed was build plumbing:

- `FORMATS` is now assembled per platform, since AU only exists on Apple and asking for it
  elsewhere just produces CMake noise.
- `build-linux.sh` checks for the required dev packages, installs what is missing, then
  configures and builds VST3 + CLAP + Standalone into `build-linux/` with Ninja.
- `.github/workflows/build.yml` builds Linux and Windows on every push and uploads all three
  formats as artefacts.

`JUCE_WEB_BROWSER=0` and `JUCE_USE_CURL=0` keep webkit and curl out of the Linux dependency
list, which is most of what usually makes a JUCE Linux build awkward.

## 8. Verified

- Every roll threshold checked by simulation: 1-in-100/40/30/20/10/5/4/3 gates all land on their
  nominal rates; clutch sizes 10/30/20 at 10/25/20 % plus the small extra share they get from
  the 2–88 fallback range; sparkle density 4–22; sparkle delay 1 ms–3 min; death tones 40 Hz–10 kHz.
- `juce::Random`'s default constructor seeds from object address, high-res ticks and wall clock;
  `Colony`, `WhistleBank` and the processor's fuse generator are additionally re-seeded from the
  system generator so two instances never march in step.
- Standalone launched and the colony UI confirmed rendering.

## 9. Next up

- [ ] Audition pass with real audio for the granular stretch and the reverb tail levels.
- [ ] User preset browser in the header dropdown alongside the factory list.
- [ ] Per-CC mapping ranges rather than the full parameter sweep.
- [ ] Undo / redo.
- [ ] macOS build pass and `auval`.

## 10. Kept in mind

- This build may become the **pro** version with a **free** version to follow; the editor's
  feature groups map cleanly onto a reduced free layout.
- Instruments in the range may get effect versions and vice versa; source acquisition is already
  separate from the swarm renderer.
