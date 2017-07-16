#!/usr/bin/env bash

csrcs=$(find $DIR/arch/$ARCH $extra_path $DIR/lib $DIR/include \
        $DIR/kernel $DIR/drivers \
        -type f -name "*.[chsS]" -print)

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
