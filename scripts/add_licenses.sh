#!/usr/bin/env bash

csrcs=$(find arch/$ARCH $extra_path lib include kernel drivers \
        -type f -name "*.[ch]" -print)

for f in $csrcs; do
        if ! grep -q "Copyright (c)" $f; then
                t=$(mktemp /tmp/XXXXX)
                echo "/*" > $t
                cat licence.txt >> $t
                echo -e "*/\n" >> $t
                cat $f >> $t
                cp $t $f
        fi
done
