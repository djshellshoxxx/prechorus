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

> **Simple and Full view.** A new plug-in opens in **Simple view**: the seven controls that matter most (MACRO, VOICES, LENGTH, PAN SPREAD,
> TILT EQ, SPACE, SWARM WET). Click **FULL VIEW** (below the preset menu) for every control, and **SIMPLE VIEW** to go back. Switching never
> changes your settings.

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
| Slice Scatter | Splits the loaded file at its transients (8 equal slices if it has none) and spreads the slices across the voices. |

- **Capture mode:** *Threshold* (starts when the input passes THRESH, stops on silence), *1/16 … 2 Bars* (exact musical length, follows host
  tempo), or *Manual* (start/stop yourself).
- **History slots 1–8** keep recent takes; **LOCK** protects the active take from being overwritten.
- **Your audio is saved inside the project**, so captures and loaded files survive closing and moving the project (OPTIONS → *Store audio
  inside project*, on by default). Very large projects skip the biggest items; check OPTIONS → *Copy diagnostics* if something is missing.
- **Smart trim** (OPTIONS, on by default) removes silence around each new take; *Normalise takes* optionally brings them to -1 dBFS. The status bar tells you what was trimmed, **RETAKE** records again into the same slot, and the meter beside it shows your input level with the red capture-threshold line.
- **Max source length** (OPTIONS): 12 / 30 / 60 / 120 s. Longer files are cropped, and the status bar tells you when that happens.

## 4. Controls

**Swarm Engine** — VOICES (1–32), DENSITY (how quickly voices join), VOICE AGE (darkens early voices), REVEAL (early voices play fragments),
HUMANIZE (micro timing/pitch variation), GRAIN MS (grain size for Granular Cloud), DIRECTION (forward / reverse / alternating / random).

**Character** (per-voice colour): Clean Digital, Analog Ensemble, Bucket-Brigade, Tape Choir, Dimension, String Ensemble, Granular Cloud, Lo-Fi Choral, Vowel Choir (a-e-i-o-u formants), Stadium Crowd (diffuse, noisy) and Glitch Scatter (scrambled fragments).

**Convergence & Macro** — MACRO scales every convergence control at once. TIME SPREAD / TIME CONV (how far back voices start / how tightly they
align), PITCH SPREAD / DETUNE / PITCH CONV, PAN SPREAD / WIDTH CONV, TONE CONV, FOCUS (laser-focus into the drop). FREEZE holds the current spread;
REV CONV runs the motion backwards (unified → scattered). SCALE restricts pitch scatter to a musical scale *relative to the source note*.

**Physics & 3D** — ATTRACT, TURBULENCE, OVERSHOOT, ORBIT, 3D DISTANCE (far and wet → close and dry).

**Tone & Colour** — LENGTH, SHAPE, TONE, BASS CUT, RESONANCE, TILT EQ, PRESENCE, AIR, SPACE, DRIVE, TRANSIENTS, FORMANT (moves the vocal formants without changing the pitch), MONO BASS.

**Mix, Duck & Stutter** — HIT DRY, SWARM WET, REPLACE (swarm replaces or blends with the dry hit), DUCKING (swarm ducks when your input plays),
THRESH, STUTTER (tempo-locked gate that accelerates 1/8 → 1/16 → 1/32 into the hit).

**Build Stutter Grid** — choose Straight, Triplet, Dotted or Custom. In Custom, click the 16 steps (the right-most step is the one just before the hit; with the grid focused use the arrow keys and Return). SWING delays every second step.

**Impact & Riser Layers** — RISER (noise sweep into the hit), REV CYMBAL (reverse swell that ends on the hit), SUB DROP (62 to 30 Hz on the hit), IMPACT (short thump) and LAYER TONE (brightness). They are synthesised into the render, so they cost nothing while you play.

**Target Source & A/B Morph** — see the next two sections.

**Pitch, Volume & Trim** — PITCH sweep and range, START / END / TENSION volume curve, TRIM IN / TRIM OUT. Drag the white points on the waveform
to shape the volume curve; the curve runs from the start of the swell to the hit and then holds its END level. Double-click any knob to reset it.

**Timing** — **Sync** + length (1/2 Bar … 4 Bars) makes the swell follow host tempo. **Hit on note (PDC)** reports the swell length as latency so the
hit lands on the MIDI note. **Sequence** limits triggering to every note, beat 1 only, every 2 bars or every 4 bars.

