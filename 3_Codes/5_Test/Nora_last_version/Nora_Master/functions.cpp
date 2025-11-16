#include <FastLED.h>
#include "pins.h"
#include "state.h"
#include "functions.h"

#if DEBUG_SERIAL
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif


extern CRGB leds[];
enum WakeCmd : uint8_t { WAKE_NOP = 0,
                         WAKE_START = 1,
                         WAKE_ABORT = 2 };

void run_led_wake_word(WakeCmd cmd = WAKE_NOP, uint8_t sections = 4, uint16_t stepMs = 1000) {
  static bool active = false;
  static uint8_t stage = 0;
  static uint8_t S = 4;
  static uint16_t step = 1000;
  static int per = 0;
  static uint32_t nextMs = 0;
  const CRGB wakeColor = CRGB(0, 255, 255);

  // فرمان‌ها
  if (cmd == WAKE_ABORT) {
    active = false;
    return;
  }
  if (cmd == WAKE_START) {
    S = sections;
    step = stepMs;
    per = (NUM_LEDS + S - 1) / S;
    stage = 0;
    active = true;
    fill_solid(leds, NUM_LEDS, wakeColor);
    FastLED.show();
    nextMs = millis() + step;
    return;
  }

  // جلو بردن انیمیشن بدون delay
  if (!active) return;
  if (millis() < nextMs) return;

  if (stage < S) {
    int start = stage * per;
    int end = min(NUM_LEDS, start + per);
    for (int i = start; i < end; ++i) leds[i] = CRGB::Black;
    FastLED.show();
    stage++;
    nextMs = millis() + step;
  } else {
    active = false;
  }
}

// CRC-8 function
uint8_t crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x80) crc = (crc << 1) ^ 0x07;
      else crc <<= 1;
    }
  }
  return crc;
}

// Helper function for lowercase conversion
String toLowerCaseString(String str) {
  String result = str;
  for (int i = 0; i < result.length(); i++) {
    if (result[i] >= 'A' && result[i] <= 'Z') {
      result[i] = result[i] + 32;
    }
  }
  return result;
}

// Hex to CRGB conversion
CRGB hexToCRGB(String hexStr) {
  if (hexStr.startsWith("#")) hexStr = hexStr.substring(1);
  if (hexStr.length() == 6) {
    int r = strtol(hexStr.substring(0, 2).c_str(), NULL, 16);
    int g = strtol(hexStr.substring(2, 4).c_str(), NULL, 16);
    int b = strtol(hexStr.substring(4, 6).c_str(), NULL, 16);
    return CRGB(r, g, b);
  }
  return CRGB::Red;
}

// ------------------- GPIO -------------------
void GPIO(int GPIOIndex, bool state) {
  if (GPIOIndex >= 0 && GPIOIndex < NUM_PINS) {
    int pin = GPIOPins[GPIOIndex];
    digitalWrite(pin, state ? HIGH : LOW);
    if (state) {
      Serial.print(F("ON: "));
      Serial.print(GPIONames[GPIOIndex]);
      Serial.print(F(" on GPIO "));
      Serial.println(pin);
    } else {
      Serial.print(F("OFF: "));
      Serial.print(GPIONames[GPIOIndex]);
      Serial.print(F(" on GPIO "));
      Serial.println(pin);
    }
  } else {
    Serial.print(F("Invalid GPIO index: "));
    Serial.println(GPIOIndex);
  }
}

