# dumps.py OUTDIR TAG KEYSFILE UNTIL ROUTINE N[/M] : two runs of HL.EXE (Germany; map 0, or
# map 8, RATIO, when MAP8 is set in the environment) with the side-0 key script
# (port/tests/keys/; a script mNN_ other than m0 and m8 is map NN, chosen
# by its code, with its pokes); -ram at ROUTINE's entry and at its RETF (the Nth entry and
# the Nth time its RETF is reached); appends one line to OUTDIR/dumps.log.
# Never overwrites: refuses when a dump file exists.  With VRAM set in the
# environment, also -vram (the video memory) beside each -ram, and with DAC
# set -dac (the DAC's 256 colours) as a .dac file; with NOPICS
# set, no_fight_pics (S37FF:0115) poked to 1 at the loop's beginning.
import os, re, struct, subprocess, sys
R={'move_arrive':('S10E1:0AF0','S10E1:0F70',['side','map_off','map_seg']),
   'move_target':('S10E1:0F82','S10E1:1444',['field','side','map_off','map_seg']),
   'pictures_load':('S02E6:2B87','S02E6:3237',['place_off','place_seg','work_off','work_seg','pal_off','pal_seg']),
   'screen_open':('S298E:000D','S298E:007B',['w','h','n','size_lo','size_hi','pages']),
   'page_flip':('S2974:0002','S2974:009A',[]),
   'pages_set':('S2A10:0006','S2A10:00C2',[]),
   'view_draw':('S0E56:0D44','S0E56:0F35',['field','side']),
   'marks_draw':('S0E56:0F36','S0E56:1012',['field','side']),
   'message_clear':('S10AF:000F','S10AF:009B',['text','side']),
   'show_message':('S10AF:000F','S10AF:0130',['text','side']),
   'field_draw':('S0E56:145A','S0E56:1725',['side','field','flag']),
   'marks_erase':('S0E56:1217','S0E56:1459',['field','side','mask']),
   'unit_panel':('S10AF:0131','S10E1:000E',['unit','side']),
   'fight_panel':('S131D:178D','S131D:1CD7',['rec_off','rec_seg','pictures']),
   'fight_pictures':('S2532:0008','S2532:01C1',['near0','standing','show','winter','kind0','kind1','ground0','ground1','count0','count1','after0','after1','none_off','none_seg','place_off','place_seg','work_off','work_seg']),
   'fight_frames':('S2532:0CAA','S2532:013C',[]),
   'fight_panel0':('S131D:178D','S131D:1F0C',['rec_off','rec_seg','pictures']),
   'fade_grey':('S0E56:21D8','S0E56:241E',['pal_off','pal_seg']),
   'fade_back':('S0E56:1F91','S0E56:21D7',['pal_off','pal_seg']),
   'bar':('S1571:01F2','S066A:1212',['x','y','colour','back','h','value','width']),
   'blast_draw':('S02E6:225C','S02E6:0BC2',['n','map_off','map_seg','view','side']),
   'pic_x':('HL.EXE+2957:0002','HL.EXE+2957:00A8',[]),
   'attacks_run':('S02E6:000F','S02E6:2064',['work_off','work_seg','place_off','place_seg','map_off','map_seg','pal_off','pal_seg','libs_off','libs_seg']),
   'game_begin':('S066A:0938','S066A:0EE6',[]),
   'game_start':('S066A:000D','S066A:0349',[]),
   'game_setup':('S066A:0417','S066A:0938',[]),
   'orders':('S02E6:0E9C','S02E6:19B8',[]),
   'lib_load':('S0C66:0009','S0C66:0246',['name_off','name_seg','place_off','place_seg','work_off','work_seg','rec_off','rec_seg','flag']),
   'file_load':('S2C45:0002','S2C45:011E',['to_off','to_seg','name_off','name_seg','work_off','work_seg']),
   'attack_order':('S0AE0:0008','S0AE0:02DB',['view_off','view_seg','map_off','map_seg','side']),
   'move_step':  ('S10E1:072C','S10E1:0AEF',[]),
   'income':('S131D:1745','S131D:178C',['side']),
   'build_list':('S131D:14D4','S131D:15FA',['points','side']),
   'repair':('S066A:3156','S066A:32A9',[]),
   'build_unit':('S066A:387A','S066A:3A42',[]),
   'move_out':('S066A:2F52','S066A:3146',[]),
   'side_input':('S0CB6:05F1','S0CB6:070E',['rec_off','rec_seg','view_off','view_seg']),
   'move_start':('S10E1:05D1','S10E1:0724',['view_off','view_seg','side']),
   'attack_unmark':('S0AE0:02E3','S0B15:000A',['view_off','view_seg','side']),
   'move_done':('S10E1:000F','S10E1:02EA',['view_off','view_seg','map_off','map_seg','side','flags']),
   'move_aim':('S10E1:02EB','S10E1:05C9',['view_off','view_seg','map_off','map_seg','side']),
   'field_menu':('S0CB6:0EAA','S0CB6:1841',['view_off','view_seg','side','bits']),
   'view_step':('S0CB6:0750','S0CB6:0CD2',['view_off','view_seg','rec_off','rec_seg','bits','map_off','map_seg','side'])}
