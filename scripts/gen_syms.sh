#!/usr/bin/env bash

inf=$1
t=$(mktemp /tmp/XXXXX)
t2=$(mktemp /tmp/XXXXX)

while read l; do
        nm $l | grep -E " [tT] " >> $t
done < $inf

# Strip out symbols with offset 0
sort $t | grep -v "00000000" > $t2

scripts/format_syms.py < $t2
