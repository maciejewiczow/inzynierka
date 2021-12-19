#include <BasicLinearAlgebra.h>
#include <InputDebounce.h>
#include <KeepMeAlive.h>
#include <LiquidCrystal_I2C.h>
#include <print_util.h>
#include <EEPROM.h>
#include <Adafruit_MAX31855.h>

#include "config.h"
#include "lcd_util.h"
#include "communication.h"
#include "Menu.h"
#include "Mesh.h"
#include "BufferedLcd.h"

using namespace lcdut;
using namespace prnt;

namespace meshconfig {
    constexpr size_t nElements = MESH_SIZE;
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
    unsigned nSteps = 20;
    unsigned integrationScheme = 1;
    float alphaAir = 300;   // [W/(m^2*K)]
    float C = 700.0;        // [J/(kg*K)]
    float Ro = 7800.0;      // [kg/m^3]
    float K = 25.0;         // [W/m*K]
};

Input input{};

namespace simulation {
    float tauEnd;
    float dTau;
} // namespace simulation

BufferedLcd<LCD_COLS, LCD_ROWS> lcd{LCD_I2C_ADDR};
Adafruit_MAX31855 thermocouple{TEMP_CLK_PIN, TEMP_CS_PIN, TEMP_DO_PIN};

float getTemp() {
    #if DEBUG_SERIAL_TEMP
    if (Serial.available() < sizeof(float))
        return NAN;

    float res;
    Serial.readBytes((byte*) &res, sizeof(float));

    return res;
    #else
    DBG_Serial("Internal Temp = " << thermocouple.readInternal() << endl);

    float c = thermocouple.readCelsius();

    #if DEBUG_PRINTS
    if (isnan(c)) {
        Serial.println("Something wrong with thermocouple!");
        Serial << "error: " << thermocouple.readError() << endl;
    } else {
        Serial << "C = " << c << endl;
    }
    #endif

    return c;
    #endif
}

Mesh<meshconfig::nNodes> mesh{};
IterationDataPacket<meshconfig::nNodes> iterData{mesh.nodes};

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
    // nSteps = (tauEnd / dTau) + 1;
    dTau = tauEnd / input.nSteps;

    iterData.nSteps = input.nSteps;
    iterData.step = 0;
    iterData.tau = 0.f;

    mesh.generate(input.t0, elemSize);
}

void updateEEPROM() {
    EEPROM.update(EEPROM_INPUT_PARAMS_ADDR, EEPROM_READ_INDICATOR_VAL);
    EEPROM.put(EEPROM_INPUT_PARAMS_ADDR + 1, input);
}

#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))

MenuItem menuItems[] = {
    // max lcd.cols chars!
    { "N krokow czas.",     &input.nSteps,            MenuItemType::_uint  },
    { "t0 [C]",             &input.t0,                MenuItemType::_float },
    { "Promien wsadu[m]",   &input.r,                 MenuItemType::_float },
    { "v0 [m/s]",           &input.v0,                MenuItemType::_float },
    { "v1 [m/s]",           &input.v1,                MenuItemType::_float },
    { "Sch. calk. 1-4",     &input.integrationScheme, MenuItemType::_uint  },
    { "\xE0 air [W/m^2*K]", &input.alphaAir,          MenuItemType::_float },
    { "Cp. wl. [J/kgK]",    &input.C,                 MenuItemType::_float },
    { "\xE6 [kg/m3]",       &input.Ro,                MenuItemType::_float },
    { "K [W/m*K]",          &input.K,                 MenuItemType::_float },
    { "Dlg. pieca [m]",     &input.furnaceLength,     MenuItemType::_float },
};

// using MyMenu = Menu;
using MyMenu = Menu<ARRAYSIZE(menuItems), lcd.cols, lcd.rows>;
MyMenu menu{lcd, menuItems};

bool isError = false;
char stored[lcd.rows*lcd.cols];
BenchmarkDataPacket benchmark;

void setup() {
    Serial.begin(SERIAL_BAUD);
    lcd.init();
    lcd.backlight();
    watchdogTimer.setDelay(8000);

    while (!Serial)
        ;

    if (!thermocouple.begin())
        Serial << "Thermocouple init error" << endl;

    lcd << pos(3, 0) << (char) 0b10111100 << " Ready " << (char) 0b11000101;
    lcd.flush();
    lcd.saveContents(stored);

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

        if (input.nSteps == 0)
            input.nSteps = 1;

        if (input.integrationScheme < 1)
            input.integrationScheme = 1;

        if (input.integrationScheme > 4)
            input.integrationScheme = 4;

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

        DBG_Serial(nameof(input.nSteps) << " = " << input.nSteps << endl);

        calculateSimulationParams();
        updateEEPROM();
    });
}

void loop() {
    static float temp = getTemp();

    menu.update();

    if (isnan(temp)) {
        #if !DEBUG_SERIAL_TEMP
        if (!isError) {
            isError = true;
            lcd.saveContents(stored);
            lcd << pos(0,0) << "Thermocouple";
            lcd << pos(0, 1) << "error nr " << thermocouple.readError();
            lcd.flush();
        }
        #endif
        temp = getTemp();
        delay(50);
        return;
    }

    if (isError) {
        isError = false;
        lcd.restoreContents(stored);
    }

    #if TELEMETRY
    iterData.send();

    benchmark.start().send();
    #endif

    mesh.integrateStep(simulation::dTau, input.r, temp, input);

    #if TELEMETRY
    benchmark.end().send().clear();
    #endif

    if (iterData.step < input.nSteps) {
        iterData.step++;
        iterData.tau += simulation::dTau;
    }
    else {
        lcd << clear << pos(0, 1) << mesh.nodes[0].t;
        lcd << pos(8, 1) << mesh.nodes[meshconfig::nNodes - 1].t;
        lcd << pos(0, 0) << temp;

        lcd.flush();

        iterData.step = 0;
        iterData.tau = 0.f;

        for (auto& node : mesh.nodes)
            node.t = input.t0;

        temp = getTemp();
    }
}
