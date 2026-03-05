#!/usr/bin/env python3

from pyftdi.i2c import I2cController
from random import randint 
from time import sleep

MAX_RETRIES = 3
DELAY_BETWEEN_RETRIES = 0.02

def crc16_ccitt(data, as_tuple=False):
    crc = 0xFFFF
    for b in data:
        crc ^= (b << 8)
        
        for _ in range(0,8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF

    if as_tuple:
        return ( ((crc & 0xFF00) >> 8) , (crc & 0x00FF) )
    else:
        return crc

def build_i2c_frame(seq: int, cmd: int, data=[]) -> bytes:

    payload = bytes(data)

    SOF = 0xAA
    frame = bytearray()
    frame.append(SOF)
    frame.append(seq & 0xFF)
    frame.append(cmd & 0xFF)
    frame.append(len(data) & 0xFF)
    frame.extend(data)
    
    crc = crc16_ccitt(frame[1:])
    frame.append((crc >> 8) & 0xFF)  # CRC MSB
    frame.append(crc & 0xFF)         # CRC LSB

    return bytes(frame)

def validate_i2c_response(frame: bytes, seq = None):

    if len(frame) < 6:
        return {"valid": False, "error": "frame too short"}

    SOF = frame[0]
    if SOF != 0xAA:
        return {"valid": False, "error": "invalid SOF"}

    seq_rx = frame[1]

    if seq is not None:
        if seq_rx != seq: 
            return {"valid": False, "error": "invalid sequence"}

    cmd = frame[2]
    length = frame[3]

    if len(frame) != 4 + length + 2:  # SOF+SEQ+CMD+LEN+DATA+CRC16
        return {"valid": False, "error": "length mismatch"}

    data = frame[4:4+length]
    crc_rx = (frame[4+length] << 8) | frame[5+length]

    crc_calc = crc16_ccitt(frame[1:4+length])

    if crc_rx != crc_calc:
        return {"valid": False, "error": "CRC mismatch"}

    if cmd >= 0xC0:
        error_code = data[0] if length >= 1 else None
        return {
            "valid": True,
            "seq": hex(seq_rx),
            "cmd": hex(cmd),
            "data": list(data),
            "error_code": error_code
        }

    return {
        "valid": True,
        "seq": hex(seq_rx),
        "cmd": hex(cmd),
        "data": list(data),
        "error_code": None
    }

def send_command(seq, cmd, payload=[], resp_max_len=32):
    for attempt in range(1, MAX_RETRIES + 1):
        frame = build_i2c_frame(seq, cmd, payload)
        try:
            # send command
            slave.write(list(frame))
            sleep(DELAY_BETWEEN_RETRIES)
            
            # read 4 bytes to setup len
            header = slave.read(4)
            if len(header) < 4 or header[0] != 0xAA:
                raise ValueError("header not valid")

            rem_len = int(header[3]) + 2  # SOF+SEQ+CMD+LEN + DATA + CRC
            if rem_len > resp_max_len:     # FIXME
                rem_len = resp_max_len
            
            response = slave.read(rem_len)

            result = validate_i2c_response(header + response)

            return result
            
            #if result["valid"]:
            #    if result["error_code"]:
            #        print(f"I2C slave error (attempt {attempt}): 0x{result['error_code']:02X}")
            #    return result
            #else:
            #    print(f"I2C communication error (attempt {attempt}): {result.get('error')}")
        except Exception as e:
            print(f"I2C exception (attempt {attempt}): {e}")
        sleep(DELAY_BETWEEN_RETRIES)
    return {"valid": False, "error": f"max retries ({MAX_RETRIES}) reached"}

def get_seq():
    return randint(0x00, 0xFF)

#

def ping():
    return send_command(get_seq(), 0x01)

def led(value):
    return send_command(get_seq(), 0x70, [value])

def dump_registers():
    for i in range(0,10):
        print(f'reg {i}: {read_register(i)}')

# main

i2c = I2cController()
i2c.configure('ftdi://ftdi:2232h/1', frequency=400000)
slave = i2c.get_port(0x50)

# commands 

while True:
    print(f"ping: {ping()}")
    print(f"led on: {led(1)}")
    print(f"led off: {led(0)}")

