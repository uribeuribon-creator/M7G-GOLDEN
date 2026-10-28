"""Create an offline-buildable source project, without Git metadata or secrets."""
from pathlib import Path
import argparse,hashlib,json,zipfile,subprocess
ap=argparse.ArgumentParser();ap.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[2]);ap.add_argument('--output',type=Path);args=ap.parse_args();root=args.repo.resolve();output=args.output or root/'WordScratcher/dist/WORD-SCRATCHER-PRO-300-Source.zip';output.parent.mkdir(parents=True,exist_ok=True)
expected_juce='4f43011b96eb0636104cb3e433894cda98243626'
if (root/'JUCE7/.git').exists():
 actual=subprocess.check_output(['git','-C',str(root/'JUCE7'),'rev-parse','HEAD'],text=True).strip()
else:
 actual=(root/'JUCE7_REVISION.txt').read_text().strip()
assert actual==expected_juce,'Unexpected JUCE source revision'
files={}
def add_tree(folder,prefix):
 for p in folder.rglob('*'):
  if not p.is_file():continue
  rel=p.relative_to(folder)
  if any(x in {'.git','dist','__pycache__','.DS_Store'} or x.startswith('build') for x in rel.parts):continue
  files[str(Path(prefix)/rel).replace('\\','/')]=p.read_bytes()
add_tree(root/'WordScratcher','WordScratcher');add_tree(root/'JUCE7','JUCE7');add_tree(root/'Assets/Fonts','Assets/Fonts')
for name in ['V11License.h','V11LicensePublic.h']:files['Source/'+name]=(root/'Source'/name).read_bytes()
files['Licensing/M7G.license']=(root/'Licensing/M7G.license').read_bytes()
files['JUCE7_REVISION.txt']=b'4f43011b96eb0636104cb3e433894cda98243626\n'
files['START-HERE.md']=b'''# WORD SCRATCHER PRO - 300\n\nIncludes the plugin project, Rubber Band source, pinned JUCE 7.0.12, fonts and shared distribution licence. No private signing key or credentials.\n\nWindows: Visual Studio 2022 C++ tools, CMake, Python 3 and NSIS; run `./WordScratcher/Build-Windows.ps1` in PowerShell.\nmacOS: Xcode C++ command tools, CMake and Ninja; run `bash WordScratcher/Build-macOS.sh`.\n\nSee WordScratcher/README.md, COPYING and NOTICES.md. Included source/binaries follow GPLv3-or-later dependency distribution; proprietary distribution requires appropriate commercial JUCE/Rubber Band licences.\n'''
try:revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
except Exception:revision='source-package'
manifest={'source_commit':revision,'juce':'7.0.12 / 4f43011b96eb0636104cb3e433894cda98243626','rubberband':'3.3.0 / 2be46b0dffb13273a67396c77bc9278736bb03d2','sha256':{name:hashlib.sha256(data).hexdigest() for name,data in sorted(files.items())}}
files['SOURCE-MANIFEST.json']=json.dumps(manifest,indent=2).encode()
with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=6)as z:
 for name,data in sorted(files.items()):z.writestr('WordScratcher300Project/'+name,data)
with zipfile.ZipFile(output)as z:
 for name,digest in manifest['sha256'].items():assert hashlib.sha256(z.read('WordScratcher300Project/'+name)).hexdigest()==digest
print('SOURCE:',output,len(files),'files',output.stat().st_size,'bytes; all file hashes verified')
