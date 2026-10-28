"""Fuentes completas para compilar con CMake o Visual Studio/Projucer; sin claves privadas."""
from pathlib import Path
import hashlib,json,subprocess,zipfile
root=Path(__file__).resolve().parents[2];product=root/'M7G_FreshVocalPro';out=product/'dist/M7G-Fresh-Vocal-Pro-Beast-Source.zip';out.parent.mkdir(exist_ok=True)
pins=json.loads((product/'ThirdParty/REVISIONS.json').read_text())
for path,expected in [(root/'JUCE7',pins['JUCE']['commit']),(product/'ThirdParty/ARA_SDK',pins['ARA_SDK']['commit']),(product/'ThirdParty/ARA_SDK/ARA_API',pins['ARA_SDK']['ARA_API']),(product/'ThirdParty/ARA_SDK/ARA_Library',pins['ARA_SDK']['ARA_Library'])]:
 if (path/'.git').exists():assert subprocess.check_output(['git','-C',str(path),'rev-parse','HEAD'],text=True).strip()==expected
files={}
def add_tree(folder,prefix):
 for p in folder.rglob('*'):
  if not p.is_file():continue
  rel=p.relative_to(folder)
  if any(x in {'.git','.vs','dist','__pycache__','.DS_Store','html_docs','ARA_Examples'} or x.startswith('build-') for x in rel.parts):continue
  if 'Builds' in rel.parts and any(x in {'Debug','Release','x64','ipch'} for x in rel.parts):continue
  if p.suffix in {'.exe','.dll','.pdb','.obj','.lib','.ilk','.tlog','.lastbuildstate'}:continue
  files[str(Path(prefix)/rel).replace('\\','/')]=p.read_bytes()
add_tree(product,'M7G_FreshVocalPro');add_tree(product/'ThirdParty/ARA_SDK','M7G_FreshVocalPro/ThirdParty/ARA_SDK');add_tree(root/'JUCE7','JUCE7')
for name in ['V11License.h','V11LicensePublic.h']:files['Source/'+name]=(root/'Source'/name).read_bytes()
files['Licensing/M7G.license']=(root/'Licensing/M7G.license').read_bytes();files['JUCE7_REVISION.txt']=(pins['JUCE']['commit']+'\n').encode()
files['START-HERE.md']=b'''# M7G Fresh Vocal Pro Beast\n\nComplete JUCE 7 / ARA / fonts / VST3 sources, including exported Visual Studio 2022 projects.\n\nWindows x64: run M7G_FreshVocalPro/Build-Windows.ps1 (VS2022 C++ tools, CMake, Python and NSIS), or open M7G_FreshVocalPro/Builds/VisualStudio2022/M7G Fresh Vocal Pro Beast.sln and select Release / x64.\n\nThe .jucer can be opened/resaved with Projucer 7.0.12. Existing generated VS projects build offline; Projucer is only downloaded if regeneration is requested. See README.md and NOTICES.md.\n'''
revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip();manifest={'source_commit':revision,'dependencies':pins,'sha256':{name:hashlib.sha256(data).hexdigest() for name,data in sorted(files.items())}}
files['SOURCE-MANIFEST.json']=json.dumps(manifest,indent=2).encode()
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for name,data in sorted(files.items()):z.writestr('FreshVocalProject/'+name,data)
with zipfile.ZipFile(out) as z:
 for name,digest in manifest['sha256'].items():assert hashlib.sha256(z.read('FreshVocalProject/'+name)).hexdigest()==digest
 assert 'FreshVocalProject/M7G_FreshVocalPro/Builds/VisualStudio2022/M7G Fresh Vocal Pro Beast.sln' in z.namelist()
 assert 'FreshVocalProject/M7G_FreshVocalPro/ThirdParty/ARA_SDK/ARA_Library/Debug/ARADebug.h' in z.namelist()
 assert 'FreshVocalProject/M7G_FreshVocalPro/ThirdParty/ARA_SDK/ARA_Library/Debug/ARADebug.cpp' in z.namelist()
 assert not any(n.endswith('.pem') or '/.git/' in n for n in z.namelist())
print('PASS: source package',len(files),'files;',out.stat().st_size,'bytes;',out)
