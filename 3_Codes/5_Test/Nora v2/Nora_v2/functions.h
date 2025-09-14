/*#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#pragma once
#include <Arduino.h>

extern uint8_t crc8(const uint8_t *data, uint8_t len);

void GPIO(int GPIOIndex, bool state);
void handleSerialCommand(String command);
void parseRGBCommand(String rgbString);

void runEqualizer1();
void runEqualizer2();
void runEqualizer3();

void runBOXEqualizer1();
void runBOXEqualizer2();
void runBOXEqualizer3();

void sound_system_off();
void normal_mode();
void party_mode();

void open_box();
void close_box();

void run_led_wake_word();

#endif*/

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#pragma once
#include <Arduino.h>
#include <FastLED.h>

// External declarations for helper functions
extern uint8_t crc8(const uint8_t *data, uint8_t len);
extern String toLowerCaseString(String str);
extern CRGB hexToCRGB(String hexStr);

// GPIO and commands
void GPIO(int GPIOIndex, bool state);
void handleSerialCommand(String command);
void parseRGBCommand(String rgbString);

// Equalizer functions
void runEqualizer1();
void runEqualizer2();
void runEqualizer3();
void runBOXEqualizer1();
void runBOXEqualizer2();
void runBOXEqualizer3();

// Modes
void sound_system_off();
void normal_mode();
void party_mode();

// Box
void open_box();
void close_box();

// Wake word
void run_led_wake_word();

// Reading Light
void readingLight(bool state);

// Back Light
void backLight(bool state);

#endif