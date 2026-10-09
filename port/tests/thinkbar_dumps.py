# thinkbar_dumps.py [OUTDIR] : the dumps and the argument list for the bar
# of the computer's thinking (docs/HANDOFF.md, "The thinking bar"): for
# each call below, dumps.py's `think` (game_loop from its call of
# computer_progress, S066A:11C1, to behind bar_draw, S066A:1212, with the
# video memory and the DAC) and `progress` (computer_progress alone,
# S1C74:0354 to its RETF), then OUTDIR/thinkbar_test.args for
# load_test.sh.  OUTDIR is ../build/thinkbar by default; dumps.py never
# overwrites, so an OUTDIR with dumps is refused.  52 runs of HL.EXE, ten
# at a time.
#
# The calls (the script, how long it runs, the calls of computer_progress
# counted from the battle's beginning): the computer is the second side
# on the maps 8 and 0 and the first on map 24; the numbers are those at
# which a run with -log at S1C74:0519 showed the first value, a middle
# one and the highest of a round.
import os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor
CALLS = [('m8_d5r', 300, [1, 86, 188, 487]),
         ('m0_hq', 300, [1, 64, 329, 768]),
         ('m24_quiet', 400, [1, 31, 279, 611, 751])]
here = os.path.dirname(os.path.abspath(__file__))
out = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, '../../build/thinkbar'))
os.makedirs(out, exist_ok=True)

def one(job):
    keys, until, rt, n = job
    env = dict(os.environ, VRAM='1', DAC='1')
    env.pop('MAP8', None)
    if keys.startswith('m8_'):
        env['MAP8'] = '1'
    r = subprocess.run([sys.executable, os.path.join(here, 'dumps.py'), out, '%s_%d' % (keys, n),
                        os.path.join(here, 'keys', keys + '.keys'), str(until), rt, str(n)],
                       capture_output=True, text=True, env=env)
    return out, rt, r.stdout.strip().splitlines()[-1] if r.stdout.strip() else r.stderr.strip()

jobs = [(k, u, rt, n) for k, u, ns in CALLS for n in ns for rt in ('think', 'progress')]
args = []
with ThreadPoolExecutor(10) as ex:
    for d, rt, line in ex.map(one, jobs):
        print(line)
        m = re.search(r'in=(\S+) \(break .*out=(\S+) \(break .* AX=(\w+) DX=(\w+) SS:SP=(\w+):(\w+)', line)
        if not m:
            sys.exit('no dump: ' + line)
        args.append('%s %s:%s %s %s %s %s' % (rt, m.group(5), m.group(6), m.group(3), m.group(4),
                                              os.path.join(d, m.group(1)), os.path.join(d, m.group(2))))
open(os.path.join(out, 'thinkbar_test.args'), 'w').write('\n'.join(args) + '\n')
print(len(args), 'calls in', os.path.join(out, 'thinkbar_test.args'))
