#!/usr/bin/env bash
set -euo pipefail
[[ "${GITHUB_ACTIONS:-}" == true ]] || { echo 'This fixture test is for a disposable CI runner.'; exit 1; }
scratch_root="$PWD"
scratch_au="$scratch_root/build-scratcher-macos/WordScratcher_artefacts/Release/AU/WORD SCRATCHER PRO - 300.component"
scratch_vst="$scratch_root/build-scratcher-macos/WordScratcher_artefacts/Release/VST3/WORD SCRATCHER PRO - 300.vst3"
mkdir -p "$HOME/Library/Audio/Plug-Ins/Components" "$HOME/Library/Audio/Plug-Ins/VST3"
[[ ! -e "$HOME/Library/Audio/Plug-Ins/Components/WORD SCRATCHER PRO - 300.component" ]]
[[ ! -e "$HOME/Library/Audio/Plug-Ins/VST3/WORD SCRATCHER PRO - 300.vst3" ]]
for scratch_bundle in "$scratch_au" "$scratch_vst"; do
 /usr/bin/codesign --force --deep --sign - "$scratch_bundle"
 /usr/bin/codesign --verify --deep --strict "$scratch_bundle"
 /usr/bin/lipo -verify_arch arm64 x86_64 "$scratch_bundle/Contents/MacOS/WORD SCRATCHER PRO - 300"
done
cp -R "$scratch_au" "$HOME/Library/Audio/Plug-Ins/Components/"
cp -R "$scratch_vst" "$HOME/Library/Audio/Plug-Ins/VST3/"
/usr/bin/auval -v aufx Ws30 M7G7 | tee "$scratch_root/WordScratcher/dist/Word-Scratcher-300-auval.txt"
if /usr/bin/arch -x86_64 /usr/bin/true 2>/dev/null; then
 /usr/bin/arch -x86_64 "$scratch_root/build-scratcher-macos/ScratchDSP" --core | tee "$scratch_root/WordScratcher/dist/Word-Scratcher-300-Intel-compatibility.txt"
 /usr/bin/arch -x86_64 /usr/bin/auval -v aufx Ws30 M7G7 | tee "$scratch_root/WordScratcher/dist/Word-Scratcher-300-auval-Intel.txt"
else
 echo "Universal Intel slice compiled and verified; Rosetta execution unavailable." > "$scratch_root/WordScratcher/dist/Word-Scratcher-300-Intel-compatibility.txt"
fi
rm -rf "$HOME/Library/Audio/Plug-Ins/Components/WORD SCRATCHER PRO - 300.component" "$HOME/Library/Audio/Plug-Ins/VST3/WORD SCRATCHER PRO - 300.vst3"
echo 'PASS: macOS universal AU/VST3 integrity, per-user install, native auval and isolated removal'
