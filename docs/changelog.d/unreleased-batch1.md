### New
- **Source audio is saved inside your project.** Live captures (all 8 history slots) and loaded files now survive closing and reopening a project, and projects can be moved between computers. Toggle in OPTIONS > Store audio inside project (default on, 24-bit FLAC, size-capped).
- **Undo / redo** for sound-design edits: Ctrl/Cmd+Z, Ctrl/Cmd+Shift+Z (or Y), also in OPTIONS.
- **Demo sources** (Ahh, Hey, Ooh chord): OPTIONS > Load demo source. No files needed.
- **Max source length** 12 / 30 / 60 / 120 s (default 30 s). Cropped files now say so.
- **Copy diagnostics** (OPTIONS) for bug reports: no audio, no folder paths.

### Fixed
- Live captures were lost when a project was reloaded.
- Editing sound controls no longer freezes the plug-in (or the host) for about half a second per change: the swarm now renders in the background.
- Keytrack: the hit now lands on the reported latency for every transposed note (faster notes start later, slower notes start further into the swarm).
