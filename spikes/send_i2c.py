#!/usr/bin/env python3

from pyftdi.i2c import I2cController
from time import sleep

def read_register(regaddr):
    slave.write([0x10, regaddr])
    data = slave.read(1)
    return(data[0])

def write_register(regaddr, value):
    slave.write([0x11, regaddr, value])

def dump_registers():
    for i in range(0,10):
        print(f'reg {i}: {read_register(i)}')


i2c = I2cController()

i2c.configure('ftdi://ftdi:2232h/1', frequency=400000)

slave_address = 0x50

slave = i2c.get_port(slave_address)

dump_registers()

# write register 0x01
write_register(0x01, 81)

print("\n")

dump_registers()

print("\n")

value = 0 
# loop
while True:
    write_register(0x01, value)
    dump_registers()
    value = (value + 1) % 255
    print("\n")
