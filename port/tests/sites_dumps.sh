#!/bin/sh
# Makes the dumps income_test.args and move_test_side1.args name (build/n2/o/, ignored): calls of
# S131D:1745 at entry and RETF from the key scripts in port/tests/keys/,
# map 8 (d4.2 and d5) and map 0 (s4: side 0 takes record 0 of sites_100).
# Each dump is two runs of the original; existing dumps are kept.
set -e
cd "$(dirname "$0")"
OUT=../../build/n2/o
mkdir -p $OUT
d() { [ -e $OUT/$2_income_in.ram ] || python3 dumps.py $OUT "$2" "$PWD/keys/$3" "$4" income "$5"; }
MAP8=1 d - i5a m8_d5.keys 320 1 &
MAP8=1 d - i5 m8_d5.keys 320 2 &
MAP8=1 d - i5c m8_d5.keys 320 3 &
MAP8=1 d - i5d m8_d5.keys 320 4 &
for k in 1 2 3 4 5 6 7; do d - s4i$k m0_s4.keys 230 $k & done
wait
# move_test_side1.args: the second side's arrivals in the s4 run
for k in 2 3 4 5; do
    [ -e $OUT/s4a${k}_move_arrive_in.ram ] || python3 dumps.py $OUT s4a$k "$PWD/keys/m0_s4.keys" 230 move_arrive $k &
done
wait
