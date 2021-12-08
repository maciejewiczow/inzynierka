#include <BasicLinearAlgebra.h>
#include <InputDebounce.h>
#include <KeepMeAlive.h>
#include <LiquidCrystal_I2C.h>
#include <print_util.h>
#include <EEPROM.h>

#include "config.h"
#include "lcd_util.h"
#include "communication.h"
#include "material.h"
#include "Menu.h"
#include "Mesh.h"
#include "BufferedLcd.h"

using namespace lcdut;
using namespace prnt;

namespace meshconfig {
    constexpr size_t nElements = GRID_SIZE;
    constexpr size_t nNodes = nElements + 1;
} // namespace meshconfig

constexpr float minParamValue = 0.000001f;

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
    float furnaceLength = 0.002;    // [m]
    float v0 = 0.005;               // [m/s]
    float v1 = 0.005;               // [m/s]
    float r = 0.002;                // [m]
    float t0 = 20;                  // [deg C]
    unsigned int nIters = 20;
};

Input input;
Material config;

namespace simulation {
    float tauEnd;
    float dTau;
} // namespace simulation

BufferedLcd<16, 2> lcd{LCD_I2C_ADDR};

Mesh<meshconfig::nNodes> mesh;

float getTemp() {
    #if DEBUG_SERIAL_TEMP
    if (Serial.available() < sizeof(float))
        return NAN;

    float res;
    Serial.readBytes((byte*) &res, sizeof(float));

    return res;
    #else
    return 200;
    #endif
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

    // float a = config.K / (config.C * config.Ro);
    float elemSize = input.r / meshconfig::nElements;

    // dTau = (elemSize * elemSize) / (0.5 * a);
    // nIters = (tauEnd / dTau) + 1;
    dTau = tauEnd / input.nIters;

    iterData.nIterations = input.nIters;
    iterData.iteration = 0;
    iterData.tau = 0.f;

    mesh.generate(input.t0, elemSize);
}

void updateEEPROM() {
    EEPROM.update(EEPROM_INPUT_PARAMS_ADDR, EEPROM_READ_INDICATOR_VAL);
    EEPROM.put(EEPROM_INPUT_PARAMS_ADDR + 1, input);
}

constexpr size_t nMenuItems = 6;
// using MyMenu = Menu;
using MyMenu = Menu<nMenuItems, lcd.cols, lcd.rows>;

MenuItem menuItems[] = {
    { "t0 [C]",             &input.t0,             MenuItemType::_float },
    { "Promien wsadu[m]",   &input.r,              MenuItemType::_float },
    { "v0 [m/s]",           &input.v0,             MenuItemType::_float },
    { "v1 [m/s]",           &input.v1,             MenuItemType::_float },
    { "Dlg. pieca [m]",     &input.furnaceLength,  MenuItemType::_float },
    { "N krokow czas.",     &input.nIters,         MenuItemType::_uint  },
};

MyMenu menu{lcd, menuItems};

void setup() {
    Serial.begin(SERIAL_BAUD);
    lcd.init();
    lcd.backlight();
    watchdogTimer.setDelay(8000);

    while (!Serial)
        ;

    lcd << pos(3, 0) << (char) 0b10111100 << " Ready " << (char) 0b11000101;
    lcd.flush();

    if (EEPROM.read(EEPROM_INPUT_PARAMS_ADDR) == EEPROM_READ_INDICATOR_VAL)
        EEPROM.get(EEPROM_INPUT_PARAMS_ADDR + 1, input);

    calculateSimulationParams();
    DBG_Serial(
        "Params: " << endl
        << "tauEnd = " << simulation::tauEnd << endl
        << "dTau = " << simulation::dTau << endl
    );

    menu.setup(
        SET_BUTTON_PIN,
        LEFT_BUTTON_PIN,
        RIGHT_BUTTON_PIN,
        INCREMENT_BUTTON_PIN,
        DECREMENT_BUTTON_PIN
    );
    menu.onParamUpdate([](){
        if (input.r == 0.f)
            input.r = minParamValue;

        if (input.v0 == 0.f)
            input.v0 = minParamValue;

        if (input.v1 == 0.f)
            input.v1 = minParamValue;

        if (input.furnaceLength == 0.f)
            input.furnaceLength = minParamValue;

        if (input.nIters == 0)
            input.nIters = 1;

        if (input.r < 0.f)
            input.r *= -1;

        if (input.v0 < 0.f)
            input.v0 *= -1;

        if (input.v1 < 0.f)
            input.v1 *= -1;

        if (input.furnaceLength < 0.f)
            input.furnaceLength *= -1;

        if (input.v0 > input.v1)
            input.v1 = input.v0;

        DBG_Serial(nameof(input.nIters) << " = " << input.nIters << endl);

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

    if (iterData.iteration < input.nIters) {
        iterData.iteration++;
        iterData.tau += simulation::dTau;
    }
    else {
        lcd << clear << pos(0, 0) << "Wew " << mesh.nodes[0].t << symbols::deg
            << "C";
        lcd << pos(0, 1) << "Zew " << mesh.nodes[meshconfig::nNodes - 1].t
            << symbols::deg << "C";

        lcd.flush();

        iterData.iteration = 0;
        iterData.tau = 0.f;

        for (auto& node : mesh.nodes)
            node.t = input.t0;

        temp = getTemp();
    }
}
