"""Mirror the case-insensitive SDK header lookup expected by Windows compilers."""
from pathlib import Path
import json
import re

root = Path('/workspace/tooling/msvc-sdk')
def tree(directory):
    children = []
    for path in directory.iterdir():
        if path.is_dir():
            children.append({'type': 'directory', 'name': path.name, 'contents': tree(path)})
        elif path.is_file() and path.name == path.resolve().name:
            children.append({'type': 'file', 'name': path.name, 'external-contents': str(path.resolve())})
    return children

overlay = Path('/workspace/tooling/msvc-sdk-overlay.json')
if not overlay.exists():
    roots = [{'type': 'directory', 'name': str(path), 'contents': tree(path)}
             for path in [root/'crt/include', *(root/'sdk/include').iterdir()] if path.is_dir()]
    overlay.write_text(json.dumps({'version': 0, 'case-sensitive': False, 'roots': roots}))
index = {}
for directory in [root/'crt/lib/x86_64', root/'sdk/lib/um/x86_64', root/'sdk/lib/ucrt/x86_64']:
    for path in directory.glob('*.lib'):
        index[path.name.lower()] = path
names = {'DbgHelp.lib', 'Dwrite.lib', 'D2d1.lib', 'DComp.lib'}
for path in Path('/workspace/M7G-GOLDEN/JUCE/modules').rglob('*'):
    if path.suffix in {'.h', '.cpp'}:
        names.update(re.findall(r'comment\s*\(\s*lib\s*,\s*"([^"]+)"', path.read_text(errors='replace')))
for name in names:
    path = index.get(name.lower())
    if path and not (path.parent/name).exists():
        (path.parent/name).symlink_to(path.name)
