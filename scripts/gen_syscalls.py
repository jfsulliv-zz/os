#!/usr/bin/env python
# Generates the syscall.h header by scanning for all system calls
# defined in the syscalls.master file.

MASTER_FILE='kernel/syscall/syscalls.master'
OUTPUT_SRC='kernel/syscall/syscall_table.c'
OUTPUT_HDR='include/sys/syscalls.h'

def parse_master_file(fn):
        syscalls=dict()
        with open(fn) as f:
                lines = f.readlines()
                for line in lines:
                        if line[0] is "#":
                                continue
                        words = line.split()
                        syscalls[int(words[0])] = (words[1], int(words[2]))
        return syscalls

def src_header():
        return """#include <sys/syscalls.h>

/* GENERATED -- DO NOT EDIT (see scripts/gen_syscalls.py) */
/* System call table for the kernel */
"""

def hdr_header():
        return """#ifndef _SYS_SYSCALLS_H_
#define _SYS_SYSCALLS_H_

#include <sys/syscall_constants.h>

/* GENERATED -- DO NOT EDIT (see scripts/gen_syscalls.py) */
/* System call numbers */
"""

def hdr_footer():
        return "#endif"

def syscalls_to_src_hdr(syscalls):
        if len(syscalls.keys()) is 0:
                return (None, None)
        table = 'const sysent_t syscalls[] =\n{\n'
        table += '        /*  name, num_args, func_ptr */\n'
        defines = '/* System call numbers */\n'
        table_fmt = '        {{ "{name}", {num_args}, (void *)sys_{name} }}, /* {num} */\n'
        defines_fmt = '#define SYS_{name}  {num}\n'
        max_num=-1
        for num in syscalls.keys():
                if num > max_num:
                        max_num = num
                (name, num_args) = syscalls[num]
                if name is None or num_args is None:
                        return (None, None)
                if num_args < 0 or num_args > 6:
                        return (None, None)
                table += table_fmt.format(name=name, num=num,
                                          num_args=num_args)
                defines += defines_fmt.format(name=name, num=num)
        table += "};\n"
        defines += defines_fmt.format(name="MAXNR", num=max_num+1)
        src = (src_header() + "\n\n" +\
                table)
        hdr = (hdr_header() + "\n\n" +\
                defines + "\n\n" +\
                hdr_footer() + "\n")
        return (src, hdr)

def main():
        syscalls = parse_master_file(MASTER_FILE)
        (source, header) = syscalls_to_src_hdr(syscalls)
        if header is None or source is None:
                print("ERROR: Failed to parse {}".format(MASTER_FILE))
                sys.exit(1)
        with open(OUTPUT_HDR, 'w') as of:
                of.write(header)
        with open(OUTPUT_SRC, 'w') as of:
                of.write(source)

if __name__ == "__main__":
        main()
