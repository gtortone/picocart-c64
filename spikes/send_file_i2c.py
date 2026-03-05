#!/usr/bin/env python3

import os
import sys
from pyftdi.i2c import I2cController
from i2clib import *

addr = 0
size = 250

filesize = os.path.getsize(sys.argv[1])

print(f"size: {filesize} bytes")
sent = 0
with open(sys.argv[1], "rb") as f:
    while True:
        #print(f"addr: {addr}")
        chunk = f.read(size)
        if not chunk:
            break
        write_memory_buffer(addr, chunk)
        addr += size
        sent += size

        progress = sent / filesize
        bar_len = 40
        filled_len = int(bar_len * progress)
        bar = "=" * filled_len + "-" * (bar_len - filled_len)
        sys.stdout.write(f"\r[{bar}] {progress*100:.1f}%")
        sys.stdout.flush()

    print("\nTransfer completed.")

