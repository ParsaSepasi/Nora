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
void runRainbow();
void runEqualize();
void runStatic();
void runBOXRainbow();
void runBOXEqualize();
void runBOXStatic();

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