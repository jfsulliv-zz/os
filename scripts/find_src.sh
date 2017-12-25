#!/bin/sh

if [ "$CONFIG_ARCH" = "" ]; then
        echo "error: Set CONFIG_ARCH"
        exit 1
fi

ARCH=$CONFIG_ARCH

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"

if [ "$ARCH" = "i386" ] || [ "$ARCH" = "x86_64" ]; then
        extra_path="$DIR/arch/x86_common*"
else
        extra_path=""
fi

find $DIR/arch/$ARCH $extra_path $DIR/lib $DIR/include $DIR/kernel $DIR/drivers \
        -type f -name "*.[chsS]" -print
