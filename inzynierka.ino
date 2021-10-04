#include <LiquidCrystal_I2C.h>
#include <print_util.h>
#include "lcd_util.h"

LiquidCrystal_I2C lcd(0x3F, 16, 2);

float getTemp() {
    while (Serial.available() < sizeof(float));

    float res;
    Serial.readBytes((byte*) &res, sizeof(float));
    return res;
}

void setup() {
    Serial.begin(9600);
    lcd.init();
    lcd.backlight();
}

void loop() {
    float temp = getTemp();
    lcd << lcdut::clear << lcdut::pos(0, 0) << "T = " << temp << lcdut::deg << "C";
    Serial << temp << endl;
}
