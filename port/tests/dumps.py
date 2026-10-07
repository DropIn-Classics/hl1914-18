# dumps.py OUTDIR TAG KEYSFILE UNTIL ROUTINE N[/M] : two runs of HL.EXE (Germany; map 0, or
# map 8, RATIO, when MAP8 is set in the environment) with the side-0 key script
# (port/tests/keys/); -ram at ROUTINE's entry and at its RETF (the Nth entry and
# the Nth time its RETF is reached); appends one line to OUTDIR/dumps.log.
# Never overwrites: refuses when a dump file exists.
import os, re, struct, subprocess, sys
R={'move_arrive':('S10E1:0AF0','S10E1:0F70',['side','map_off','map_seg']),
   'move_target':('S10E1:0F82','S10E1:1444',['field','side','map_off','map_seg']),
   'orders':('S02E6:0E9C','S02E6:19B8',[]),
   'file_load':('S2C45:0002','S2C45:011E',['to_off','to_seg','name_off','name_seg','work_off','work_seg']),
   'attack_order':('S0AE0:0008','S0AE0:02DB',['view_off','view_seg','map_off','map_seg','side']),
   'move_step':  ('S10E1:072C','S10E1:0AEF',[]),
   'income':('S131D:1745','S131D:178C',['side']),
   'build_list':('S131D:14D4','S131D:15FA',['points','side']),
   'repair':('S066A:3156','S066A:32A9',[]),
   'build_unit':('S066A:387A','S066A:3A42',[])}
out,tag,keys,until,rt,n=sys.argv[1:7]
n_in,n_out=(n.split('/')+[n])[:2]
here=os.path.dirname(os.path.abspath(__file__)); kit=os.path.join(here,'../../doskit/tools/run.py')
ent,ex,an=R[rt]
pre=(['-mouse','21','150,73,1','-mouse','21.2','150,73,0','-key','22','r','-key','22.5','a','-key','23','t','-key','23.5','i','-key','24','o','-key','25','enter','-mouse','27','150,48,1','-mouse','27.2','150,48,0'] if os.environ.get('MAP8') else ['-mouse','22','150,48,1','-mouse','22.2','150,48,0','-mouse','31','230,80,1','-mouse','31.2','230,80,0'])+['-keys',os.path.abspath(keys),'-until',until]
def run(addr,f,n):
    p=os.path.join(out,f)
    if os.path.exists(p): sys.exit('exists: '+p)
    r=subprocess.run(['python3',kit]+pre+['-break','%s#%s'%(addr,n),'-ram',os.path.abspath(p),'HL.EXE'],capture_output=True,text=True,cwd=out)
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
