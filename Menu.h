#ifndef MENU_HEADER_GUARD
#define MENU_HEADER_GUARD

#include <LiquidCrystal_I2C.h>
#include <InputDebounce.h>
#include <print_util.h>
#include "debug.h"
#include "limits.h"
#include "lcd_util.h"
#include "BufferedLcd.h"

using namespace lcdut;
using namespace prnt;

namespace {
    bool operator==(const InputDebounce& a, const InputDebounce& b) {
        return a.getPinIn() == b.getPinIn();
    }

    template<typename T>
    void arrayRotateRight(T* arr, size_t n) {
        T tmp = arr[n - 1];

        for (int i = n - 1; i > 0; i--)
            arr[i] = arr[i-1];

        arr[0] = tmp;
    }

    template<typename T>
    void arrayRotateLeft(T* arr, size_t n) {
        T tmp = arr[0];

        for (int i = 0; i < n-1; i++)
            arr[i] = arr[i+1];

        arr[n - 1] = tmp;
    }

    template<typename T>
    void swap(T& a, T& b) {
        T tmp = a;
        a = b;
        b = tmp;
    }
}

enum class MenuItemType : char {
    _uint, _float
};

struct MenuItem {
    const char* name;
    void* value;
    MenuItemType type;
};

// constexpr size_t nItems = 6;
// constexpr uint8_t nLcdCols = 16;
// constexpr uint8_t nLcdRows = 2;

template<size_t nItems, uint8_t nLcdCols, uint8_t nLcdRows>
class Menu {
public:
    using updateCb = void (*)();
    using lcdT = BufferedLcd<nLcdCols, nLcdRows>;

private:
    MenuItem* items;
    MenuItem* current;
    uint8_t position;
    lcdT& output;
    updateCb m_onUpdate = nullptr;

    char savedScreenBuffer[lcdT::size];
    char valueBuffer[lcdT::cols + 1];

    static constexpr int8_t maxFloatDigits = 7;
    static constexpr int8_t maxUintDigits = 5;

    int8_t maxDigits = maxFloatDigits;

    class MenuInputDebounce : public InputDebounce {
        Menu& menu;
        unsigned long lastDuration;

    public:
        MenuInputDebounce(Menu& menu): menu(menu) {}

    protected:
        virtual void pressed() {
            lastDuration = 0;
            menu.handleButtonPress(*this);
        }

        virtual void pressedDuration(unsigned long duration) {
            if (duration - lastDuration >= 200) {
                lastDuration = duration;
                menu.handleButtonPress(*this, true);
            }
        }
    };

    MenuInputDebounce setButton;
    MenuInputDebounce leftButton;
    MenuInputDebounce rightButton;
    MenuInputDebounce incButton;
    MenuInputDebounce decButton;

    void handleButtonPress(MenuInputDebounce& input, bool isRepeated = false) {
        if (input == setButton) {
            // do not go around after exiting menu mode
            // when set button is continiously pressed
            if ((!isRepeated || current))
                handleSetButtonPress();
        }

        if (!current)
            return;

        if (input == leftButton)
            handleLeftButtonPress();
        else if (input == rightButton)
            handleRightButtonPress();
        else if (input == incButton)
            handleIncButtonPress();
        else if (input == decButton)
            handleDecButtonPress();
    }

    void beginMenuMode() {
        output.cursor_on();
        // output.blink_on();
        output.saveContents(savedScreenBuffer);
    }

    void endMenuMode() {
        // output.blink_off();

        if (m_onUpdate)
            m_onUpdate();

        output.cursor_off();
        output.restoreContents(savedScreenBuffer);
    }

    void handleSetButtonPress() {
        position = 0;

        if (current) {
            DBG_Serial(current->name << " set to ");
            switch (current->type) {
                case MenuItemType::_float:
                    *static_cast<float*>(current->value) = atof(valueBuffer);
                    #if DEBUG_PRINTS
                    Serial.println(*static_cast<float*>(current->value), 8);
                    #endif
                    break;

                case MenuItemType::_uint: {
                    auto val = strtoul(valueBuffer, nullptr, 10);

                    if (val > UINT_MAX)
                        val = UINT_MAX;

                    *static_cast<unsigned int*>(current->value) = (unsigned int)val;
                    DBG_Serial(val << endl);
                    break;
                }
            }
        }

        if (!current) {
            beginMenuMode();

            current = items;
        } else if (current < items + nItems - 1) {
            current++;
        } else {
            current = nullptr;

            endMenuMode();
        }

        if (current)
            updateDisplayedItem();
    }

