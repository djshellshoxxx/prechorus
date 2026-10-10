# PreChorus — User Manual

PreChorus builds a cloud of up to 32 related voices from a vocal, instrument or hit, and makes that cloud grow more focused and
intimate until it lands on your drop. You hear the cloud *before* the event — that is the "pre-chorus" swell.

## 1. Quick start (2 minutes)

1. **Get a sound in.** Choose one of:
   - **OPTIONS → Load demo source → Ahh / Hey / Ooh chord** (no files needed), or
   - **LOAD** a WAV / AIFF / FLAC / MP3 / OGG file (or drag a file onto the window), or
   - **Live capture:** set the source mode to *Live Capture*, press **ARM**, then sing/play (or press the capture button for manual).
2. **Press Space (or PLAY)** to hear the swarm build and land on the hit (the amber part of the waveform).
3. **Pick a preset** from the PRESETS menu, or turn the big **MACRO** knob in the *Convergence & Macro* panel.
4. **Make it yours:** change **VOICES**, **CHARACTER**, **TIME SPREAD**, **TONE**, **SPACE**. Press **R** to randomize, **G** for a new seed.
5. **Use it in your song** — either:
   - **Play it live from your DAW:** put PreChorus on a track and send it a MIDI note at your drop. Turn on **Hit on note (PDC)** so the
     *hit* (not the start of the swell) lands exactly on the note; or
   - **Print it:** drag the **DRAG TO DAW** pad onto an audio track, or use **EXPORT WAV**.

## 2. Workflows

**Vocal build into the chorus.** Capture or load the last word of the verse, preset *Pop Vocal Double* or *Cinematic Choir Pad*, **Sync** on
with *1 Bar* or *2 Bars*, **Hit on note (PDC)** on, and send a MIDI note on the first beat of the chorus.

**EDM / dubstep riser.** Preset *EDM Riser Swarm* or *Dubstep Chaos Impact*, raise **PITCH SPREAD** and **STUTTER**, set **Post-Target Release**
to *Cut at Impact* for a clean drop.

**Play it like an instrument.** Turn on **KEYTRACK**: MIDI note C4 is the original pitch, other notes transpose the swarm. With **Hit on note
(PDC)** the impact stays on the beat for every note you play.

**A held cloud.** Turn on **FREEZE** (or set Post-Target Release to *Sustain Chorus*) for an ambient pad that never resolves.

**Compare ideas.** **A / A>B** buttons (shortcut **B**) hold two sound-design snapshots; **COPY** copies the current one across.

## 3. Sources and capture

| Source mode | What it does |
|---|---|
| Live Capture | Uses the active history slot (1–8) recorded from the plug-in's input. |
| Loaded Sample | Uses the loaded file or demo source. |
| Hybrid Layer | Mixes the loaded file and the active capture at equal level. |
| Slice Scatter | Splits the loaded file into 8 slices and spreads them across the voices. |

- **Capture mode:** *Threshold* (starts when the input passes THRESH, stops on silence), *1/16 … 2 Bars* (exact musical length, follows host
  tempo), or *Manual* (start/stop yourself).
- **History slots 1–8** keep recent takes; **LOCK** protects the active take from being overwritten.
- **Your audio is saved inside the project**, so captures and loaded files survive closing and moving the project (OPTIONS → *Store audio
  inside project*, on by default). Very large projects skip the biggest items; check OPTIONS → *Copy diagnostics* if something is missing.
- **Max source length** (OPTIONS): 12 / 30 / 60 / 120 s. Longer files are cropped, and the status bar tells you when that happens.

## 4. Controls

**Swarm Engine** — VOICES (1–32), DENSITY (how quickly voices join), VOICE AGE (darkens early voices), REVEAL (early voices play fragments),
HUMANIZE (micro timing/pitch variation), GRAIN MS (grain size for Granular Cloud), DIRECTION (forward / reverse / alternating / random).

**Character** (per-voice colour): Clean Digital, Analog Ensemble, Bucket-Brigade, Tape Choir, Dimension, String Ensemble, Granular Cloud, Lo-Fi Choral.

**Convergence & Macro** — MACRO scales every convergence control at once. TIME SPREAD / TIME CONV (how far back voices start / how tightly they
align), PITCH SPREAD / DETUNE / PITCH CONV, PAN SPREAD / WIDTH CONV, TONE CONV, FOCUS (laser-focus into the drop). FREEZE holds the current spread;
REV CONV runs the motion backwards (unified → scattered). SCALE restricts pitch scatter to a musical scale *relative to the source note*.

**Physics & 3D** — ATTRACT, TURBULENCE, OVERSHOOT, ORBIT, 3D DISTANCE (far and wet → close and dry).

**Tone & Colour** — LENGTH, SHAPE, TONE, BASS CUT, RESONANCE, TILT EQ, PRESENCE, AIR, SPACE, DRIVE, TRANSIENTS, FORMANT, MONO BASS.

**Mix, Duck & Stutter** — HIT DRY, SWARM WET, REPLACE (swarm replaces or blends with the dry hit), DUCKING (swarm ducks when your input plays),
THRESH, STUTTER (tempo-locked gate that accelerates 1/8 → 1/16 → 1/32 into the hit).

**Pitch, Volume & Trim** — PITCH sweep and range, START / END / TENSION volume curve, TRIM IN / TRIM OUT. Drag the white points on the waveform
to shape the volume curve. Double-click any knob to reset it.

**Timing** — **Sync** + length (1/2 Bar … 4 Bars) makes the swell follow host tempo. **Hit on note (PDC)** reports the swell length as latency so the
hit lands on the MIDI note. **Sequence** limits triggering to every note, beat 1 only, every 2 bars or every 4 bars.

## 5. Shortcuts

| Key | Action |
|---|---|
| Space | Preview |
| Esc | Stop preview / close help |
| R | Randomize sound design |
| G | New swarm seed |
| B | A/B switch |
| Ctrl/Cmd+Z | Undo (sound-design edits) |
| Ctrl/Cmd+Shift+Z or Ctrl/Cmd+Y | Redo |
| H or F1 | Help |

## 6. Presets

Factory presets and your own `.pcpreset` files appear in the PRESETS menu. **Save User Preset…** stores the sound design (never your audio).
User presets live in `Documents/Circuit Drift Labs/PreChorus/Presets`.

## 7. Troubleshooting

| Problem | Try this |
|---|---|
| No sound on Space | Source mode must match your source (a loaded file needs *Loaded Sample*). Check SWARM WET is up. |
| The hit is late in the DAW | Turn on **Hit on note (PDC)** and make sure your DAW has plug-in delay compensation enabled. |
| Controls feel slow to update the sound | The swarm re-renders in the background after each edit (about half a second for 32 voices). Fewer voices render faster. |
| A captured take is missing after reopening a project | Check *Store audio inside project* is on; projects saved by v0.0.1 did not store captures. |
| Windows SmartScreen warns about the installer | The beta is not code-signed yet. Choose *More info → Run anyway* if you downloaded it from the official release page. |
| Crash or odd behaviour | OPTIONS → **Copy diagnostics to clipboard**, then paste it into a bug report (no audio, no folder paths). |

## 8. Privacy

PreChorus makes no network connections and has no telemetry or licence checks. See `LICENSE` for terms.
