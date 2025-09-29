#pragma once
#include <Arduino.h>

// #define NUM_PINS     6
// #define BACKLIGHT    15
// #define READINGLIGHT 2
// #define OPEN_BOX     4
// #define CLOSE_BOX    5
// #define MUTE         18
// #define PARTY        19

// LED
//#define LED_PIN      21
#define BOX_PIN      7
//#define NUM_LEDS     98
#define NUM_BOX_LEDS 15
#define LED_TYPE     WS2811
#define COLOR_ORDER  GRB

// میکروفون (EQ3)
//#define MIC_PIN      32

// MAX7219 (برای نمایشگر ساعت)
#define CLK_PIN      1  // پین CLK برای MAX7219
#define MAX7219_DATA_PIN 0  // پین DIN (renamed to avoid FastLED conflict)
#define CS_PIN       3  // پین CS
#define MAX_DEVICES  1   // تعداد ماژول‌های MAX7219