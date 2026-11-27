import json, re, zipfile
from pathlib import Path

root=Path(__file__).resolve().parents[1]
dist=root/'dist'
m=json.loads((dist/'manifest.json').read_text(encoding='utf-8'))
prefix='com.farissoze.voicemixerfix'
assert m['Author']=='FarisSoze'
assert m['Name']=='Voice Mixer Plus — FIX'
assert m['Software']['MinimumVersion']=='6.4'
actions={a['UUID'] for a in m['Actions']}
assert all(x.startswith(prefix+'.') for x in actions)
for profile in m['Profiles']:
    p=dist/(profile['Name']+'.streamDeckProfile')
    assert p.exists(),p
    with zipfile.ZipFile(p) as z:
        for entry in z.namelist():
            if not entry.endswith('.json'): continue
            text=z.read(entry).decode('utf-8')
            assert 'cz.danol.discordmixer' not in text
            data=json.loads(text)
            def check(v):
                if isinstance(v,dict):
                    if isinstance(v.get('UUID'),str) and v['UUID'].startswith(prefix): assert v['UUID'] in actions
                    for value in v.values(): check(value)
                elif isinstance(v,list):
                    for value in v: check(value)
            check(data)
for value in [m['Icon'],m['CategoryIcon']]+[a['Icon'] for a in m['Actions']]+[s['Image'] for a in m['Actions'] for s in a['States']]:
    assert (dist/(value+'.png')).exists(),value
assert not list(dist.rglob('discordOauth.json'))
assert not list(dist.rglob('clog.txt'))
print('Distribution identity, profiles, action references, icons and absence of credentials verified.')
