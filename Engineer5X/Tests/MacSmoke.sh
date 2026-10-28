#!/usr/bin/env bash
set -euo pipefail
[[ "${GITHUB_ACTIONS:-}" == true ]] || { echo 'Disposable CI runner test only.'; exit 1; }
engineer_name='Makkine Engineer 5X - AI Vocal Tuner Evolution'
engineer_au="$PWD/build-engineer-macos/Engineer5X_artefacts/Release/AU/$engineer_name.component"
engineer_vst="$PWD/build-engineer-macos/Engineer5X_artefacts/Release/VST3/$engineer_name.vst3"
mkdir -p "$HOME/Library/Audio/Plug-Ins/Components" "$HOME/Library/Audio/Plug-Ins/VST3" Engineer5X/dist
[[ ! -e "$HOME/Library/Audio/Plug-Ins/Components/$engineer_name.component" ]]
[[ ! -e "$HOME/Library/Audio/Plug-Ins/VST3/$engineer_name.vst3" ]]
for engineer_bundle in "$engineer_au" "$engineer_vst"; do
 /usr/bin/codesign --force --deep --sign - "$engineer_bundle"
 /usr/bin/codesign --verify --deep --strict "$engineer_bundle"
 /usr/bin/lipo -verify_arch arm64 x86_64 "$engineer_bundle/Contents/MacOS/$engineer_name"
done
cp -R "$engineer_au" "$HOME/Library/Audio/Plug-Ins/Components/"
cp -R "$engineer_vst" "$HOME/Library/Audio/Plug-Ins/VST3/"
/usr/bin/auval -v aufx E5x1 M7G7 | tee Engineer5X/dist/Makkine-Engineer-5X-auval.txt
printf 'Native test architecture: %s\nUniversal slices: arm64 + x86_64\n' "$(uname -m)" > Engineer5X/dist/Makkine-Engineer-5X-macOS-architectures.txt
rm -rf "$HOME/Library/Audio/Plug-Ins/Components/$engineer_name.component" "$HOME/Library/Audio/Plug-Ins/VST3/$engineer_name.vst3"
echo 'PASS: universal AU/VST3 signatures, architectures, native auval and isolated install/removal'
