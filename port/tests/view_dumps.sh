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
# move_done (S10E1:000F): the first side's in k4, the computer's in s2c,
# d5r and site
d mdk4 m0_k4.keys 60 move_done 1 &
d mds2c_5 m0_s2c.keys 108 move_done 5 &
MAP8=1 d mdd5_40 m8_d5r.keys 405 move_done 40 &
MAP8=1 d mdd5_88 m8_d5r.keys 405 move_done 88 &
MAP8=1 d mdsite_101 m8_site.keys 405 move_done 101 &
wait
# move_aim (S10E1:02EB): the first side's in k4, calls 1, 30, 60 and 73 of
# d5r (the first side's and the computer's)
d mak4 m0_k4.keys 60 move_aim 1 &
for n in 1 30 60 73; do MAP8=1 d mad5_$n m8_d5r.keys 300 move_aim $n & done
wait

# field_menu (S0CB6:0EAA): the entry, a held button and the releases
# (the calls before S0CB6:106C was reached, found with run.py -log)
for n in 2 3 10 16; do d fmk4_$n m0_k4.keys 60 field_menu $n & done
for n in 30 43 45 71 85 98; do d fms4_$n m0_s4.keys 130 field_menu $n & done
wait
for n in 32 51 54 70 99; do d fmhq_$n m0_hq.keys 130 field_menu $n & done
for n in 30 44 58 160; do MAP8=1 d fmd5_$n m8_d5r.keys 130 field_menu $n & done
wait
for n in 16 30 44 146; do MAP8=1 d fmsite_$n m8_site.keys 125 field_menu $n & done
wait
# view_step (S0CB6:0750): calls found with run.py -log on its branches
# (S0CB6:080F, 0837, 0909, 0B24, 0C90): a press in step 0, the aim (16h),
# the move's start (3), 19h after it, a call without a press, the wait
# after a refused aim (17h) and the attack's order (13h)
for n in 32 100 204 231 232; do d vsk4_$n m0_k4.keys 60 view_step $n & done
for n in 233 260; do d vss4_$n m0_s4.keys 130 view_step $n & done
wait
for n in 990 1572 1600 2045; do MAP8=1 d vsd5_$n m8_d5r.keys 130 view_step $n & done
for n in 1395 1868; do MAP8=1 d vssite_$n m8_site.keys 125 view_step $n & done
wait
# refused aims of d5r (move_aim 15 and 16, from view_step 1571 and 1585,
# found with run.py -log S0CB6:07C1 and S10E1:02EB)
for n in 1571 1585; do MAP8=1 d vsd5_$n m8_d5r.keys 130 view_step $n & done
for n in 15 16; do MAP8=1 d mad5r_$n m8_d5r.keys 100 move_aim $n & done
wait
