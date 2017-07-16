#!/usr/bin/env bash

target=""
recursive=""

while getopts ht:r optchar; do
        case "${optchar}" in
        h) echo "usage: $0 -t target [-r]" >&2
           exit 1;
           ;;
        t) target="${OPTARG}"
           ;;
        r) recursive="1"
           ;;
        esac
done

if [ "$target" = "" ]; then
        echo "No target specified (-t)." >&2
        exit 1
fi

targets=${target}
if [ "$recursive" = "1" ]; then
        targets=$(find ${target} -type d)
fi

MODULE_HEADER='''MODULES += $(dir)

sp := $(sp).x
dirstack_$(sp) := $(d)
d  := $(dir)
'''

MODULE_FOOTER='''d := $(dirstack_$(sp))
sp := $(basename $(sp))'''

for t in $targets; do
        module="${t}/module.mk"
        echo "Generating ${module}"
        srcs=$(find $t -maxdepth 1 -type f -name '*.c')
        subdirs=$(find $t -maxdepth 1 -type d | grep -v "^$t\$" | sed -e "s:$t/::g")
        echo "${MODULE_HEADER}" > ${module}
        echo "" >> ${module}
        for f in ${subdirs}; do
                echo "dir := \$(d)/${f}" >> ${module}
                echo "include \$(dir)/module.mk" >> ${module}
        done 
        echo "" >> ${module}
        if [ "$srcs" != "" ]; then
                src_bases=$(basename -a $(tr '\n' ' ' <<< ${srcs}))
                all_srcs=""
                for f in ${src_bases}; do
                        all_srcs+="\$(d)/${f} " 
                done
                fold <<< "SRCS_\$(d) := ${all_srcs}" | sed 's:.*$:& \\:g' >> ${module} 
        fi
        echo "" >> ${module}
        echo "${MODULE_FOOTER}" >> ${module}
done
