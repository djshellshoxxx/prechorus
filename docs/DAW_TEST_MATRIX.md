# PreChorus DAW test matrix

Record one row per host, format and operating system for every release candidate. **Pass**, **Fail** (link the issue) or **n/a**.
Use the *DAW bug* issue template for failures and paste OPTIONS → *Copy diagnostics*.

| Host (version) | OS | Format | Instantiate/remove ×20 | Save/recall (embedded audio) | ≥ 5 automated params | PDC: hit on the grid | Loop / scrub recovery | MIDI trigger accuracy | Next-bar / loop-end target | 44.1/48/96 k + buffer 32–2048 | Offline bounce = realtime | Multiple instances | Sidechain + stems routing | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Reaper | Windows 11 | VST3 | | | | | | | | | | | | |
| Reaper | Windows 11 | CLAP | | | | | | | | | | | | |
| FL Studio | Windows 11 | VST3 | | | | | | | | | | | | |
| Ableton Live | Windows 11 | VST3 | | | | | | | | | | | | |
| Bitwig | Windows 11 | VST3 | | | | | | | | | | | | |
| Bitwig | Windows 11 | CLAP | | | | | | | | | | | | |
| Cubase / Studio One | Windows 11 | VST3 | | | | | | | | | | | | |
| Logic Pro | macOS | AU | | | | | | | | | | | | |
| Ableton Live | macOS | VST3 / AU | | | | | | | | | | | | |
| Reaper | macOS | VST3 / CLAP | | | | | | | | | | | | |
| Reaper | Linux | VST3 / CLAP | | | | | | | | | | | | |

Extra checks per host: 100 % CPU headroom test with 32 voices; closing the project while a render is in progress (no crash); window resize; MIDI learn (does the host forward CC to effects?); whether the sidechain input and the Swarm/Hit/Dry outputs can be enabled.

Status: **not yet run** (needs testers). Automated coverage: unit/behavioural tests, pluginval strictness 8 (VST3) and clap-validator on Linux; Windows and macOS build and test in CI.
