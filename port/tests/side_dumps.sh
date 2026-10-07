#!/bin/sh
# Makes the dumps side_test.args names (build/n2/o/, ignored): build_list
# (S131D:14D4) and the repairing (S066A:3156 to 32A9) from the map-0 runs
# s4 and hq (port/tests/keys/; m0_hq.keys is made by make_hq.py with the
# arguments below).  Each dump is two runs of the original; existing dumps
# are kept.
set -e
cd "$(dirname "$0")"
OUT=../../build/n2/o
mkdir -p $OUT
python3 keys/make_hq.py a84 m102 a125 m145 a165 m185 a205 m225 a245 | cmp -s - keys/m0_hq.keys \
    || { echo "keys/m0_hq.keys is not what make_hq.py makes" >&2; exit 1; }
d() { [ -e $OUT/$1_$4_in.ram ] || python3 dumps.py $OUT "$1" "$PWD/keys/$2" "$3" "$4" "$5"; }
d s4b1 m0_s4.keys 230 build_list 1 &
d hqb1 m0_hq.keys 100 build_list 1 &
d hqb2 m0_hq.keys 265 build_list 2 &
d hqr1 m0_hq.keys 100 repair 1 &
wait
