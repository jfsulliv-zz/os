#!/usr/bin/env python

import sys
import struct

for line in sys.stdin:
        ws = line.strip('\n').split(' ')
        if len(ws) < 3:
                continue
        print("{}\0{}".format(ws[0],ws[2]), end='\0')
