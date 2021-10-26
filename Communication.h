#ifndef COMMUNICATION_HEADER_GUARD
#define COMMUNICATION_HEADER_GUARD

#include "Mesh.h"

enum class PacketType : unsigned int {
    InvalidPacket,
    IterationData,
    BenchmarkData
};

struct Packet {
    const PacketType type;
    // lenght does not include the packet header
    const size_t length;

    Packet(PacketType t, size_t len): type(t), length(len) {}
};

template<size_t nNodesT>
struct IterationDataPacket : public Packet {
    float tau;
    int iteration;
    unsigned nIterations;
    const size_t nNodes = nNodesT;
    const size_t nodeSize = sizeof(typename Mesh<nNodesT>::Node);
    const typename Mesh<nNodesT>::Node* nodes;

    IterationDataPacket(const typename Mesh<nNodesT>::Node* nodes):
        Packet(PacketType::IterationData, sizeof(*this) - sizeof(Packet) - sizeof(nodes)),
        nodes(nodes)
    {}

    void send() {
        Serial.write((const byte*)this, sizeof(*this) - sizeof(nodes));
        Serial.write((const byte*)nodes, nodeSize*nNodes);
    }
};

struct BenchmarkDataPacket : public Packet {
    unsigned long microsStart;
    unsigned long microsEnd;

    BenchmarkDataPacket():
        Packet(PacketType::BenchmarkData, sizeof(*this) - sizeof(Packet)),
        microsStart(0),
        microsEnd(0)
    {}

    BenchmarkDataPacket& start() {
        microsStart = micros();
        return *this;
    }

    BenchmarkDataPacket& end() {
        microsEnd = micros();
        return *this;
    }

    BenchmarkDataPacket& send() {
        // send whole packet, with header!
        Serial.write((const byte*)this, sizeof(*this));
        return *this;
    }

    BenchmarkDataPacket& clear() {
        microsStart = 0;
        microsEnd = 0;
        return *this;
    }
};

#endif
