"""Run a Windows check in the isolated Wine prefix and preserve its real exit code."""
from pathlib import Path
import os,re,subprocess,sys
if len(sys.argv)<2:raise SystemExit('Usage: run-wine-test.py executable [arguments]')
env=os.environ.copy()
env.update(PROOT_NO_SECCOMP='1',DISPLAY=':98',XDG_CACHE_HOME='/workspace/tooling/cache',WINEPREFIX='/workspace/tooling/wine-prefix-final',WINEDEBUG='-all',WINEDLLPATH='/workspace/tooling/wine/usr/lib/x86_64-linux-gnu/wine',LD_LIBRARY_PATH='/workspace/tooling/proot/usr/lib/x86_64-linux-gnu:/workspace/tooling/wine/usr/lib/x86_64-linux-gnu')
args=[]
for argument in sys.argv[1:]:
 if argument.startswith('/workspace/') or argument.startswith('/tmp/'):argument='Z:'+argument.replace('/','\\')
 args.append('"'+argument+'"' if ' ' in argument else argument)
command=' '.join(args)+' & echo __ALIEN_TEST_EXIT__!errorlevel!'
run=subprocess.run(['/workspace/tooling/proot/usr/bin/proot','-b','/workspace/tooling/wine/usr/share/wine:/usr/share/wine','/workspace/tooling/wine/usr/lib/wine/wine64','cmd.exe','/d','/v:on','/c',command],env=env,capture_output=True,timeout=180)
output=run.stdout.decode('utf-8',errors='replace')+run.stderr.decode('utf-8',errors='replace')
print(output,end='')
status=re.findall(r'__ALIEN_TEST_EXIT__(\d+)',output)
if not status:raise SystemExit('No Windows exit-status marker; check was not verified')
raise SystemExit(int(status[-1]))
