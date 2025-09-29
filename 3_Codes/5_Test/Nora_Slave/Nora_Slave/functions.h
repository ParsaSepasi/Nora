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

// Equalizer functions
void runRainbow();
//void runEqualize();
void runStatic();

#endif