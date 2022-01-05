#ifndef CONFIG_HEADER_GUARD
#define CONFIG_HEADER_GUARD

/* This file contains all macros that alter the compilation of the software */

// misc
#define SERIAL_BAUD 57600

// Interface button pins
#define SET_BUTTON_PIN 7
#define LEFT_BUTTON_PIN 8
#define RIGHT_BUTTON_PIN 9
#define INCREMENT_BUTTON_PIN 10
#define DECREMENT_BUTTON_PIN 11

// EEPROM
#define EEPROM_INPUT_PARAMS_ADDR 0
#define EEPROM_READ_INDICATOR_VAL 21

// LCD
#define LCD_I2C_ADDR 0x3F
#define LCD_COLS 16
#define LCD_ROWS 2

// MAX31855 pins
#define TEMP_DO_PIN 4
#define TEMP_CS_PIN 3
#define TEMP_CLK_PIN 2

// serial communication
#define DEBUG_SERIAL_TEMP false
#define TELEMETRY false

// number of mesh elements
#define MESH_SIZE 10

// debug
#define DEBUG_PRINTS false

#endif
