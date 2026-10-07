#!/bin/sh
# Makes the dumps view_test.args names (build/n2/o/, ignored): calls of the
# routines view_step calls, from the key scripts in port/tests/keys/ (the
# computer's turns call them too).  Each dump is two runs of the original;
# existing dumps are kept.
set -e
cd "$(dirname "$0")"
OUT=../../build/n2/o
mkdir -p $OUT
d() { [ -e $OUT/$1_$4_in.ram ] || python3 dumps.py $OUT "$1" "$PWD/keys/$2" "$3" "$4" "$5"; }
# move_start (S10E1:05D1)
d msk4 m0_k4.keys 80 move_start 1 &
d mss2 m0_s2.keys 80 move_start 1 &
d mss4_2 m0_s4.keys 260 move_start 2 &
d mss4_20 m0_s4.keys 260 move_start 20 &
MAP8=1 d msd5_10 m8_d5r.keys 260 move_start 10 &
MAP8=1 d msd5_63 m8_d5r.keys 260 move_start 63 &
wait
# attack_unmark (S0AE0:02E3) and attack_order (S0AE0:0008), d5r's calls 1,
# 5 and 9 (the first side's, then the computer's)
for n in 1 5 9; do
    MAP8=1 d au$n m8_d5r.keys 330 attack_unmark $n &
    MAP8=1 d ao$n m8_d5r.keys 330 attack_order $n &
done
wait

