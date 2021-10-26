#ifndef MENU_HEADER_GUARD
#define MENU_HEADER_GUARD

#include <LiquidCrystal_I2C.h>
#include <InputDebounce.h>
#include <print_util.h>
#include "lcd_util.h"

using namespace lcdut;
using namespace prnt;

constexpr size_t nItems = 4;

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
void swap(T& a, T& b) {
    T tmp = a;
    a = b;
    b = tmp;
}

class Menu {
public:
    struct Item {
        const char* name;
        float* value;
    };

    using updateCb = void (*)();

private:
    Item* items;
    Item* current;
    uint8_t position;
    LiquidCrystal_I2C& output;
    updateCb m_onUpdate = nullptr;

    static constexpr size_t lcdColumns = 16;

    char valueBuffer[lcdColumns + 1];

    static const int8_t maxDigits = 8;

    struct MenuInputDebounce : public InputDebounce {
        Menu& menu;
        unsigned long lastDuration;

        MenuInputDebounce(Menu& menu): menu(menu) {}

    protected:
        virtual void pressed() {
            lastDuration = 0;
            menu.handleButtonPress(*this);
        }

        virtual void pressedDuration(unsigned long duration) {
            if (duration - lastDuration >= 200) {
                lastDuration = duration;
                menu.handleButtonPress(*this);
            }
        }
    };

    MenuInputDebounce setButton;
    MenuInputDebounce leftButton;
    MenuInputDebounce rightButton;
    MenuInputDebounce incButton;

    void handleButtonPress(MenuInputDebounce& input) {
        if (input == setButton)
            handleSetButtonPress();
        else if (input == leftButton)
            handleLeftButtonPress();
        else if (input == rightButton)
            handleRightButtonPress();
        else if (input == incButton)
            handleIncButtonPress();
    }

    void handleSetButtonPress() {
        position = 0;

        if (current) {
            *current->value = atof(valueBuffer);

            if (m_onUpdate)
                m_onUpdate();

            // Serial << current->name << " set to ";
            // Serial.println(*current->value, 8);
        }

        if (!current) {
            current = items;
        } else if (current < items+nItems) {
            current++;
        } else {
            current = nullptr;
            output.clear();
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

    void updateDisplayedItem() {
        output << clear << pos(0, 0) << current->name;
        fillValueBuffer();
        updateDisplayedValue();
    }

    void fillValueBuffer() {
        memset(valueBuffer, ' ', lcdColumns);
        valueBuffer[sizeof(valueBuffer) - 1] = 0;

        auto digits = round(ceil(log10(*current->value)));

        if (digits < 0)
            digits = 0;

        int8_t prec = maxDigits - 1 - digits;

        if (prec < 0)
            prec = 0;

        dtostrf(*current->value, 8, prec, valueBuffer);

        // for (auto c : valueBuffer) {
        //     Serial << (int) c;

        //     if (isPrintable(c))
        //         Serial << " ('" << c << "')";

        //     Serial << ' ';
        // }

        // Serial << endl;
    }

    void updateDisplayedValue() {
        output << lcdut::pos(0, 1) << valueBuffer;
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
    Menu(LiquidCrystal_I2C& lcd, Item items[nItems]):
        output(lcd),
        items(items),
        current(nullptr),
        setButton(*this),
        leftButton(*this),
        rightButton(*this),
        incButton(*this)
    {}

    void setup(
        uint8_t setButtonPin,
        uint8_t leftButtonPin,
        uint8_t rightButtonPin,
        uint8_t incButtonPin
    ) {
        setButton.setup(setButtonPin);
        leftButton.setup(leftButtonPin);
        rightButton.setup(rightButtonPin);
        incButton.setup(incButtonPin);
    }

    void update() {
        output.cursor_on();
        // output.blink_on();
        do {
            setButton.process(millis());
            leftButton.process(millis());
            rightButton.process(millis());
            incButton.process(millis());

            if (current) {
                delay(50);
            }
        } while(current);
        // output.blink_off();
        output.cursor_off();
    }

    void onParamUpdate(updateCb cb) {
        m_onUpdate = cb;
    }
};

#endif
