### New
- **Preset browser** with search, tags, favourites, recents, INIT, rename/delete and shareable `.pcbank` banks; 24 factory presets (14 new).
- **Smart capture**: automatic silence trimming (optional normalise), RETAKE, live input meter with the threshold line.
- **MIDI learn** on every knob (CC, aftertouch, pitch bend), saved with the project.
- **Target source**: next bar line, loop end or input transient, besides MIDI notes.
- **Optional sidechain input and Swarm / Hit / Dry stem outputs**; DETECT chooses the detector input; stem export.
- **Export options**: WAV 16/24/32-bit float, FLAC, 44.1/48/96 kHz, normalise, tail trim.
- **Themed randomize** (Subtle, Wide, Dark, Wild, Tight, Cinematic) with lock groups; **A/B morph**.
- **Impact & riser layers** (riser, reverse cymbal, sub drop, impact); **stutter grids** (triplet, dotted, custom 16-step, swing).
- Three new characters: **Vowel Choir**, **Stadium Crowd**, **Glitch Scatter**.
- **Performance variations** V1-V4 (store/recall, optional MIDI note switching).
- **Swarm view**, keyboard-operable knobs with focus rings, screen-reader names, high-contrast palette.

### Fixed
- The target hit overwrote the Sustain Chorus / Scatter Out tail instead of sounding with it.
- The default volume envelope (0 to 1 over the whole render) attenuated the hit by about 40 dB; it now runs to the hit.
- **Formant** was only a pitch shift; it now moves the spectral envelope and leaves the pitch alone.
- **Slice Scatter** used equal slices; it now splits at transients.
- Dry / Swarm Wet / Ducking changes could zipper under automation; they are now ramped across each block.
- Several knob value boxes were styled inconsistently.
