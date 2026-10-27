#!/usr/bin/env bash
set -euo pipefail
cd /workspace/M7G-GOLDEN
TOOLS=/workspace/tooling
mkdir -p "$TOOLS/apt/lists/partial" "$TOOLS/apt/cache/archives/partial" "$TOOLS/apt/empty" "$TOOLS/sysroot"
python3 -m pip install --target "$TOOLS" --upgrade cmake==4.4.4 ninja==1.13.2
cat > "$TOOLS/apt/sources.list" <<'SOURCES'
deb https://deb.debian.org/debian trixie main
SOURCES
cat > "$TOOLS/apt/apt.conf" <<'CONFIG'
Dir::Etc::sourcelist "/workspace/tooling/apt/sources.list";
Dir::Etc::sourceparts "/workspace/tooling/apt/empty";
Dir::Etc::parts "/workspace/tooling/apt/empty";
Dir::State::lists "/workspace/tooling/apt/lists";
Dir::Cache "/workspace/tooling/apt/cache";
APT::Get::List-Cleanup "0";
CONFIG
export APT_CONFIG="$TOOLS/apt/apt.conf"
apt-get update
apt-get --download-only -y -o Debug::NoLocking=1 install libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev libfreetype-dev libfontconfig1-dev libgl1-mesa-dev
python3 - <<'PYTHON'
from pathlib import Path
import subprocess
root=Path('/workspace/tooling/sysroot')
for package in Path('/workspace/tooling/apt/cache/archives').glob('*.deb'):
    subprocess.run(['dpkg-deb','-x',str(package),str(root)],check=True)
for path in root.rglob('*'):
    if path.is_symlink() and not path.exists():
        system=Path('/usr/lib/x86_64-linux-gnu')/path.readlink().name
        if system.exists():
            path.unlink()
            path.symlink_to(system.resolve())
PYTHON
if [ ! -f JUCE/CMakeLists.txt ]; then
 git clone --depth 1 --branch 8.0.6 https://github.com/juce-framework/JUCE.git JUCE
fi
[ "$(git -C JUCE rev-parse HEAD)" = 51a8a6d7aeae7326956d747737ccf1575e61e209 ]
export PKG_CONFIG_SYSROOT_DIR="$TOOLS/sysroot"
export PKG_CONFIG_PATH="$TOOLS/sysroot/usr/lib/x86_64-linux-gnu/pkgconfig:$TOOLS/sysroot/usr/share/pkgconfig"
CMAKE="$TOOLS/cmake/data/bin/cmake"
"$CMAKE" -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$TOOLS/sysroot/usr" -DCMAKE_CXX_FLAGS="-I$TOOLS/sysroot/usr/include" -DCMAKE_C_FLAGS="-I$TOOLS/sysroot/usr/include"
"$CMAKE" --build build --config Release -j 4
"$TOOLS/cmake/data/bin/ctest" --test-dir build -C Release --output-on-failure
