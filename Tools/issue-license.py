"""Issue an offline perpetual licence. Keep the private PEM outside the repo."""
import argparse,hashlib,json,secrets
from pathlib import Path
from cryptography.hazmat.primitives import hashes,serialization
from cryptography.hazmat.primitives.asymmetric import padding,rsa
p=argparse.ArgumentParser();p.add_argument('--private',required=True);p.add_argument('--owner',required=True);p.add_argument('--output',required=True);p.add_argument('--public-header');a=p.parse_args()
keyfile=Path(a.private)
if keyfile.exists():key=serialization.load_pem_private_key(keyfile.read_bytes(),password=None)
else:
 key=rsa.generate_private_key(public_exponent=65537,key_size=2048)
 keyfile.write_bytes(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()));keyfile.chmod(0o600)
serial='M7G-GOLDEN-'+secrets.token_hex(10).upper()
owner=a.owner.strip()
if any(c in owner for c in '\n\r|'):raise SystemExit('Invalid owner')
payload='M7G-LICENSE-1|'+serial+'|'+owner+'|ALL|PERPETUAL'
sig=key.sign(payload.encode(),padding.PKCS1v15(),hashes.SHA256()).hex()
Path(a.output).write_text(json.dumps(dict(format=1,key=serial,owner=owner,scope='ALL',term='PERPETUAL',signature=sig),indent=2)+'\n')
if a.public_header:
 n=key.public_key().public_numbers().n
 Path(a.public_header).write_text('#pragma once\ninline constexpr const char *m7gLicensePublicKey = "10001,'+format(n,'x')+'";\n')
print('Issued perpetual global licence:',serial)
