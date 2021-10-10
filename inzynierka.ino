#include <LiquidCrystal_I2C.h>
#include <BasicLinearAlgebra.h>
#include <print_util.h>
#include <KeepMeAlive.h>
#include "lcd_util.h"
#include "Mesh.h"
#include "Config.h"

using namespace lcdut;
using namespace prnt;

namespace meshconfig {
    constexpr size_t nElements = 20;
    constexpr size_t nNodes = nElements + 1;
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
    constexpr float furnanceLength = 0.2; // [m]
    float v0 = 0.005;    // [m/s]
    float v1 = 0.005;    // [m/s]
    float r = 0.03;    // [m]

    float t0 = 20;      // [deg C]
}

Config config;

namespace simulation {
    float tauEnd;
    float dTau;
    int nIters;
}

LiquidCrystal_I2C lcd{0x3F, 16, 2};

Mesh<meshconfig::nNodes> mesh;

float getTemp() {
    // return 500.f;

    while (Serial.available() < sizeof(float));

    float res;
    Serial.readBytes((byte*) &res, sizeof(float));

    return res;
}

template<size_t nNodesT>
struct IterationDataPacket {
    float tau;
    int iteration;
    unsigned nIterations;
    const size_t nNodes = nNodesT;
    const size_t nodeSize = sizeof(typename Mesh<nNodesT>::Node);
    const typename Mesh<nNodesT>::Node* nodes;

    void send() {
        Serial.write((const byte*)this, sizeof(*this) - sizeof(this->nodes));
        Serial.write((const byte*)nodes, nodeSize*nNodes);
    }
};

IterationDataPacket<meshconfig::nNodes> iterData;

void initializeParams() {
    using namespace simulation;
    // założenia:
    //    - liniowe przyspieszenie między v0 i v1
    //    - piec jest po środku między szpulami
    //    - prędkość nie zmienia się znacząco na długości pieca
    float v = 1.5*input::v0 - 0.5*input::v1;

    tauEnd = input::furnanceLength/v;

    float a = config.K / (config.C*config.Ro);
    float elemSize = input::r/meshconfig::nElements;

    dTau = (elemSize*elemSize)/(0.5*a);
    // dTau = 0.5;
    nIters = (tauEnd/dTau) + 1;
    dTau = tauEnd / nIters;

    iterData.nIterations = nIters;

    mesh.generate(input::t0, elemSize);
}

void setup() {
    Serial.begin(9600);
    lcd.init();
    lcd.backlight();
    watchdogTimer.setDelay(8000);

    lcd << lcdut::pos(3, 0) << (char)0b10111100 << " Ready " << (char)0b11000101;

    while (!Serial);

    initializeParams();
    // Serial << "Params: " << endl
    //     << "tauEnd = " << simulation::tauEnd << endl
    //     << "dTau = " << simulation::dTau << endl
    //     << "nIters = " << simulation::nIters << endl << endl;

    iterData.iteration = 0;
    iterData.tau = 0.f;
    iterData.nodes = mesh.nodes;
}

void loop() {
    static float temp = getTemp();

    mesh.integrateStep(simulation::dTau, input::r, temp, config);

    if (iterData.iteration == simulation::nIters) {
        lcd << lcdut::clear << lcdut::pos(0, 0) << "Wew " << mesh.nodes[0].t << lcdut::symbols::deg << "C";
        lcd << lcdut::pos(0, 1) << "Zew " << mesh.nodes[meshconfig::nNodes-1].t << lcdut::symbols::deg << "C";

        iterData.iteration = 0;
        iterData.tau = 0.f;
        temp = getTemp();

        for (auto& node : mesh.nodes)
            node.t = input::t0;

    } else {
        iterData.iteration++;
        iterData.tau += simulation::dTau;
    }

    iterData.send();
}
