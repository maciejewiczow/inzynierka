#include <iostream>
#include "BasicLinearAlgebra.h"
#include "Mesh.h"
#include "Config.h"

namespace meshconfig {
    // teoretycznie max 36 elementów zmieści się w pamięci (obliczenia na kolanie)
    // przymując zużycie 1200 bajtów na same elementy, węzły i macierze globalne
    constexpr size_t nElements = 30;
    constexpr size_t nNodes = nElements + 1;

    // n*sizeof(Node) + n*sizeof(float) + n*sizeof(float)*3
}

namespace input {
    /*
    Stałe parametry:
    len - długość pieca [m] - zmienna kompilacji

    Dane wejściowe:
    v0 - prędkośc pierwszej szpuli [m/s] - wprowadzanie ręczne
    v1 - prędkość drugiej szpuli (>=v0) [m/s] - wprowadzanie ręczne
    r - promień wsadu [m] - wprowadzanie ręczne

    t_0 - temperatura początkowa wsadu [deg C] - wprowadzać ręcznie/z drugiego czujnika temp pokojowej??
    t_furnance - temperatura w piecu [deg C] - z termopary
    */
    constexpr float furnanceLength = 0.2f; // [m]
    float v0 = 0.03f;    // [m/s]
    float v1 = 0.04f;    // [m/s]
    float r = 0.05f;    // [m]

    float t0 = 20.f;      // [deg C]
}

Config config;


namespace simulation {
    float tauEnd;
    float dTau;
    int nIters;
}


Mesh<meshconfig::nNodes> mesh;


void initializeParams() {
    using namespace simulation;
    // założenia:
    //    - liniowe przyspieszenie między v0 i v1
    //    - piec jest po środku między szpulami
    //    - prędkość nie zmienia się znacząco na długości pieca
    float v = 1.5f*input::v0 - 0.5f*input::v1;

    tauEnd = input::furnanceLength/v;

    float a = config.K / (config.C*config.Ro);
    float elemSize = input::r/meshconfig::nElements;

    dTau = (elemSize*elemSize)/(0.5f*a);
    nIters = (tauEnd/dTau) + 1;
    dTau = tauEnd / nIters;

    mesh.generate(input::t0, elemSize);
}

int main() {
    initializeParams();

    float temp = /* getTemp() */ 200.0;

    // for (int iteration = 0; iteration < simulation::nIters; iteration++) {
    //     Serial << "Iteration " << iteration << endl;

    //     mesh.integrateStep(simulation::dTau, input::r, temp, config);

    //     tau += simulation::dTau;
    // }

    while (true) {
        mesh.integrateStep(simulation::dTau, input::r, temp, config);


        for (const auto& node : mesh.nodes)
            std::cout << "x = " << node.x << ", t = " << node.t << std::endl;
    }

    // Serial << temp << endl;

}
