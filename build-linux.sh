#!/usr/bin/env bash
# PreChorus - Linux build (VST3 + CLAP + Standalone).
# Works on a plain Ubuntu/Debian box and under WSL.  Run:  ./build-linux.sh
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$root"

# JUCE_WEB_BROWSER=0 and JUCE_USE_CURL=0 are set in CMakeLists, so webkit and
# curl are deliberately not in this list.
DEPS=(
  build-essential cmake ninja-build pkg-config git
  libasound2-dev libjack-jackd2-dev
  libfreetype-dev libfontconfig1-dev
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev
  libxinerama-dev libxrandr-dev libxrender-dev
  libglu1-mesa-dev mesa-common-dev
)

missing=()
for p in "${DEPS[@]}"; do
  dpkg -s "$p" >/dev/null 2>&1 || missing+=("$p")
done
if [ ${#missing[@]} -gt 0 ]; then
  echo "Installing: ${missing[*]}"
  sudo apt-get update -qq
  sudo apt-get install -y "${missing[@]}"
fi

BUILD_DIR="${BUILD_DIR:-build-linux}"
JOBS="${JOBS:-$(nproc)}"

echo "Configuring in $BUILD_DIR ..."
cmake -S "$root" -B "$root/$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release

echo "Building with $JOBS job(s) ..."
cmake --build "$root/$BUILD_DIR" --config Release -j "$JOBS"

echo
echo "DONE.  Artefacts:"
find "$root/$BUILD_DIR/PreChorus_artefacts" \( -name '*.vst3' -o -name '*.clap' -o -name 'PreChorus' \) -maxdepth 3 2>/dev/null | sed 's/^/  /'
echo
echo "Install locally with:"
echo "  mkdir -p ~/.vst3 ~/.clap"
echo "  cp -r $BUILD_DIR/PreChorus_artefacts/Release/VST3/PreChorus.vst3 ~/.vst3/"
echo "  cp    $BUILD_DIR/PreChorus_artefacts/Release/CLAP/PreChorus.clap ~/.clap/"
