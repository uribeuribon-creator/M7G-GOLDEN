#!/usr/bin/env bash
set -euo pipefail
scratch_source_root="$(cd "$(dirname "$0")/.." && pwd)"
scratch_build_root="${1:-$scratch_source_root}"
if [[ ! -f "$scratch_source_root/JUCE7/CMakeLists.txt" ]]; then
 git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE.git "$scratch_source_root/JUCE7"
fi
if [[ -d "$scratch_source_root/JUCE7/.git" ]]; then
 scratch_revision="$(git -C "$scratch_source_root/JUCE7" rev-parse HEAD)"
else
 scratch_revision="$(tr -d '\r\n' < "$scratch_source_root/JUCE7_REVISION.txt")"
fi
[[ "$scratch_revision" == '4f43011b96eb0636104cb3e433894cda98243626' ]]
cmake -S "$scratch_source_root/WordScratcher" -B "$scratch_build_root/build-scratcher-macos" -G Ninja -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build "$scratch_build_root/build-scratcher-macos" -j 2
mkdir -p "$HOME/Library/M7G"
if [[ ! -f "$HOME/Library/M7G/M7G.license" ]]; then cp "$scratch_source_root/Licensing/M7G.license" "$HOME/Library/M7G/M7G.license"; fi
ctest --test-dir "$scratch_build_root/build-scratcher-macos" --output-on-failure --verbose
