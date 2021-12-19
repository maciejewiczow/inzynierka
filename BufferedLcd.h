#ifndef BUFFEREDLCD_HEADER_GUARD
#define BUFFEREDLCD_HEADER_GUARD

#include <LiquidCrystal_I2C.h>
#include <Print.h>
#include <print_util.h>
#include "lcd_util.h"
#include "debug.h"

using namespace prnt;
using namespace lcdut;

// constexpr uint8_t nColsT = 16;
// constexpr uint8_t nRowsT = 2;

template<uint8_t nColsT, uint8_t nRowsT>
class BufferedLcd : public LiquidCrystal_I2C {
public:
    static constexpr uint8_t size = nRowsT * nColsT;

private:
    char screenBuffer[size];
    uint8_t row = 0;
    uint8_t col = 0;

    using super = LiquidCrystal_I2C;

    size_t writeToScreen(const uint8_t *buffer, size_t size) {
        DBG_Serial("Screen write called" << endl);
        size_t n = 0;

        while (size--) {
            if (super::write(*buffer++))
                n++;
            else
                break;
        }

        return n;
    }

    void flushNoCursorReset() {
        DBG_Serial(DBG_HEADER() << "Flushing the screen buffer:" << endl);

        for (uint8_t i = 0; i < nRowsT; i++) {
            super::setCursor(0, i);

            auto start = screenBuffer + i*nColsT;

            writeToScreen((const uint8_t *)start, nColsT);

            DBG_Serial("line " << i << " (string): " << start << endl);
            DBG_Serial("line " << i << " (ints):   ");

            #if DEBUG_PRINTS
            for (auto j = start; j < start + nColsT; j++)
                DBG_Serial((uint8_t) *start << ' ');
            #endif

            DBG_Serial(endl);
        }
    }

public:
    BufferedLcd(uint8_t lcdAddr): super(lcdAddr, nColsT, nRowsT) {
        clear();
    }

    virtual void clear() {
        memset(screenBuffer, ' ', size);
    }

    static constexpr uint8_t cols = nColsT;
    static constexpr uint8_t rows = nRowsT;

    void saveContents(char* dest) {
        memcpy(dest, screenBuffer, size);
    }

    void restoreContents(const char* src) {
        memcpy(screenBuffer, src, size);
        flush();
    }

    void flush() {
        flushNoCursorReset();
        super::setCursor(col, row);
    }

    virtual void setCursor(uint8_t col, uint8_t row) {
        DBG_Serial(DBG_HEADER() << "Cursor set to " << col << ", " << row << endl);
        this->row = row;
        this->col = col;
        super::setCursor(col, row);
    }

    virtual size_t write(uint8_t val) {
        if (row < nRowsT && col < nColsT) {
            auto c = (char) val;

            if (isPrintable(c))
                DBG_Serial(c);
            else
                DBG_Serial(val);

            // DBG_Serial(" at (" << row << ", " << col << ") ");
            DBG_Serial(' ');

            screenBuffer[row*nColsT + col] = val;
            col++;
        } else {
            flushNoCursorReset();
            super::write(val);
        }

        return 1;
    }

    virtual size_t write(const uint8_t* buffer, size_t size) {
        DBG_Serial("Buffer write called" << endl);
        size_t n = 0;

        while (size--) {
            if (write(*buffer++))
                n++;
            else
                break;
        }

        return n;
    }
};

#endif
