#!/usr/bin/env bash

target=""

while getopts ht:r optchar; do
        case "${optchar}" in
        h) echo "usage: $0 -t target [-r]" >&2
           exit 1;
           ;;
        t) target="${OPTARG}"
           ;;
        esac
done

if [ "$target" = "" ]; then
        echo "No target specified (-t)." >&2
        exit 1
fi
targets=$(find ${target} -type f -name *.[c,h])

scratch=$(mktemp)
for t in $targets; do
        echo "Converting includes for $t."
        rm $scratch
        while read -r ln; do
                if [ "$(grep '^#include "' <<< $ln)" != "" ]; then
                        echo FIXME >> $scratch
                else
                        echo $ln >> $scratch
                fi
        done < $t 
        cat $scratch
done
