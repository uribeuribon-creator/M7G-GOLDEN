#!/usr/bin/env bash
set -euo pipefail
engineer_source_root="$(cd "$(dirname "$0")/.." && pwd)"
engineer_build_root="${1:-$engineer_source_root}"
if [[ ! -f "$engineer_source_root/JUCE7/CMakeLists.txt" ]]; then
 git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE.git "$engineer_source_root/JUCE7"
fi
if [[ -d "$engineer_source_root/JUCE7/.git" ]]; then
 engineer_revision="$(git -C "$engineer_source_root/JUCE7" rev-parse HEAD)"
else
 engineer_revision="$(tr -d '\r\n' < "$engineer_source_root/JUCE7_REVISION.txt")"
fi
[[ "$engineer_revision" == '4f43011b96eb0636104cb3e433894cda98243626' ]]
cmake -S "$engineer_source_root/Engineer5X" -B "$engineer_build_root/build-engineer-macos" -G Ninja -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build "$engineer_build_root/build-engineer-macos" -j 2
mkdir -p "$HOME/Library/M7G"
if [[ ! -f "$HOME/Library/M7G/M7G.license" ]]; then cp "$engineer_source_root/Licensing/M7G.license" "$HOME/Library/M7G/M7G.license"; fi
ctest --test-dir "$engineer_build_root/build-engineer-macos" --output-on-failure --verbose
