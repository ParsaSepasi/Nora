#pragma once
#include <Arduino.h>
#include <FastLED.h>

#define DEBUG_SERIAL 1

// LED آرایه‌ها
extern CRGB leds[NUM_LEDS];
extern CRGB box_leds[NUM_BOX_LEDS];

extern String inputString;
extern int customBrightness;

extern bool RainbowActive;
extern bool EqualizeActive;
extern bool StaticActive;

extern bool boxRainbowActive;
extern bool boxEqualizeActive;
extern bool boxStaticActive;

extern String ledComponent;
extern String ledMode; // برای magicl (GPIO 21)
extern String boxLedMode; // برای magicbl (GPIO 22)
extern String ledColor;
extern int brightnessLevel;

// Relay
extern unsigned long relayOnTime;
extern bool relayActive;
extern bool soundBoost;
extern bool boxOpen;

// EQ1
extern CRGBPalette16 currentPalette;
extern TBlendType currentBlending;

// EQ2
extern int customR, customG, customB;

// EQ3
extern float smoothedLevel;
extern int dynamicMin;
extern int dynamicMax;
extern bool dynamicRangeValid;
extern unsigned long lastCalibrate;
extern uint8_t colorIndex;

// پین‌ها
extern const int GPIOPins[NUM_PINS];
extern const String GPIONames[NUM_PINS];

// متغیرهای پردازش سریال
extern bool stringStart;
extern String inputdata;
extern bool inputdataComplete;

// clock
extern String currentTime;
extern String clockTime;
extern bool readingLightOn;
extern bool backLightOn;