// ------------------- Serial Commands -------------------
void handleSerialCommand(String command) {
#if DEBUG_SERIAL
  Serial.print(F("Processing command: '"));
  Serial.print(command);
  Serial.println(F("'"));
#endif

  command.toUpperCase();
  command.trim();

  // Check CRC if present
  int crcPos = command.lastIndexOf('_');
  if (crcPos > 0 && (command.length() - crcPos - 1) == 2) {
    String crcStr = command.substring(crcPos + 1);
    bool isValidHex = true;
    for (int i = 0; i < crcStr.length(); i++) {
      char c = crcStr[i];
      if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
        isValidHex = false;
        break;
      }
    }
    if (isValidHex) {
      String payload = command.substring(0, crcPos);
      uint8_t expectedCrc = strtol(crcStr.c_str(), NULL, 16);
      uint8_t calcCrc = crc8((uint8_t *)payload.c_str(), payload.length());
      if (calcCrc != expectedCrc) {
        Serial.println(F("CRC Mismatch!"));
        return;
      }
      Serial.println(F("CRC OK"));
      command = payload;
    }
  }

  String parts[10];
  int partCount = 0;
  int lastIndex = 0;
  for (int i = 0; i < command.length() && partCount < 10; i++) {
    if (command[i] == '_') {
      parts[partCount] = command.substring(lastIndex, i);
      parts[partCount].trim();
      lastIndex = i + 1;
      partCount++;
    }
  }
  if (lastIndex < command.length()) {
    parts[partCount] = command.substring(lastIndex);
    parts[partCount].trim();
    partCount++;
  }

#if DEBUG_SERIAL
  Serial.print(F("Parsed parts: "));
  for (int i = 0; i < partCount; i++) {
    Serial.print(parts[i]);
    Serial.print(F(" "));
  }
  Serial.println();