**Target source** — *MIDI Note* (default) fires the swarm from MIDI. *Next Bar Line* starts the swarm early on its own so the impact lands exactly on every bar line (Sequence still applies); *Loop End* aims the impact at the end of the host loop; *Input Transient* starts the swarm when the input passes the TRANSIENT level. The automatic modes need the host to be playing and do not use PDC latency. **DETECT** chooses whether Ducking, Threshold capture and Input Transient listen to the main input or the sidechain.

**Sidechain and stems** — in your DAW's routing, enable PreChorus's *Sidechain* input and/or its *Swarm*, *Hit* and *Dry* outputs. The main output is always Swarm + Hit + Dry.

**MIDI learn** — right-click any knob → *MIDI learn…*, then move a CC, the mod wheel, aftertouch or the pitch wheel. The knob label shows the mapping; right-click → *Clear MIDI mapping* removes it (OPTIONS → *Clear all MIDI mappings*). Mappings are saved with the project. Sound controls re-render the swarm, so they update a few times per second; levels, MACRO and morph work best. Not every host forwards CC to effects.

**A/B morph** — store a sound as A, press **A>B**, switch to B and change it, then move **A/B MORPH**. Numbers blend; switches change at the halfway point. The swarm re-renders when you stop moving the knob.

**Performance variations (V1–V4)** — next to KEYTRACK. Click an empty slot to store the current sound, click a stored slot to recall it. Right-click for *Store*, *Clear* and *Switch variations with MIDI notes C1–D#1*; in that mode those notes pick variations instead of triggering the swarm. Switching re-renders in the background, so the previous sound keeps playing until the new one is ready.

**Randomize** — **R** / RANDOM is the classic randomize. The small button next to it opens the themes (Subtle, Wide, Dark, Wild, Tight, Cinematic; the same seed and theme always give the same sound) and the **lock groups** (Swarm, Convergence, Physics, Tone, Mix & Stutter, Character & Layers) that randomize leaves alone.

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
| Tab, then Up/Down (Shift = fine), PageUp/PageDown, Home/End | Operate the focused knob |
| Esc | Also cancels MIDI learn |

## 6. Presets and export

**Preset browser** — the PRESETS button opens a browser with 24 tagged factory presets and your own presets. Type to search names and tags, click tag buttons to filter (Vocal, EDM, Dubstep, Pop, FX, Subtle, Extreme, User, Favorites), click the star to favourite, sort by name or recent, and click a preset to load it. The **< >** buttons step through the list. **INIT** resets every sound-design control. Right-click a user preset to rename, delete or show it in the file manager. **SAVE…** stores the current sound; presets never contain your audio. **IMPORT…** accepts a `.pcpreset` or a `.pcbank`; **EXPORT BANK…** bundles all your user presets into a `.pcbank` you can share (banks are validated on import and never run code).
User presets live in `Documents/Circuit Drift Labs/PreChorus/Presets` (sub-folders are fine); favourites are remembered in `settings.xml` next to it.

**Export** — the small button next to EXPORT WAV sets the format (WAV 16-bit with dither, 24-bit, 32-bit float, FLAC 24-bit), sample rate, *Normalise to -1 dBFS* and *Trim silent tail*, and exports **stems** (`_Swarm` and `_Hit`). DRAG TO DAW uses the same settings.

**Swarm view** — the SWARM VIEW button swaps the waveform for a timeline of every voice (top = left pan, colour = pitch offset) with the convergence curve; hover a row for details.

**Accessibility** — Tab moves between controls; OPTIONS → *High contrast* and *Reduced motion* change the look; the status line is announced to screen readers.

## 7. Troubleshooting

| Problem | Try this |
|---|---|
| No sound on Space | Source mode must match your source (a loaded file needs *Loaded Sample*). Check SWARM WET is up. |
| The hit is late in the DAW | Turn on **Hit on note (PDC)** and make sure your DAW has plug-in delay compensation enabled. |
| Controls feel slow to update the sound | The swarm re-renders in the background after each edit (about half a second for 32 voices). Fewer voices render faster. |
| A captured take is missing after reopening a project | Check *Store audio inside project* is on; projects saved by v0.0.1 did not store captures. |
| Target source *Next Bar Line* does nothing | The host must be playing and report tempo and position; check TARGET CONFIDENCE. In loops use *Loop End* only when the host's loop is switched on. |
| Sidechain or stems missing | Enable the extra inputs/outputs in your DAW's routing for PreChorus; they are off by default. |
| Windows SmartScreen warns about the installer | The beta is not code-signed yet. Choose *More info → Run anyway* if you downloaded it from the official release page. |
| Crash or odd behaviour | OPTIONS → **Copy diagnostics to clipboard**, then paste it into a bug report (no audio, no folder paths). |

## 8. Privacy

PreChorus makes no network connections and has no telemetry or licence checks. See `LICENSE` for terms.
