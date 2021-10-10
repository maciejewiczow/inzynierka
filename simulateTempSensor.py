from dataclasses import dataclass, field
from typing import List, cast
from serial import Serial
from math import exp, log1p
import sys
from sys import argv
from binaryComunication import ArduinoIterationPacket
from heatingCurve import heatingCurve
from util import getAndUpdateArduinoPort
import time
import struct
from matplotlib import pyplot as plt
import numpy as np

port = getAndUpdateArduinoPort('./.vscode/arduino.json')

serial = Serial(port=port.port, baudrate=9600, timeout=7000)
time.sleep(8)
serial.write(0)
time.sleep(5)

tauStart = 1
tauEnd = 300
tStart = 20
tEnd = 4000

temp = heatingCurve(tauStart, tStart, tauEnd, tEnd)

firstInter = True
manualInput = len(argv) > 1

packet = ArduinoIterationPacket()

plt.ion()

figure, (oneCycleAx, allCyclesAx) = plt.subplots(nrows=2, ncols=1, figsize=(10, 8))

oneCycleAx.title.set_text("Temperatura w jednym cyklu")
allCyclesAx.title.set_text("Temperatura")

oneCycleData = np.array([[0,20,20]])
allCyclesData = np.array([[tauStart,tStart,tStart,tStart]])

try:
    for tau in np.arange(1.0, 300.0, 0.5):
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
            # print(f">> {serial.readline().decode('utf-8').strip()}")
            packet.unpackFromSerial(serial)
            oneCycleData = np.array([[packet.tau, packet.nodes[0].t, packet.nodes[-1].t]])

            while packet.iteration < packet.nIterations:
                packet.unpackFromSerial(serial)
                oneCycleData = np.append(oneCycleData, [[packet.tau, packet.nodes[0].t, packet.nodes[-1].t]], axis=0)

            # print(oneCycleData)
            oneCycleAx.clear()
            oneCycleAx.plot(oneCycleData[:,0], oneCycleData[:,1:])

            oneCycleData = np.array([[0,0,0]])
            allCyclesData = np.append(allCyclesData, [[tau, t, packet.nodes[0].t, packet.nodes[-1].t]], axis=0)

            # print(allCyclesData)
            allCyclesAx.clear()
            allCyclesAx.plot(allCyclesData[:,0], allCyclesData[:,1:])

            figure.canvas.draw()
            figure.canvas.flush_events()

        except KeyboardInterrupt:
            if firstInter and not manualInput:
                manualInput = True
                firstInter = False
            else:
                raise

except KeyboardInterrupt:
    pass

