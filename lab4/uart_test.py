import serial

p = serial.Serial("/dev/serial0", 115200, timeout=1)
p.write(b"merhaba\n")
print(p.readline())
