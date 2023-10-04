import serial
import os,sys
import time

file_name = sys.argv[1]
tty = serial.Serial("/dev/ttyUSB0", 115200, timeout=0.5)
file_stats = os.stat(file_name)

# issue request and tell the size of img to rec
tty.write(str(file_stats.st_size).encode('utf-8'))
time.sleep(0.001)

# size sended
tty.write("\n".encode('utf-8'))
time.sleep(0.001)

# send img byte-by-byte
# delay to ensure no loss
with open(file_name, "rb") as fp:
    byte = fp.read(1)
    while byte:
        tty.write(byte)
        byte = fp.read(1)
        tty.flush()
        time.sleep(0.0001)
