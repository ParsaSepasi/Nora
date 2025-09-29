#pragma once
#include <Arduino.h>
#include <FastLED.h>

#define DEBUG_SERIAL 0  // غیرفعال کردن سریال برای جلوگیری از تداخل

// LED آرایه‌ها
extern CRGB box_leds[NUM_BOX_LEDS];

extern String inputString;
extern int customBrightness;

extern bool RainbowActive;
//extern bool EqualizeActive;
extern bool StaticActive;

extern String ledComponent;
extern String ledMode;
extern String ledColor;
extern int brightnessLevel;

// EQ1
extern CRGBPalette16 currentPalette;
extern TBlendType currentBlending;

// EQ2
extern int customR, customG, customB;

// EQ3
// extern float smoothedLevel;
// extern int dynamicMin;
// extern int dynamicMax;
// extern bool dynamicRangeValid;
// extern unsigned long lastCalibrate;
// extern uint8_t colorIndex;

// متغیرهای پردازش سریال
extern bool stringStart;
extern String inputdata;
extern bool inputdataComplete;

// clock
extern String currentTime;
extern String clockTime;