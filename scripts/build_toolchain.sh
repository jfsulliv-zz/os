#!/usr/bin/env bash

set -e

target=""
prefix=""
toolchain_dir=""
ld=""
cc=""
iconv=""
while getopts ht:p:l:c:d:i: optchar; do
        case "${optchar}" in
        h) echo "usage: $0 -t target -p prefix -l ld -c libgcc -i libiconv" >&2
           exit 1;
           ;;
        t) target="${OPTARG}"
           ;;
        p) prefix="${OPTARG}"
           ;;
        l) ld="${OPTARG}"
           ;;
        c) cc="${OPTARG}"
           ;;
        i) iconv="${OPTARG}"
           ;;
        d) toolchain_dir="${OPTARG}"
           ;;
        esac
done

if [ "$target" = "" ]; then
        echo "No target specified (-t). Example: i386-elf" >&2
        exit 1
elif [ "$prefix" = "" ]; then
        echo "No prefix specified (-p)." >&2
        exit 1
elif [ "$ld" = "" ]; then
        echo "No directory specified for linker (-l)." >&2
        exit 1
elif [ "$cc" = "" ]; then
        echo "No directory specified for libgcc (-c)." >&2
        exit 1
elif [ "$iconv" = "" ]; then
        echo "No directory specified for iconv (-i)." >&2
        exit 1
elif [ "$toolchain_dir" = "" ]; then
        echo "No toolchain base directory specified (-d)." >&2
        exit 1
fi

cc_vers=$(sed 's:.*-::g' <<< ${cc})

if [ ! -e ${toolchain_dir}/${ld} ]; then
        echo "Extracting source for ${ld}..."
        tar -xf ${toolchain_dir}/${ld}.tar.gz -C ${toolchain_dir}
fi
if [ ! -e ${toolchain_dir}/${iconv} ]; then
        echo "Extracting source for ${iconv}..."
        tar -xf ${toolchain_dir}/${iconv}.tar.gz -C ${toolchain_dir}
fi
if [ ! -e ${toolchain_dir}/${cc} ]; then
        echo "Extracting source for ${cc}..."
        tar -xf ${toolchain_dir}/${cc}.tar.bz2 -C ${toolchain_dir}
fi

ld_name=${prefix}/${ld}/bin/${target}-ld
if [ ! -s ${ld_name} ]; then
        mkdir -p ${prefix}/${ld}
        pushd ${prefix}/${ld}
        echo "Building ${ld_name}..."
        ${toolchain_dir}/${ld}/configure \
                --target=${target} \
                --prefix=${prefix}/${ld} \
                --with-sysroot \
                --disable-nls \
                --disable-werror
        make -j && make install
        popd
else
        echo "Detected target ${ld_name}, skipping build"
fi

iconv_name=${prefix}/${iconv}/lib/libcharset.a
if [ ! -s ${iconv_name} ]; then
        mkdir -p ${prefix}/${iconv}
        pushd ${prefix}/${iconv}
        echo "Building ${iconv_name}..."
        ${toolchain_dir}/${iconv}/configure \
                --target=${target} \
                --prefix=${prefix}/${iconv} \
                --with-sysroot \
                --disable-nls \
                --disable-werror
        make -j && make install
        popd
else
        echo "Detected target ${iconv_name}, skipping build"
fi

# Required for building libgcc
export PATH=${prefix}/${ld}/bin:${PATH}

cc_name=${prefix}/${cc}/gcc/libgcc.a
if [ ! -s ${cc_name} ]; then
        mkdir -p ${prefix}/${cc}
        pushd ${prefix}/${cc}
        echo "Building ${cc_name}..."
        ${toolchain_dir}/${cc}/configure \
                --target=${target} \
                --prefix=${prefix}/${cc} \
                --disable-nls \
                --disable-werror \
                --enable-languages=c \
                --with-libiconv-prefix=${prefix}/${iconv} \
                --without-headers
        make -j4 all-target-libgcc && make install-target-libgcc
        popd
else
        echo "Detected target ${cc_name}, skipping build"
fi

