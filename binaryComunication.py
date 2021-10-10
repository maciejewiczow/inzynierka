import struct
from dataclasses import dataclass, field
from typing import List
from serial import Serial

@dataclass
class Node:
    t: float = 0
    x: float = 0

    def unpack(self, data: bytes):
        t, x = struct.unpack('ff', data)
        self.t = t
        self.x = x

@dataclass
class ArduinoIterationPacket:
    tau: float = 0
    iteration: int = 0
    nIterations: int = 0
    nNodes: int = 0
    nodeSize: int = 0
    nodes: List[Node] = field(default_factory=list)

    def unpackFromSerial(self, serial: Serial) -> None:
        self.nodes.clear()
        data = serial.read(12)

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
            node = Node()
            node.unpack(data[i:i+self.nodeSize])
            self.nodes.append(node)
