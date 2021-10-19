from datetime import datetime
from typing import List, Optional
from serial import Serial
import os
from os import path
from Comunication import ArduinoBenchmarkPacket, ArduinoIterationPacket, Packet, totalMicrosecons
from heatingCurve import heatingCurve
from util import getAndUpdateArduinoPort
import time
import struct
from matplotlib import pyplot as plt
from matplotlib.pyplot import Axes, Line2D
import numpy as np
from argparse import ArgumentParser
import csv as csvModule

parser = ArgumentParser()

parser.add_argument('--tauStart', type=float, default=1)
parser.add_argument('--tauEnd', type=float, default=300)
parser.add_argument('--dTau', type=float, default=0.5)
parser.add_argument("--tStart", type=float, default=20)
parser.add_argument("--tEnd", type=float, default=1700)
parser.add_argument('-o', '--outputFile', type=str)
parser.add_argument('-m', '--manual', dest="manualInput", type=bool, default=False)

args = parser.parse_args()

temp = heatingCurve(args.tauStart, args.tStart, args.tauEnd, args.tEnd)

firstInterrupt = True

plt.ion()

figure, (oneCycleAx, allCyclesAx) = plt.subplots(nrows=2, ncols=1, figsize=(10, 8))

figure.canvas.mpl_connect('close_event', lambda e: exit())

oneCycleAx.title.set_text("Temperatura w jednym cyklu")
allCyclesAx.title.set_text("Temperatura")

for ax in [allCyclesAx, oneCycleAx]:
    ax.set_ylabel("Temperatura [C]")
    ax.set_xlabel("Czas [s]")
    ax.grid()

oneCycleData = np.array([[0,20,20]])
allCyclesData = np.array([[args.tauStart, args.tStart, args.tStart, args.tStart]])

oneCycleLines = oneCycleAx.plot(oneCycleData[:,0], oneCycleData[:,1:])
oneCycleAx.legend(oneCycleLines, ['Temp wewnętrzna', 'Temp zewnętrzna'])

allCyclesLines = allCyclesAx.plot(allCyclesData[:,0], allCyclesData[:,1:])
allCyclesAx.legend(allCyclesLines, ['Temp pieca', 'Końcowa temp wewn', 'Końcowa temp zewn'])

port = getAndUpdateArduinoPort('./.vscode/arduino.json')

serial = Serial(port=port.port, baudrate=57600, timeout=7000)
time.sleep(8)
serial.write(0)
time.sleep(5)

csv: Optional[csvModule.DictWriter] = None

if args.outputFile:
    if not path.exists(path.dirname(args.outputFile)):
        os.makedirs(path.dirname(args.outputFile))

    file = open(args.outputFile, 'x', newline='')
    csv = csvModule.DictWriter(file, fieldnames=['cycle', 'iteration', 'arduinoDurationMicros', 'pcDuration'])
    csv.writeheader()

def updatePlotLines(ax: Axes, lines: List[Line2D], data: np.array) -> None:
    for i, line in enumerate(lines):
        line.set_xdata(data[:,0])
        line.set_ydata(data[:,i+1])

    ax.relim()
    ax.autoscale_view()

try:
    for i, tau in enumerate(np.arange(args.tauStart, args.tauEnd, args.dTau)):
        try:
            if args.manualInput:
                try:
                    t = float(input("<< "))
                except ValueError:
                    continue
            else:
                t = temp(tau)
                print(f"<< {t}")

            serial.write(struct.pack('f', t))

            iterPacket: ArduinoIterationPacket = Packet.readFromSerial(serial)
            print(f"tau = {iterPacket.tau:.4f}, iteration: {iterPacket.iteration}/{iterPacket.nIterations}", end='', flush=True)
            # first packet - before integration step starts
            startTime = datetime.now()
            benchmarkPacket: ArduinoBenchmarkPacket = Packet.readFromSerial(serial)
            # second packet - after integration step
            benchmarkPacket.unpackFromSerial(serial)
            elapsed = datetime.now() - startTime
            oneCycleData = np.array([[iterPacket.tau, iterPacket.nodes[0].t, iterPacket.nodes[-1].t]])

            # write to file
            if csv:
                csv.writerow({
                    'cycle': i,
                    'iteration': iterPacket.iteration,
                    'arduinoDurationMicros': benchmarkPacket.arduinoTimeElapsedMicros,
                    'pcDuration': totalMicrosecons(elapsed)
                })

            while iterPacket.iteration < iterPacket.nIterations:
                iterPacket.unpackFromSerial(serial)
                # first packet - before integration step starts
                startTime = datetime.now()
                benchmarkPacket = Packet.readFromSerial(serial)
                # second packet - after integration step
                benchmarkPacket.unpackFromSerial(serial)
                elapsed = datetime.now() - startTime

                oneCycleData = np.append(oneCycleData, [[iterPacket.tau, iterPacket.nodes[0].t, iterPacket.nodes[-1].t]], axis=0)

                print(f"\rtau = {iterPacket.tau:.4f}, iteration: {iterPacket.iteration}/{iterPacket.nIterations}  ", end='', flush=True)

                if csv:
                    csv.writerow({
                        'cycle': i,
                        'iteration': iterPacket.iteration,
                        'arduinoDurationMicros': benchmarkPacket.arduinoTimeElapsedMicros,
                        'pcDuration': totalMicrosecons(elapsed)
                    })

                figure.canvas.flush_events()

            print()

            # print(oneCycleData)
            updatePlotLines(oneCycleAx, oneCycleLines, oneCycleData)

            oneCycleData = np.array([[0,0,0]])
            allCyclesData = np.append(allCyclesData, [[tau, t, iterPacket.nodes[0].t, iterPacket.nodes[-1].t]], axis=0)

            # print(allCyclesData)
            updatePlotLines(allCyclesAx, allCyclesLines, allCyclesData)

            figure.canvas.draw()
            figure.canvas.flush_events()

        except KeyboardInterrupt:
            if firstInterrupt and not args.manualInput:
                args.manualInput = True
                firstInterrupt = False
                print()
            else:
                raise

except KeyboardInterrupt:
    pass

plt.ioff()
