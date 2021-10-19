from math import nan
import struct
from dataclasses import dataclass, field
from typing import List
from serial import Serial
from enum import IntEnum
from datetime import datetime, timedelta

class PacketType(IntEnum):
    InvalidPacket = 0
    IterationData = 1
    BenchmarkData = 2

@dataclass
class Packet:
    type: PacketType = PacketType.InvalidPacket
    lenght: int = 0

    def unpackHeader(self, serial: Serial):
        data = serial.read(2*2)
        type, length = struct.unpack('HH', data)
        self.type = type
        self.lenght = length

    @staticmethod
    def readFromSerial(serial: Serial):
        packetBase = Packet()
        packetBase.unpackHeader(serial)

        PacketClass = packetMap.get(packetBase.type, None)

        if not PacketClass:
            return

        packet = PacketClass()
        packet.type = packetBase.type
        packet.lenght = packetBase.lenght
        packet.unpackFromSerial(serial, unpackHeader=False)

        return packet


@dataclass
class ArduinoIterationPacket(Packet):
    tau: float = 0
    iteration: int = 0
    nIterations: int = 0
    nNodes: int = 0
    nodeSize: int = 0
    nodes: List['Node'] = field(default_factory=list)

    @dataclass
    class Node:
        t: float = 0
        x: float = 0

        def unpack(self, data: bytes):
            t, x = struct.unpack('ff', data)
            self.t = t
            self.x = x

    def unpackFromSerial(self, serial: Serial, unpackHeader = True) -> None:
        self.nodes.clear()

        if unpackHeader:
            self.unpackHeader(serial)

        data = serial.read(self.lenght)

        tau, iteration, nIterations, nNodes, nodeSize = struct.unpack('fhHHH', data)
        self.tau = tau
        self.iteration = iteration
        self.nIterations = nIterations
        self.nNodes = nNodes
        self.nodeSize = nodeSize

        if self.nodeSize * self.nNodes == 0:
            return

        data = serial.read(self.nodeSize * self.nNodes)

        for i in range(0, len(data), self.nodeSize):
            node = ArduinoIterationPacket.Node()
            node.unpack(data[i:i+self.nodeSize])
            self.nodes.append(node)

def totalMicrosecons(diff: timedelta) -> int:
    micros = diff.days * 24 * 60 * 60 * 1000000
    micros += diff.seconds * 1000000
    micros += diff.microseconds

    return micros

@dataclass
class ArduinoBenchmarkPacket(Packet):
    arduinoMicrosStart: int = nan
    arduinoMicrosEnd: int = nan

    def unpackFromSerial(self, serial: Serial, unpackHeader = True):
        if unpackHeader:
            self.unpackHeader(serial)

        microsStart, microsEnd = struct.unpack('LL', serial.read(self.lenght))

        # if self.pcTimeStart and not self.pcTimeEnd:
        #     self.pcTimeEnd = datetime.now()

        # if not self.pcTimeStart and not self.pcTimeEnd:
        #     self.pcTimeStart = datetime.now()

        self.arduinoMicrosStart = microsStart
        self.arduinoMicrosEnd = microsEnd

    @property
    def arduinoTimeElapsedMicros(self):
        return self.arduinoMicrosEnd - self.arduinoMicrosStart

packetMap = {
    PacketType.IterationData: ArduinoIterationPacket,
    PacketType.BenchmarkData: ArduinoBenchmarkPacket
}
