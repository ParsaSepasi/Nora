#include "pins.h"
#include "state.h"

CRGB box_leds[NUM_BOX_LEDS];

// LED States
String ledComponent = "magicbl";
String ledMode = "off";
String ledColor = "#FF0000";
int brightnessLevel = 1;

String clockTime = "00:00:00";

String inputString = "";
int customBrightness = 100;

bool RainbowActive = false;
//bool EqualizeActive = false;
bool StaticActive = false;

// EQ1
CRGBPalette16 currentPalette;
TBlendType currentBlending;

// EQ2
int customR = 255, customG = 50, customB = 0;

// EQ3
float smoothedLevel = 0;
int dynamicMin = 4095;
int dynamicMax = 0;
bool dynamicRangeValid = false;
unsigned long lastCalibrate = 0;
uint8_t colorIndex = 0;

// پین‌ها
// const int GPIOPins[NUM_PINS] = {BACKLIGHT, READINGLIGHT, PARTY, MUTE, OPEN_BOX, CLOSE_BOX};
// const String GPIONames[NUM_PINS] = {"BACKLIGHT", "READINGLIGHT", "PARTY", "MUTE", "OPEN_BOX", "CLOSE_BOX"};

// متغیرهای پردازش سریال
bool stringStart = false;
String inputdata = "";
bool inputdataComplete = false;

// default clock
String currentTime = "00:00:00";