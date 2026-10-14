# Security Policy

PreChorus makes no network connections and collects no telemetry. The only external data it reads are audio files, presets
(`.pcpreset`) and DAW project state you open yourself; all are size- and schema-validated before use.

If you find a security problem (for example a crash or memory-safety bug triggered by a crafted audio file, preset or project),
please report it privately through GitHub's "Report a vulnerability" button on the Security tab rather than opening a public issue.
Include the format, host, OS and, if possible, a sample file. You will get a reply as soon as the maintainer can look at it.