out,tag,keys,until,rt,n=sys.argv[1:7]
n_in,n_out=(n.split('/')+[n])[:2]
here=os.path.dirname(os.path.abspath(__file__)); kit=os.path.join(here,'../../doskit/tools/run.py')
ent,ex,an=R[rt]
tag_k=os.path.basename(keys)[:-5]
if re.match(r'm\d\d_',tag_k) and not tag_k.startswith(('m0','m8_')):
    # map NN, chosen in the menu by its code as play_dumps.py does, and
    # the key script's pokes
    sys.path.insert(0,os.path.join(here,'../../tools')); import tpwmfiles
    game=os.environ.get('DOSKIT_GAME') or os.path.join(here,'../../game')
    rec=tpwmfiles.unpack(open(os.path.join(game,'CODES.DAT'),'rb').read())[int(tag_k[1:3])*10:][:5]
    pre=['-mouse','21','150,73,1','-mouse','21.2','150,73,0']
    for i,b in enumerate(rec): pre+=['-key',str(22+i*0.5),chr(b-0x11+ord('a'))]
    pre+=['-key','25','enter','-mouse','27','150,48,1','-mouse','27.2','150,48,0']
    pre+=subprocess.run(['python3',os.path.join(here,'play_pokes.py'),tag_k],capture_output=True,text=True).stdout.split()
    pre+=(['-poke','S066A:0EE9#1','S37FF:0115','01'] if os.environ.get('NOPICS') else [])+['-keys',os.path.abspath(keys),'-until',until]
else: pre=(['-mouse','21','150,73,1','-mouse','21.2','150,73,0','-key','22','r','-key','22.5','a','-key','23','t','-key','23.5','i','-key','24','o','-key','25','enter','-mouse','27','150,48,1','-mouse','27.2','150,48,0'] if os.environ.get('MAP8') else ['-mouse','22','150,48,1','-mouse','22.2','150,48,0','-mouse','31','230,80,1','-mouse','31.2','230,80,0'])+(['-poke','S066A:0EE9#1','S37FF:0115','01'] if os.environ.get('NOPICS') else [])+['-keys',os.path.abspath(keys),'-until',until]
def run(addr,f,n):
    p=os.path.join(out,f)
    if os.path.exists(p): sys.exit('exists: '+p)
    r=subprocess.run(['python3',kit]+pre+['-break','%s#%s'%(addr,n),'-ram',os.path.abspath(p)]+(['-vram',os.path.abspath(p[:-4]+'.vram')] if os.environ.get('VRAM') else [])+(['-dac',os.path.abspath(p[:-4]+'.dac')] if os.environ.get('DAC') else [])+['HL.EXE'],capture_output=True,text=True,cwd=out)
    t=re.search(r'stop (\S+) t=([\d.]+)',r.stdout+r.stderr)
    g=re.search(r'regs (.*)',r.stdout)
    return p,(t.group(1),t.group(2)) if t else ('?','?'),dict(re.findall(r'(\w\w)=([0-9A-F]+)',g.group(1))) if g else {}
pi,ti,gi=run(ent,'%s_%s_in.ram'%(tag,rt),n_in)
args=[]
if gi and ti[0]=='break':
    m=open(pi,'rb').read(); sp=int(gi['SS'],16)*16+int(gi['SP'],16)
    args=['%s=%04X'%(a,struct.unpack_from('<H',m,sp+4+2*i)[0]) for i,a in enumerate(an)]
po,to,go=run(ex,'%s_%s_out.ram'%(tag,rt),n_out)
line='%s %s#%s/%s in=%s (%s t=%s) args[%s] out=%s (%s t=%s) AX=%s DX=%s SS:SP=%s:%s BP=%s SI=%s'%(tag,rt,n_in,n_out,os.path.basename(pi),ti[0],ti[1],' '.join(args),os.path.basename(po),to[0],to[1],go.get('AX','?'),go.get('DX','?'),gi.get('SS','?'),gi.get('SP','?'),gi.get('BP','?'),gi.get('SI','?'))
open(os.path.join(out,'dumps.log'),'a').write(line+'\n'); print(line)
