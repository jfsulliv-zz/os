#!/usr/bin/env bash

csrcs=$(find . -name *.[ch])

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
