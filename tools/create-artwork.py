from PIL import Image,ImageDraw,ImageFont
from pathlib import Path
import json, math
repo=Path(__file__).resolve().parents[1]
root=repo
icons=repo/'dist/icons'; icons.mkdir(exist_ok=True)
for p in icons.iterdir():
    if p.is_file() and p.suffix=='.png': p.unlink()
out=repo/'artwork/marketplace'; out.mkdir(parents=True,exist_ok=True)
ink='#f4f7f6'; mint='#78e6ba'; muted='#acbdb7'; bg='#111a18'; panel='#1c2b26'
def glyph(kind,size,color=ink,brand=False):
    scale=4; im=Image.new('RGBA',(size*scale,size*scale)); d=ImageDraw.Draw(im)
    def pt(p): return tuple(int(v*size*scale) for v in p)
    def line(coords,w=.055,fill=color): d.line([pt(p) for p in coords],fill=fill,width=max(1,int(w*size*scale)),joint='curve')
    def ellipse(box,w=.05,fill=None): d.ellipse(pt(box),fill=fill,outline=None if fill else color,width=max(1,int(w*size*scale)))
    if brand:
        d.rounded_rectangle((0,0,size*scale,size*scale),radius=int(size*.22*scale),fill=bg)
    if kind in ['plugin','mixer']:
        for x,y in [(.28,.38),(.5,.65),(.72,.3)]:
            line([(x,.2),(x,.8)],fill=mint if brand else color)
            d.rounded_rectangle(pt((x-.09,y-.055,x+.09,y+.055)),radius=int(size*.025*scale),fill=mint if brand else color)
    elif kind in ['back','previous','next']:
        if kind=='back': line([(.75,.7),(.75,.4),(.25,.4)]); line([(.4,.22),(.22,.4),(.4,.58)])
        elif kind=='next': line([(.37,.22),(.65,.5),(.37,.78)])
        else: line([(.65,.22),(.37,.5),(.65,.78)])
    elif kind in ['plus','minus']:
        line([(.24,.5),(.76,.5)])
        if kind=='plus': line([(.5,.24),(.5,.76)])
    elif kind=='user':
        ellipse((.36,.18,.64,.46)); d.arc(pt((.22,.48,.78,.94)),180,360,fill=color,width=int(size*.055*scale))
    elif kind.startswith('microphone'):
        d.rounded_rectangle(pt((.39,.15,.61,.59)),radius=int(size*.1*scale),outline=color,width=int(size*.05*scale))
        d.arc(pt((.28,.27,.72,.72)),0,180,fill=color,width=int(size*.05*scale));line([(.5,.72),(.5,.85)]);line([(.37,.85),(.63,.85)])
        if kind.endswith('muted'): line([(.18,.18),(.82,.82)],.06)
    elif kind.startswith('speaker'):
        d.polygon([pt(p) for p in [(.19,.39),(.36,.39),(.56,.2),(.56,.8),(.36,.61),(.19,.61)]],outline=color,width=int(size*.05*scale))
        if kind.endswith('muted'):
            line([(.68,.4),(.85,.6)]);line([(.85,.4),(.68,.6)])
        else: d.arc(pt((.38,.19,.85,.81)),-60,60,fill=color,width=int(size*.05*scale))
    elif kind=='speaking_deco':
        d.rounded_rectangle(pt((.035,.035,.965,.965)),radius=int(size*.12*scale),outline=mint,width=int(size*.04*scale))
    return im.resize((size,size),Image.Resampling.LANCZOS)
kinds=['mixer','back','user','plus','minus','next','previous','microphone-muted','microphone','speaker-muted','speaker','noicon','speaking_deco']
for kind in kinds:
    for suffix,n in [('',72),('@2x',144)]: glyph(kind,n).save(icons/(kind+suffix+'.png'))
    if kind not in ['noicon','speaking_deco']:
        for suffix,n in [('',20),('@2x',40)]: glyph(kind,n).save(icons/('action-'+kind+suffix+'.png'))
