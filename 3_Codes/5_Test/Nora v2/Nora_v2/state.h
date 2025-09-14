#pragma once
#include <Arduino.h>
#include <FastLED.h>

//#define HEADER_NORA "NORA"
//#define HEADER_LENGTH 4

#define DEBUG_SERIAL 1

// LED آرایه
extern CRGB leds[NUM_LEDS];

extern String inputString;
extern int customBrightness;

extern bool equalizer1Active;
extern bool equalizer2Active;
extern bool equalizer3Active;

extern bool boxequalizer1Active;
extern bool boxequalizer2Active;
extern bool boxequalizer3Active;

extern unsigned long relayOnTime;
extern bool relayActive;

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