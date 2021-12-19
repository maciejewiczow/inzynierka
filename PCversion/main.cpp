#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>

#include "BasicLinearAlgebra.h"
#include "Mesh.h"
#include "Config.h"
#include "communication.h"
#include "material.h"

namespace fs = std::filesystem;
namespace chron = std::chrono;

namespace meshconfig {
    constexpr size_t nElements = 5;
    constexpr size_t nNodes = nElements + 1;
} // namespace meshconfig

struct Input {
    /*
    Dane wejściowe:
    v0 - prędkośc pierwszej szpuli [m/s] - wprowadzanie ręczne
    v1 - prędkość drugiej szpuli (>=v0) [m/s] - wprowadzanie ręczne
    r - promień wsadu [m] - wprowadzanie ręczne
    len - długość pieca [m] - zmienna kompilacji
    n - ilośc kroków czasowych do wykonania

    t_0 - temperatura początkowa wsadu [deg C] - wprowadzać ręcznie/z drugiego czujnika temp pokojowej
    t_furnance - temperatura w piecu [deg C] - z termopary
    */
    float furnaceLength = 0.02;    // [m]
    float v0 = 0.005;               // [m/s]
    float v1 = 0.0055;               // [m/s]
    float r = 0.005;                // [m]
    float t0 = 20;                  // [deg C]
    unsigned int nSteps = 0;
};

Input input;
Material config;

namespace simulation {
    float tauEnd;
    float dTau;
} // namespace simulation

Mesh<meshconfig::nNodes> mesh;

float getTemp(float x, float tStart, float tEnd, float tauStart, float tauEnd) {
    if (x < tauStart)
        return tStart;

    if (x > tauEnd)
        return tEnd;

    return (tStart - tEnd)/(log(tauStart) - log(tauEnd))*log(x*exp((tEnd*log(tauStart) - tStart*log(tauEnd))/(tStart - tEnd)));
}

IterationDataPacket<meshconfig::nNodes> iterData{mesh.nodes};

void calculateSimulationParams() {
    using namespace simulation;
    // założenia:
    //    - liniowe przyspieszenie między v0 i v1
    //    - piec jest po środku między szpulami
    //    - prędkość nie zmienia się znacząco na długości pieca
    float v = (input.v0 + input.v1)/2.f;

    tauEnd = input.furnaceLength / v;

    // float a = config.K / (config.C * config.Ro);
    float elemSize = input.r / meshconfig::nElements;

    // dTau = (elemSize * elemSize) / (0.5 * a);
    // nSteps = (tauEnd / dTau) + 1;
    dTau = tauEnd / input.nSteps;

    iterData.nSteps = input.nSteps;
    iterData.step = 0;
    iterData.tau = 0.f;

    mesh.generate(input.t0, elemSize);
}

int main(int argc, char* argv[]) {
    if (argc < 3)
        return -1;

    float tauStart = 0.0001;
    float tauEnd = 10;
    float tEnd = 800.f;
    float tau = tauStart;
    float dTau = 0.5;

    for (int i = 2; i < argc; i++) {
        char filename[400] = { 0 };

        input.nSteps = atoi(argv[i]);
        snprintf(filename, 400, argv[1], input.nSteps);
        calculateSimulationParams();

        fs::path path{filename};

        if (!fs::exists(path.parent_path()))
            fs::create_directories(path.parent_path());

        std::ofstream out{path};

        out << "cycle, iteration, arduinoDurationMicros, pcDuration, tempAmb, tempIn, tempOut\n";

        tau = tauStart;
        for (int j = 1; tau < tauEnd; tau += dTau, j++) {
            float temp = getTemp(tau, 20, tEnd, tauStart, tauEnd);

            while (iterData.step < input.nSteps) {
                auto start = chron::high_resolution_clock::now();
                mesh.integrateStep(simulation::dTau, input.r, temp, config);

                auto duration = chron::duration_cast<chron::microseconds>(chron::high_resolution_clock::now() - start).count();
                out << j << ','
                    << iterData.step << ','
                    << duration << ','
                    << duration << ','
                    << temp << ','
                    << mesh.nodes[0].t << ','
                    << mesh.nodes[meshconfig::nNodes - 1].t << '\n';

                iterData.step++;
                iterData.tau += simulation::dTau;
            }

            iterData.step = 0;
            iterData.tau = 0.f;

            for (auto& node : mesh.nodes)
                node.t = input.t0;
        }
    }

    return 0;
}
