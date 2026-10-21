# Developer Guide

- Windows: run `build.ps1` in an elevated PowerShell. It builds VST3, CLAP and Standalone and installs the plug-ins (README "Build (Windows)").
- macOS and Linux: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`, then `cmake --build build --config Release`.
- Tests: `Tests/PreChorusTests.cpp` (see `CMakeLists.txt`). Not run in this documentation pass.
- Releases: `.github/workflows/release.yml` (tags `v*`) builds the installer (`installer/PreChorus.iss`) and archives.
- Shared baseline: `docs/standards/CDL_PLUGIN_BASELINE.md`.
