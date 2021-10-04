from serial import Serial
from math import exp, log1p
from sys import argv
from util import getAndUpdateArduinoPort
import time
import struct

port = getAndUpdateArduinoPort('./.vscode/arduino.json')

serial = Serial(port=port.port, baudrate=9600, timeout=.1)

def temp(tau: float):
    sqrt2pi = 2.506628274631
    sigma = 0.39
    mi = -0.6

    f = lambda x:1.0/(x*sigma*sqrt2pi)*exp(-pow(log1p(x-1) - mi, 2)/(2*sigma*sigma))

    peakStart = 304.444052915472 # [s]
    peakEnd = 1100.0 # [s]

    tMax = 10000.0 # [deg C]
    tMin = -180.0 # [deg C]

    tNorm = 0
    if tau < peakStart:
        # rise
        tNorm = f(tau/646.0 + 0.0001)
    elif peakStart <= tau < peakEnd:
        # peak
        tNorm = 2.
    else:
        # fall
        tNorm = f((tau - (peakEnd - peakStart) + 25.081)/646.0 + 0.0001) + 0.041

    tNorm /= 2.

    return tMin + (tMax-tMin)*tNorm

firstInter = True
manualInput = len(argv) > 1

try:
    tau = 40
    dt = 1
    while True:
        try:
            if manualInput:
                try:
                    t = float(input("<< "))
                except ValueError:
                    continue
            else:
                t = temp(tau)
                print(f"<< {t}")

            serial.write(struct.pack('f', t))
            print(f">> {serial.readline().decode('utf-8').strip()}")
            print()

            if not manualInput:
                time.sleep(0.3)

            tau += dt
        except KeyboardInterrupt:
            if firstInter:
                manualInput = True
                firstInter = False
            else:
                raise

except KeyboardInterrupt:
    pass