#endif

  if (partCount < 2) {
    Serial.println(F("Invalid command format"));
    return;
  }

  String component = toLowerCaseString(parts[0]);
  String action = toLowerCaseString(parts[1]);
  String parameter = (partCount >= 3) ? toLowerCaseString(parts[2]) : "";

  if (component == "magicl") {
    Serial.println(F("Processing magicl locally in Master..."));
    ledComponent = "magicl";

    if (action == "mode") {
      if (parameter == "off") {
        ledMode = "off";
        RainbowActive = false;
        EqualizeActive = false;
        StaticActive = false;
        WakeActive = false;
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
        Serial.println(F("magicl Off in Master (GPIO 21)"));
      } else if (parameter == "rainbow") {
        ledMode = "rainbow";
        RainbowActive = true;
        EqualizeActive = false;
        StaticActive = false;
        WakeActive = false;
        Serial.println(F("magicl Rainbow On in Master (GPIO 21)"));
      } else if (parameter == "equalize") {
        ledMode = "equalize";
        RainbowActive = false;
        EqualizeActive = true;
        StaticActive = false;
        WakeActive = false;
        Serial.println(F("magicl Equalize On in Master (GPIO 21)"));
      } else if (parameter == "static") {
        if (partCount >= 4) {
          ledColor = parts[3];
          ledMode = "static";
          RainbowActive = false;
          EqualizeActive = false;
          StaticActive = true;
          WakeActive = false;

          Serial.print(F("magicl Static On in Master (GPIO 21) - Color: "));
          Serial.println(ledColor);
        } else {
          Serial.println(F("Static requires color!"));
        }
      } else if (parameter == "wakeup") {
        ledMode = "wakeup";
        RainbowActive = false;
        EqualizeActive = false;
        StaticActive = false;
        WakeActive = true;
        Serial.println(F("magicl Wakeup On in Master (GPIO 21)"));
        wake_start(4, 1000);
      } else {
        Serial.println(F("Unknown magicl mode"));
      }
    } else if (action == "brightness") {
      if (partCount >= 3) {
        String levelStr = parts[2];
        levelStr.toLowerCase();  // برای پذیرش LOW / Low / lOw و ...

        int level = -1;  // مقدار نامعتبر (نگهبان)

        if (levelStr == "low") level = 0;
        else if (levelStr == "mid" || levelStr == "medium") level = 1;
        else if (levelStr == "high") level = 2;

        if (level != -1) {
          brightnessLevel = level;
          Serial.print(F("magicl Brightness set to: "));
          Serial.println(levelStr);
        } else {
          Serial.println(F("Invalid brightness level (low / mid / high)"));
        }
      } else {
        Serial.println(F("Missing brightness parameter"));
      }
    } else {
      Serial.println(F("Unknown magicl action"));
    }
  }

  else if (component == "magicbl") {
    Serial.println(F("Processing magicbl locally in Master..."));
    ledComponent = "magicbl";

    if (action == "mode") {
      if (parameter == "off") {
        boxRainbowActive = false;
        boxEqualizeActive = false;
        boxStaticActive = false;
        fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Black);
        FastLED.show();
        Serial.println(F("magicbl Off in Master (GPIO 22)"));
      } else if (parameter == "rainbow") {
        boxRainbowActive = true;
        boxEqualizeActive = false;
        boxStaticActive = false;
        Serial.println(F("magicbl Rainbow On in Master (GPIO 22)"));
      } else if (parameter == "equalize") {
        boxRainbowActive = false;
        boxEqualizeActive = true;
        boxStaticActive = false;
        Serial.println(F("magicbl Equalize On in Master (GPIO 22)"));
      } else if (parameter == "static") {
        if (partCount >= 4) {
          boxledColor = parts[3];
          boxRainbowActive = false;
          boxEqualizeActive = false;
          boxStaticActive = true;
          Serial.print(F("magicbl Static On in Master (GPIO 22) - Color: "));
          Serial.println(ledColor);
        } else {
          Serial.println(F("Static requires color!"));
        }
      } else {
        Serial.println(F("Unknown magicbl mode"));
      }
    } else if (action == "brightness") {
      if (partCount >= 3) {
        String levelStr = parts[2];
        levelStr.toLowerCase();

        int level = -1;

        if (levelStr == "low") level = 0;
        else if (levelStr == "mid" || levelStr == "medium") level = 1;
        else if (levelStr == "high") level = 2;

        if (level != -1) {
          brightnessLevel = level;
          Serial.print(F("magicbl Brightness set to: "));
          Serial.println(levelStr);
        } else {
          Serial.println(F("Invalid brightness level (low / mid / high)"));
        }
      } else {
        Serial.println(F("Missing brightness parameter"));
      }
    } else {
      Serial.println(F("Unknown magicl action"));
    }
  } else {
    Serial.println(F("Unknown component"));
  }
  if (component == "clock") {
    if (partCount >= 3 && parts[1] == "TIME") {
      clockTime = parts[2];
      if (clockTime.length() == 8 && clockTime[2] == ':' && clockTime[5] == ':') {
        Serial.print(F("🕒 Clock Time: "));
        Serial.println(clockTime);
      } else {
        Serial.println(F("❌ Invalid time format"));
      }
    }
  } else if (component == "sound") {
    if (parts[1] == "ON") {
      normal_mode();
      Serial.println(F("🔊 Sound ON"));
    } else if (parts[1] == "OFF") {
      sound_system_off();
      Serial.println(F("🔇 Sound OFF"));
    } else if (parts[1] == "BOOST") {
      soundBoost = true;
      party_mode();
      Serial.println(F("🚀 Sound Boost"));
    }
  } else if (component == "box") {
    if (parts[1] == "OPEN") {
      open_box();
      boxOpen = true;
    } else if (parts[1] == "CLOSE") {
      close_box();
      boxOpen = false;
    }
    Serial.print(F("📦 Box: "));
    Serial.println(parts[1]);
  } else if (component == "readingl") {
    if (parts[1] == "ON") {
      Serial.println(F("Executing readinglight ON"));
      readingLight(true);
      readingLightOn = true;
    } else if (parts[1] == "OFF") {
      Serial.println(F("Executing readinglight OFF"));
      readingLight(false);
      readingLightOn = false;
    }
    Serial.print(F("🔦 Reading Light: "));
    Serial.println(parts[1]);
  } else if (component == "backl") {
    if (parts[1] == "ON") {
      Serial.println(F("Executing backlight ON"));
      backLight(true);
      backLightOn = true;
    } else if (parts[1] == "OFF") {
      Serial.println(F("Executing backlight OFF"));
      backLight(false);
      backLightOn = false;
    }
    Serial.print(F("🔦 Back Light: "));
    Serial.println(parts[1]);
  } else {
    Serial.print(F("❌ Unknown component: "));
    Serial.println(component);
  }
}


