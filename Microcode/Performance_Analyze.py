import serial
from time import *

conn1 = serial.Serial('COM5',250000)
conn2 = serial.Serial('COM7',250000)

while True:

    conn1.write(bytes("C:-90,Cm:2,E:0,A:0,F:0"+'\n','utf-8'))
    conn2.write(bytes("C:-90,Cm:2,E:0,A:0,F:0"+'\n','utf-8'))
    line = (conn2.readline()).decode('utf-8',errors='ignore')

    if 'Block' in line:
        print(line)

    sleep(0.01)
