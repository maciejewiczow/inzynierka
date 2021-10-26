#include <BasicLinearAlgebra.h>
#include <InputDebounce.h>
#include <KeepMeAlive.h>
#include <LiquidCrystal_I2C.h>
#include <print_util.h>
#include <EEPROM.h>
#include "lcd_util.h"
#include "Communication.h"
#include "Config.h"
#include "Menu.h"
#include "Mesh.h"

#define SERIAL_BAUD 57600

#define SET_BUTTON_PIN 7
#define LEFT_BUTTON_PIN 8
#define RIGHT_BUTTON_PIN 9
#define INCREMENT_BUTTON_PIN 10

#define EEPROM_INPUT_PARAMS_ADDR 0
#define EEPROM_READ_INDICATOR_VAL 69

using namespace lcdut;
using namespace prnt;

namespace meshconfig {
    constexpr size_t nElements = 5;
    constexpr size_t nNodes = nElements + 1;
} // namespace meshconfig

struct Input {
    /*
    Stałe parametry:
        len - długość pieca [m] - zmienna kompilacji

    Dane wejściowe:
        v0 - prędkośc pierwszej szpuli [m/s] - wprowadzanie ręczne
        v1 - prędkość drugiej szpuli (>=v0) [m/s] - wprowadzanie ręczne
        r - promień wsadu [m] - wprowadzanie ręczne

        t_0 - temperatura początkowa wsadu [deg C] - wprowadzać ręcznie/z drugiego czujnika temp pokojowej
        t_furnance - temperatura w piecu [deg C] - z termopary
    */
    float furnaceLength = 0.2; // [m]
    float v0 = 0.005;          // [m/s]
    float v1 = 0.005;          // [m/s]
    float r = 0.002;           // [m]

    float t0 = 20; // [deg C]
};

Input input;
Config config;

namespace simulation {
    float tauEnd;
    float dTau;
    int nIters;
} // namespace simulation

LiquidCrystal_I2C lcd{0x3F, 16, 2};

Mesh<meshconfig::nNodes> mesh;

float getTemp() {
    if (Serial.available() < sizeof(float))
        return NAN;

    float res;
    Serial.readBytes((byte*) &res, sizeof(float));

    return res;
}

IterationDataPacket<meshconfig::nNodes> iterData{mesh.nodes};
BenchmarkDataPacket benchmark;

void calculateSimulationParams() {
    using namespace simulation;
    // założenia:
    //    - liniowe przyspieszenie między v0 i v1
    //    - piec jest po środku między szpulami
    //    - prędkość nie zmienia się znacząco na długości pieca
    float v = 1.5 * input.v0 - 0.5 * input.v1;

    tauEnd = input.furnaceLength / v;

    float a = config.K / (config.C * config.Ro);
    float elemSize = input.r / meshconfig::nElements;

    dTau = (elemSize * elemSize) / (0.5 * a);
    nIters = (tauEnd / dTau) + 1;
    dTau = tauEnd / nIters;

    iterData.nIterations = nIters;
    iterData.iteration = 0;
    iterData.tau = 0.f;

    mesh.generate(input.t0, elemSize);
}

void updateEEPROM() {
    EEPROM.update(EEPROM_INPUT_PARAMS_ADDR, EEPROM_READ_INDICATOR_VAL);
    EEPROM.put(EEPROM_INPUT_PARAMS_ADDR + 1, input);
}

Menu::Item menuItems[] = {
    { "t0 [C]",              &input.t0              },
    { "Promien wsadu[m]",    &input.r               },
    { "v0 [m/s]",            &input.v0              },
    { "v1 [m/s]",            &input.v1              },
    { "Dlg. pieca [m]",      &input.furnaceLength   },
};

Menu menu{lcd, menuItems};

void setup() {
    Serial.begin(SERIAL_BAUD);
    lcd.init();
    lcd.backlight();
    watchdogTimer.setDelay(8000);

    lcd << pos(3, 0) << (char) 0b10111100 << " Ready " << (char) 0b11000101;
    // lcd << pos(0, 1) << sizeof(unsigned long);

    while (!Serial)
        ;

    if (EEPROM.read(EEPROM_INPUT_PARAMS_ADDR) == EEPROM_READ_INDICATOR_VAL)
        EEPROM.get(EEPROM_INPUT_PARAMS_ADDR + 1, input);

    calculateSimulationParams();
    // Serial << "Params: " << endl
    //     << "tauEnd = " << simulation::tauEnd << endl
    //     << "dTau = " << simulation::dTau << endl
    //     << "nIters = " << simulation::nIters << endl << endl;

    menu.setup(SET_BUTTON_PIN, LEFT_BUTTON_PIN, RIGHT_BUTTON_PIN, INCREMENT_BUTTON_PIN);
    menu.onParamUpdate([](){
        calculateSimulationParams();
        updateEEPROM();
    });
}

void loop() {
    static float temp = getTemp();

    menu.update();

    if (isnan(temp)) {
        delay(50);
        temp = getTemp();
        return;
    }

    iterData.send();

    benchmark.start().send();
    mesh.integrateStep(simulation::dTau, input.r, temp, config);
    benchmark.end().send().clear();

    if (iterData.iteration < simulation::nIters) {
        iterData.iteration++;
        iterData.tau += simulation::dTau;
    }
    else {
        lcd << clear << pos(0, 0) << "Wew " << mesh.nodes[0].t << symbols::deg
            << "C";
        lcd << pos(0, 1) << "Zew " << mesh.nodes[meshconfig::nNodes - 1].t
            << symbols::deg << "C";

        iterData.iteration = 0;
        iterData.tau = 0.f;

        for (auto& node : mesh.nodes)
            node.t = input.t0;

        temp = getTemp();
    }
}