// ------------------- Equalizer Functions -------------------
void runRainbow() {
  //Serial.println(F("Running Rainbow (GPIO 21)"));
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, startIndex + i * 255 / NUM_LEDS, 255, currentBlending);
  }
}


// void runEqualize() {
//   uint8_t level = readEnvelope();

//   uint8_t numLedsOn = map(level, 30, 180, 0, NUM_LEDS);
//   numLedsOn = constrain(numLedsOn, 0, NUM_LEDS);

//   static uint8_t smoothLeds = 0;
//   smoothLeds = (smoothLeds * 2 + numLedsOn) / 3;

//   uint8_t hue = map(smoothLeds, 0, NUM_LEDS, 0, 170);

//   for (int i = 0; i < NUM_LEDS; i++) {
//     if (i < smoothLeds) {
//       leds[i] = CHSV(hue, 255, 255);
//     } else {
//       leds[i] = CRGB::Black;
//     }
//   }

//   FastLED.setBrightness(brightnessLevel * 85 + 50);
//   FastLED.show();
// }
void runEqualize() {
  uint8_t level = readEnvelope();

  // دیباگ
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 200) {
    Serial.printf("Level: %3d → LEDs: ", level);
    lastPrint = millis();
  }

  uint8_t numLedsOn = map(level, 0, 255, 0, NUM_LEDS);
  numLedsOn = constrain(numLedsOn, 0, NUM_LEDS);

  static uint8_t smoothLeds = 0;
  smoothLeds = (smoothLeds * 2 + numLedsOn) / 3;

  uint8_t hue = map(smoothLeds, 0, NUM_LEDS, 0, 170);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < smoothLeds) {
      leds[i] = CHSV(hue, 255, 255);  // FastLED خودش تبدیل می‌کنه
    } else {
      leds[i] = CRGB::Black;
    }
  }
  FastLED.setBrightness(brightnessLevel * 85 + 50);
  FastLED.show();

  if (millis() - lastPrint > 0) {
    Serial.println(smoothLeds);
  }
}

void runStatic() {
  CRGB color = hexToCRGB(ledColor);
  fill_solid(leds, NUM_LEDS, color);
}

void runBOXRainbow() {
  static uint8_t hue = 0;
  fill_rainbow(box_leds, NUM_BOX_LEDS, hue, 7);
  hue += 2;
}

void runBOXStatic() {
  CRGB color = hexToCRGB(boxledColor);
  fill_solid(box_leds, NUM_BOX_LEDS, color);
}

// ------------------- Modes -------------------
void sound_system_off() {
  digitalWrite(MUTE, LOW);
  digitalWrite(PARTY, LOW);
  Serial.println(F("Sound System OFF (Mute + PartyOff)"));
}

void normal_mode() {
  digitalWrite(MUTE, HIGH);
  digitalWrite(PARTY, LOW);
  Serial.println(F("Normal Mode (Unmute + PartyOff)"));
}

void party_mode() {
  digitalWrite(MUTE, HIGH);
  digitalWrite(PARTY, HIGH);
  Serial.println(F("Party Mode (Unmute + PartyOn)"));
}

// ------------------- Box -------------------
void open_box() {
  digitalWrite(OPEN_BOX, HIGH);
  relayOnTime = millis();
  relayActive = true;
  Serial.println(F("box opened"));
}

void close_box() {
  digitalWrite(CLOSE_BOX, HIGH);
  relayOnTime = millis();
  relayActive = true;
  Serial.println(F("box closed"));
}

// TODO: Implement parseRGBCommand if needed
void parseRGBCommand(String rgbString) {
  Serial.print(F("RGB command parsed: "));
  Serial.println(rgbString);
}

