#ifndef LCD_UTIL_HEADER_GUARD
#define LCD_UTIL_HEADER_GUARD

#include <LiquidCrystal_I2C.h>

namespace lcdut {

    namespace symbols {
        constexpr char deg = 223;
        constexpr char alpha = 0b11100000;
        constexpr char beta = 0b11100010;
    }

    struct pos {
        uint8_t row, col;

        explicit pos(uint8_t col, uint8_t row): col(col), row(row) {}

        friend LiquidCrystal_I2C& operator<<(LiquidCrystal_I2C& lcd, const pos& obj) {
            lcd.setCursor(obj.col, obj.row);
            return lcd;
        }
    };

    LiquidCrystal_I2C& clear(LiquidCrystal_I2C& lcd) {
        lcd.clear();
        return lcd;
    }

    LiquidCrystal_I2C& operator<<(LiquidCrystal_I2C& lcd, LiquidCrystal_I2C& (*modifier)(LiquidCrystal_I2C&) ) {
        modifier(lcd);
        return lcd;
    }
}


#endif
