#!/usr/bin/env python

import sys
import struct

for line in sys.stdin:
        ws = line.strip('\n').split(' ')
        if len(ws) < 3:
                continue
        print("{} {}".format(ws[2],ws[0]))
