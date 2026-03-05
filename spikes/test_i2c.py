#!/usr/bin/env python3

from pyftdi.i2c import I2cController
from i2clib import *

# main

# commands 

#for i in range(0,16):
#    print(f"read memory({i}): {read_memory(i)}")

#print(f"write memory(4) value 0x23: {write_memory(4, 0x23)}")
#print(f"write memory(4) value [0x23, 0x26]: {write_memory_buffer(4, [0x23, 0x26])}")
#print(f"write memory(7) value [0x10]: {write_memory_buffer(7, [0x10])}")

#for i in range(0,16):
#    print(f"read memory({i}): {read_memory(i)}")

addr = 0
size = 250
with open("Kickman.crt", "rb") as f:
    while True:
        print(f"addr: {addr}")
        chunk = f.read(size)
        if not chunk:
            break
        print(write_memory_buffer(addr, chunk))
        addr += size

#while True:
#    print(f"ping: {ping()}")
#    print(f"led on: {led(1)}")
#    print(f"led off: {led(0)}")
#
#    dump_registers()
#
#    v = randint(0x00, 0xFF)
#    print(f"write {v} to reg(1): {write_register(1, v)}")
#
#    for i in range(0,16):
#        print(f"read memory({i}): {read_memory(i)}")
#
#    #print(f"write memory(4) value 0x23: {write_memory(4, 0x23)}")
#
#    #for i in range(0,16):
#    #    print(f"read memory({i}): {read_memory(i)}")
#
#    print('---')
#
