#!/bin/sh

if [[ $# -lt 1 ]]; then
        ARCH="i386"
fi

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"

find $DIR -type f \
        -path "$DIR/arch/*"  ! -path "$DIR/arch/$ARCH*" -prune -o      \
        -path "$DIR/scripts*" -prune -o                                \
        -name "*.[chsS]" -print
