#!/bin/sh
# Makes the dumps side_test.args names (build/n2/o/, ignored): build_list
# (S131D:14D4), the repairing (S066A:3156 to 32A9) and the building of a
# unit (S066A:387A to 3A42) from the map-0 runs s4 and hq and the map-8
# run site (port/tests/keys/; m0_hq.keys is made by make_hq.py with the
# arguments below).  Each dump is two runs of the original; existing dumps
# are kept.
set -e
M8='e45 g55 s64 l67 d69 E70 f95 f100 f105 f110 E116 f150 f155 f160 f165 f170 u182 L183 s185 s189 s192 l195 d197 E198 f205 f210 f215 f220 f225 f230 f235 E245 f250 f255 f260 f265 f270 f275 f280 f285 f290 f295 f300 u312 L313 s315 R320 r322 l325 d327 E328 f335 f340 f345 f350 f355 f360 f365 f370 f375 u388 L389 s391 R396 o398'
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
# map 8: m8_site.keys, made by make_m8.py with the arguments below
python3 keys/make_m8.py $M8 | cmp -s - keys/m8_site.keys \
    || { echo "keys/m8_site.keys is not what make_m8.py makes" >&2; exit 1; }
MAP8=1 d m8b2 m8_site.keys 196 build_list 2 &
MAP8=1 d m8b3 m8_site.keys 196 build_list 3 &
MAP8=1 d m8u1 m8_site.keys 196 build_unit 1 &
MAP8=1 d m8r1 m8_site.keys 326 repair 1 &
for k in 1 2 20 41; do MAP8=1 d m8o$k m8_site.keys 402 move_out $k & done
wait
