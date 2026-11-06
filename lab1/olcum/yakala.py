import serial, time
s = serial.Serial('COM8', 115200, timeout=1)
bitis = time.time() + 20
with open('esp32_ham.txt', 'wb') as f:
    while time.time() < bitis:
        f.write(s.read(4096))
print("bitti")