#include "pins.h"
#include "state.h"

CRGB leds[NUM_LEDS];

String inputString = "";
int customBrightness = 100;

bool equalizer1Active = false;
bool equalizer2Active = false;
bool equalizer3Active = false;
bool boxequalizer1Active = false;
bool boxequalizer2Active = false;
bool boxequalizer3Active = false;

unsigned long relayOnTime = 0;
bool relayActive = false;

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
const int GPIOPins[NUM_PINS] = {BACKLIGHT, READINGLIGHT, PARTY, MUTE, OPEN_BOX, CLOSE_BOX};
const String GPIONames[NUM_PINS] = {"BACKLIGHT", "READINGLIGHT", "PARTY", "MUTE", "OPEN_BOX", "CLOSE_BOX"};

// متغیرهای پردازش سریال
bool stringStart = false;
String inputdata = "";
bool inputdataComplete = false;

// default clock
String currentTime = "00:00:00";