"""Exporta el .jucer con Projucer oficial 7.0.12 y valida VST3 normal/x64."""
from pathlib import Path
import argparse,hashlib,io,subprocess,urllib.request,zipfile,xml.etree.ElementTree as ET
ap=argparse.ArgumentParser();ap.add_argument('--exe',type=Path);args=ap.parse_args()
root=Path(__file__).resolve().parents[1]
exe=args.exe
if exe is None:
 cache=root.parent/'build-fresh-tools';cache.mkdir(exist_ok=True)
 exe=cache/'Projucer.exe'
 if not exe.exists():
  raw=urllib.request.urlopen('https://github.com/juce-framework/JUCE/releases/download/7.0.12/juce-7.0.12-windows.zip',timeout=120).read()
  with zipfile.ZipFile(io.BytesIO(raw)) as z:
   name=next(n for n in z.namelist() if n.endswith('/Projucer.exe'));exe.write_bytes(z.read(name))
  (cache/'DOWNLOAD.sha256').write_text(hashlib.sha256(raw).hexdigest()+'  juce-7.0.12-windows.zip\n')
subprocess.run([str(exe.resolve()),'--resave',str(root/'M7G_FreshVocalPro.jucer')],check=True)
defines=(root/'JuceLibraryCode/JucePluginDefines.h').read_text()
for marker in ['JucePlugin_Enable_ARA             0','0x46724231']:
 assert marker in defines,marker
project=ET.parse(root/'M7G_FreshVocalPro.jucer').getroot();assert project.get('cppLanguageStandard')=='17'
assert all(c.get('architecture')=='x64' for c in project.findall('.//CONFIGURATION'))
registered={p.get('file') for p in project.findall('.//FILE') if p.get('compile')=='1'}
assert registered=={p.relative_to(root).as_posix() for p in (root/'Source').rglob('*.cpp')}
assert 'juce_opengl' in {m.get('id') for m in project.findall('.//MODULE')}
assert (root/'Builds/VisualStudio2022/M7G Fresh Vocal Pro Beast.sln').exists()
print('PASS: official Projucer export, every source registered, standard VST3 without ARA, C++17, OpenGL and x64')