    void handleLeftButtonPress() {
        if (position == 0)
            setPosition(maxDigits);
        else
            decPosition();

        output.setCursor(position, 1);
    }

    void handleRightButtonPress() {
        if (position == maxDigits)
            setPosition(0);
        else
            incPosition();
    }

    void handleIncButtonPress() {
        if (valueBuffer[position] == '.') {
            if (isdigit(valueBuffer[position+1])) {
                // swap the dot with next digit
                swap(valueBuffer[position], valueBuffer[position+1]);
                incPosition();
            } else {
                // move all digits to the right and the dot to front
                arrayRotateRight(valueBuffer, position+1);
                setPosition(0);
            }
        } else if (isdigit(valueBuffer[position])) {
            if (valueBuffer[position] == '9')
                valueBuffer[position] = '0';
            else
                valueBuffer[position]++;
        }

        updateDisplayedValue();
    }

    void handleDecButtonPress() {
        if (valueBuffer[position] == '.') {
            if (isdigit(valueBuffer[position-1])) {
                // swap the dot with prev digit
                swap(valueBuffer[position], valueBuffer[position-1]);
                decPosition();
            } else {
                // move all digits to the left and the dot to end
                arrayRotateLeft(valueBuffer, maxFloatDigits+2);
                setPosition(maxFloatDigits+1);
            }
        } else if (isdigit(valueBuffer[position])) {
            if (valueBuffer[position] == '0')
                valueBuffer[position] = '9';
            else
                valueBuffer[position]--;
        }

        updateDisplayedValue();
    }

    void updateDisplayedItem() {
        output << clear << pos(0, 0) << current->name;
        fillValueBuffer();

        if (current->type == MenuItemType::_uint)
            position = maxDigits;

        updateDisplayedValue();
    }

    void fillValueBuffer() {
        memset(valueBuffer, ' ', lcdT::cols);
        valueBuffer[sizeof(valueBuffer) - 1] = 0;

        switch (current->type) {
            case MenuItemType::_float: {
                maxDigits = maxFloatDigits;
                auto value = *static_cast<float*>(current->value);
                auto digits = round(ceil(log10(value)));

                if (digits < 0)
                    digits = 0;

                int8_t prec = maxFloatDigits - digits;

                if (prec < 0)
                    prec = 0;

                dtostrf(value, maxFloatDigits, prec, valueBuffer);
                break;
            }

            case MenuItemType::_uint: {
                maxDigits = maxUintDigits - 1;
                snprintf(valueBuffer, lcdT::cols, "%05u", *static_cast<unsigned int*>(current->value));
                break;
            }
        }
    }

    void updateDisplayedValue() {
        output << lcdut::pos(0, 1) << valueBuffer;
        output.flush();
        output.setCursor(position, 1);
    }

    void setPosition(uint8_t pos) {
        position = pos;
        output.setCursor(position, 1);
    }

    void incPosition() {
        position++;
        output.setCursor(position, 1);
    }

    void decPosition() {
        position--;
        output.setCursor(position, 1);
    }

public:
    Menu(lcdT& lcd, MenuItem items[nItems]):
        output(lcd),
        items(items),
        current(nullptr),
        setButton(*this),
        leftButton(*this),
        rightButton(*this),
        incButton(*this),
        decButton(*this)
    {}

    void setup(
        uint8_t setButtonPin,
        uint8_t leftButtonPin,
        uint8_t rightButtonPin,
        uint8_t incButtonPin,
        uint8_t decButtonPin
    ) {
        setButton.setup(setButtonPin);
        leftButton.setup(leftButtonPin);
        rightButton.setup(rightButtonPin);
        incButton.setup(incButtonPin);
        decButton.setup(decButtonPin);
    }

    void update() {
        do {
            auto time = millis();

            setButton.process(time);
            leftButton.process(time);
            rightButton.process(time);
            incButton.process(time);
            decButton.process(time);

            if (current) {
                delay(50);
            }
        } while(current);
    }

    void onParamUpdate(updateCb cb) {
        m_onUpdate = cb;
    }
};

#endif
