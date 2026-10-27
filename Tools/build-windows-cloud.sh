#!/usr/bin/env bash
set -euo pipefail
cd /workspace/M7G-GOLDEN
TOOLS=/workspace/tooling
export APT_CONFIG="$TOOLS/apt/apt.conf"
if [ ! -x "$TOOLS/windows-tools/usr/lib/llvm-19/bin/clang-cl" ]; then
 apt-get --download-only -y -o Debug::NoLocking=1 --no-install-recommends install clang-19 lld-19 nsis
 python3 - <<'PY'
from pathlib import Path
import subprocess
root=Path('/workspace/tooling/windows-tools');root.mkdir(exist_ok=True)
for p in Path('/workspace/tooling/apt/cache/archives').glob('*.deb'):
 if any(x in p.name for x in ['clang','llvm','lld','nsis','libobjc','lib32','libc6-i386','libpfm']):
  subprocess.run(['dpkg-deb','-x',str(p),str(root)],check=True)
PY
fi
XWIN="$TOOLS/xwin-download/xwin-0.10.0-x86_64-unknown-linux-musl/xwin"
if [ ! -x "$XWIN" ]; then
 mkdir -p "$TOOLS/xwin-download"
 curl -fLsS https://github.com/Jake-Shadle/xwin/releases/download/0.10.0/xwin-0.10.0-x86_64-unknown-linux-musl.tar.gz -o "$TOOLS/xwin-download/xwin.tar.gz"
 python3 - <<'PY'
from pathlib import Path
import hashlib,tarfile
r=Path('/workspace/tooling/xwin-download')
assert hashlib.sha256((r/'xwin.tar.gz').read_bytes()).hexdigest()=='d870eb4b2f390878af6da1ccd3cf321d22fcb72720984853b4be732ae597fc88'
with tarfile.open(r/'xwin.tar.gz') as t:t.extractall(r,filter='data')
PY
fi
if [ ! -f "$TOOLS/msvc-sdk/sdk/lib/um/x86_64/kernel32.lib" ]; then
 "$XWIN" --accept-license --cache-dir "$TOOLS/xwin-cache" splat --output "$TOOLS/msvc-sdk"
fi
python3 Tools/msvc-overlay.py
CMAKE="$TOOLS/cmake/data/bin/cmake"
"$CMAKE" -S . -B build-windows -G Ninja -DCMAKE_MAKE_PROGRAM="$TOOLS/bin/ninja" -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/windows-clang.cmake" -DCMAKE_BUILD_TYPE=Release -DM7G_HOST_JUCEAIDE="$PWD/build/JUCE/tools/extras/Build/juceaide/juceaide_artefacts/Debug/juceaide" -DBUILD_TESTING=ON
"$CMAKE" --build build-windows -j4
python3 - <<'PY'
from pathlib import Path
import struct
p=Path('build-windows/M7G_Golden_Voice_AI_artefacts/Release/VST3/M7G ALIEN VOICE V11 - ALIEN INTELLIGENCE.vst3/Contents/x86_64-win/M7G ALIEN VOICE V11 - ALIEN INTELLIGENCE.vst3')
d=p.read_bytes();offset=struct.unpack_from('<I',d,0x3c)[0]
assert d[:2]==b'MZ' and d[offset:offset+4]==b'PE\0\0' and struct.unpack_from('<H',d,offset+4)[0]==0x8664
PY
mkdir -p dist
NSISDIR="$TOOLS/windows-tools/usr/share/nsis" "$TOOLS/windows-tools/usr/bin/makensis" -V3 "-DARTIFACT_DIR=$PWD/build-windows/M7G_Golden_Voice_AI_artefacts/Release" "-DOUTPUT_FILE=$PWD/dist/M7G-ALIEN-VOICE-V11-ALIEN-INTELLIGENCE-Windows-x64.exe" Installer/Windows.nsi
