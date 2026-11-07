import struct
import time
from smbus2 import SMBus

ADRES = 0x68
with SMBus(1) as bus:
    print("WHO_AM_I:", hex(bus.read_byte_data(ADRES, 0x75)))
    bus.write_byte_data(ADRES, 0x6B, 0x00)   # uyku modundan cikar
    time.sleep(0.1)
    for _ in range(10):
        ham = bus.read_i2c_block_data(ADRES, 0x3B, 6)
        ax, ay, az = struct.unpack(">hhh", bytes(ham))
        print(f"ax={ax/16384:.3f} g  ay={ay/16384:.3f} g  az={az/16384:.3f} g")
        time.sleep(0.5)