for suffix,n in [('',256),('@2x',512)]: glyph('plugin',n,brand=True).save(icons/('plugin'+suffix+'.png'))
for suffix,n in [('',28),('@2x',56)]: glyph('mixer',n).save(icons/('category'+suffix+'.png'))
mfile=repo/'dist/manifest.json'; m=json.loads(mfile.read_text(encoding='utf-8'));m['CategoryIcon']='icons/category'
for action in m['Actions']: action['Icon']='icons/action-'+action['Icon'].split('/')[-1].removeprefix('action-')
mfile.write_text(json.dumps(m,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
glyph('plugin',512,brand=True).save(out/'app-icon.png')
def font(n,bold=False):return ImageFont.truetype('C:/Windows/Fonts/segoeuib.ttf' if bold else 'C:/Windows/Fonts/segoeui.ttf',n)
def canvas(title,sub):
    im=Image.new('RGB',(1920,960),bg); d=ImageDraw.Draw(im)
    im.paste(glyph('plugin',90,brand=True),(110,72),glyph('plugin',90,brand=True))
    d.text((224,91),'FarisSoze  /  Voice Mixer Plus — FIX',font=font(28),fill=muted)
    d.text((110,205),title,font=font(86,True),fill=ink)
    d.text((114,325),sub,font=font(32),fill=muted)
    d.line((110,864,1810,864),fill='#34463e',width=2)
    d.text((110,891),'Windows • Stream Deck • Stream Deck+',font=font(24),fill=muted)
    d.text((1110,891),'Community fork based on Danol’s open-source mixer',font=font(22),fill=muted)
    return im,d
im,d=canvas('Discord voice. Under control.','Per-person volume, dial controls, and the connection fixes you need.')
for x,title,sub,kind in [(110,'Individual volume','Adjust each voice-chat member.','user'),(690,'Live connection status','Know when you are connected.','mixer'),(1270,'One-click reconnect','Keep Stream Deck running.','back')]:
    d.rounded_rectangle((x,440,x+540,785),24,fill=panel)
    ic=glyph(kind,92,mint); im.paste(ic,(x+35,475),ic)
    d.text((x+35,600),title,font=font(35,True),fill=ink);d.text((x+35,660),sub,font=font(25),fill=muted)
im.save(out/'thumbnail.png')
im,d=canvas('Give every voice its place.','Turn a dial to adjust volume. Press to mute. Keep the conversation balanced.')
for x,n,percent in [(150,'Alex',75),(725,'Sam',100),(1300,'Taylor',125)]:
    d.rounded_rectangle((x,450,x+470,790),24,fill=panel)
    d.ellipse((x+40,490,x+150,600),fill='#344b41');d.text((x+77,506),n[0],font=font(46,True),fill=mint)
    d.text((x+180,495),n,font=font(40,True),fill=ink);d.text((x+180,552),str(percent)+'%',font=font(32),fill=mint)
    d.rounded_rectangle((x+40,665,x+430,680),7,fill='#344b41');d.rounded_rectangle((x+40,665,x+40+int(390*percent/200),680),7,fill=mint)
    d.text((x+40,724),'VOLUME  /  MUTE',font=font(25),fill=muted)
d.text((125,816),'Illustrative member names and levels.',font=font(20),fill=muted)
im.save(out/'gallery-01-volume.png')
im,d=canvas('Reconnect. Keep your setup.','See connection status and a useful error reason. Retry without restarting Stream Deck.')
for x,label,caption in [(140,'Connecting','A clear pending state.'),(730,'Connected','Your mixer is ready.'),(1320,'Reconnect','Retry from the inspector.')]:
    d.rounded_rectangle((x,460,x+460,755),24,fill=panel)
    d.ellipse((x+40,505,x+60,525),fill=mint)
    d.text((x+40,560),label,font=font(46,True),fill=ink)
    d.text((x+40,640),caption,font=font(25),fill=muted)
d.text((145,800),'Includes an Arabic / English “Reconnect” button.',font=font(25),fill=mint)
im.save(out/'gallery-02-reconnect.png')
im,d=canvas('Your Discord. Your controls.','A local Discord integration. Requires your own Discord application credentials.')
rows=[('01','Create your Discord application','Use the same account as your desktop Discord client.'),('02','Add your Client ID and secret','Follow the included setup guide, then authorize Discord.'),('03','Open your mixer','Choose a profile and adjust people’s volume from your deck.')]
for y,(num,title,sub) in zip([445,575,705],rows):
    d.text((130,y),num,font=font(43,True),fill=mint)
    d.text((250,y),title,font=font(37,True),fill=ink);d.text((250,y+52),sub,font=font(27),fill=muted)
im.save(out/'gallery-03-setup.png')
print('Original icons and marketplace images generated:',out)
