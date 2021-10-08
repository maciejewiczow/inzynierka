#include <LiquidCrystal_I2C.h>
#include <BasicLinearAlgebra.h>
#include <print_util.h>
#include <avr/wdt.h>
#include "lcd_util.h"
#include "Mesh.h"
#include "Config.h"
#include "Arduino.h"

using namespace lcdut;
using namespace prnt;

namespace meshconfig {
    // teoretycznie max 36 elementów zmieści się w pamięci (obliczenia na kolanie)
    // przymując zużycie 1200 bajtów na same elementy, węzły i macierze globalne
    constexpr size_t nElements = 15;
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
    constexpr float furnanceLength = 0.2; // [m]
    float v0 = 0.03;    // [m/s]
    float v1 = 0.04;    // [m/s]
    float r = 0.05;    // [m]

    float t0 = 20;      // [deg C]
}

Config config;

ISR(WDT_vect) {
    Serial << "Watchdog reset" << endl;
}

namespace simulation {
    float tauEnd;
    float dTau;
    int nIters;
}

LiquidCrystal_I2C lcd(0x3F, 16, 2);

Mesh<meshconfig::nNodes> mesh;

float getTemp() {
    while (Serial.available() < sizeof(float));

    float res;
    Serial.readBytes((byte*) &res, sizeof(float));

    return res;
}

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

    mesh.generate(input::t0, elemSize);
}

void setup() {
    Serial.begin(9600);
    lcd.init();
    lcd.backlight();

    wdt_enable(WDTO_8S);

    lcd << lcdut::pos(3, 0) << (char)0b10111100 << " Ready " << (char)0b11000101;

    initializeParams();
    Serial << "Params: " << endl
        << "tauEnd = " << simulation::tauEnd << endl
        << "dTau = " << simulation::dTau << endl
        << "nIters = " << simulation::nIters << endl << endl;
}

void loop() {
    float temp = /* getTemp() */ 400.0;

    // for (int iteration = 0; iteration < simulation::nIters; iteration++) {
    //     Serial << "Iteration " << iteration << endl;

    //     mesh.integrateStep(simulation::dTau, input::r, temp, config);

    //     tau += simulation::dTau;
    // }

    lcd << lcdut::clear << lcdut::pos(0, 0) << "Tin = " << mesh.nodes[0].t << lcdut::symbols::deg << "C";
    lcd << lcdut::pos(0, 1) << "Ts = " << mesh.nodes[meshconfig::nNodes-1].t << lcdut::symbols::deg << "C";

    mesh.integrateStep(simulation::dTau, input::r, temp, config);

    for (const auto& node : mesh.nodes)
        Serial << "x = " << node.x << ", t = " << node.t << endl;


    // Serial << temp << endl;

}