// ------------------- Reading Light -------------------
void readingLight(bool state) {
  GPIO(1, state);
  if (state) {
    Serial.println(F("Reading Light ON"));
  } else {
    Serial.println(F("Reading Light OFF"));
  }
}

void backLight(bool state) {
  GPIO(0, state);
  if (state) {
    Serial.println(F("Back Light ON"));
  } else {
    Serial.println(F("Back Light OFF"));
  }
}


bool pulse1 = true;
unsigned long timepulse1 = 0;

bool pulse0 = false;
unsigned long timepulse0 = 0;

void pulse_option1() {

  if (pulse1 == true && millis() - timepulse1 >= 20000) {
    pulse1 = false;
    pulse0 = true;
    Serial.println("pulse_option1");
    digitalWrite(ADKEY, HIGH);
    timepulse0 = millis();
  }
}
void pulse_option0() {

  if (pulse0 == true && millis() - timepulse0 >= 100) {
    pulse0 = false;
    Serial.println("pulse_option0");
    digitalWrite(ADKEY, LOW);
  }
}

// --------- Audio ADC config + Envelope ---------
#ifndef ADC_ATTEN_DB_11
#define ADC_ATTEN_DB_11 ADC_11db
#endif

static const float ENV_ALPHA = 0.15f;
static const float DC_ALPHA = 0.01f;
static float envSmooth = 0.0f;
static float dcSlow = 2048.0f;

void setup_audio_adc() {
  analogReadResolution(12);
  analogSetPinAttenuation(MIC_PIN, ADC_ATTEN_DB_11);
  // Warm-up
  for (int i = 0; i < 8; ++i) (void)analogRead(MIC_PIN);
}

// uint8_t readEnvelope() {
//   const int N = 16;
//   int sum = 0;
//   for (int i = 0; i < N; ++i) sum += analogRead(MIC_PIN);
//   int sample = sum / N;

//   dcSlow = (1.0f - DC_ALPHA) * dcSlow + DC_ALPHA * sample;
//   int env = abs(sample - (int)dcSlow);
//   envSmooth = (1.0f - ENV_ALPHA) * envSmooth + ENV_ALPHA * (float)env;

//   const int ENV_MIN = 1;
//   const int ENV_MAX = 49;

//   int clamped = constrain((int)envSmooth, ENV_MAX, ENV_MIN);
//   return (uint8_t)map(clamped, ENV_MAX, ENV_MIN, 0, 255);
// }
uint8_t readEnvelope() {
  const int N = 16;
  int sum = 0;
  for (int i = 0; i < N; ++i) sum += analogRead(MIC_PIN);
  int sample = sum / N;

  dcSlow = (1.0f - DC_ALPHA) * dcSlow + DC_ALPHA * sample;
  int env = abs(sample - (int)dcSlow);
  envSmooth = (1.0f - ENV_ALPHA) * envSmooth + ENV_ALPHA * (float)env;

  const int ENV_MIN = 1;   // صدای زیاد → عددهای نزدیک 1
  const int ENV_MAX = 49;  // سکوت → عددهای نزدیک 49

  // ✅ این‌بار درست:
  int clamped = constrain((int)envSmooth, ENV_MIN, ENV_MAX);

  // حالا برعکس نگاشت می‌کنیم:
  //  ENV_MIN(≈1)  → 255   (صدای زیاد → level بالا)
  //  ENV_MAX(≈49) → 0     (سکوت → level پایین)
  int inverted = map(clamped, ENV_MIN, ENV_MAX, 255, 0);

  return (uint8_t)inverted;
}


void wake_start(uint8_t sections, uint16_t stepMs) {
  run_led_wake_word(WAKE_START, sections, stepMs);
}

void wake_update(uint16_t) {
  run_led_wake_word(WAKE_NOP);
}

void wake_abort() {
  run_led_wake_word(WAKE_ABORT);
}