#include <FastLED.h>
#include "pins.h"
#include "state.h"
#include "functions.h"

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(GPIOPins[i], OUTPUT);
    digitalWrite(GPIOPins[i], LOW);
  }
  inputString.reserve(100);  // Reserve memory for inputString
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(customBrightness);
  FastLED.clear(true);

  normal_mode();
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
}

void loop() {
  serialEvent();  // Manual call for ESP32 (not serialEventRun)

  if (inputdataComplete) {
    handleSerialCommand(inputdata);
    if (ledMode == "equalize" && equalizer1Active) {
      runEqualizer1();  // Assume this exists
    } else if (ledMode != "off") {
      if (ledMode == "rainbow") {
        currentPalette = RainbowColors_p;
      } else if (ledMode == "static") {
        CRGB color = hexToCRGB(ledColor);
        fill_solid(leds, NUM_LEDS, color);
      }
      FastLED.setBrightness(brightnessLevel * 85 + 50);
      FastLED.show();
    }
    inputdataComplete = false;
  } else if (relayActive && millis() - relayOnTime >= 8000) {
    digitalWrite(OPEN_BOX, LOW);
    digitalWrite(CLOSE_BOX, LOW);
    relayActive = false;
    Serial.println(F("Relay auto off after 8s"));
  } else if (equalizer1Active) {
    runEqualizer1();
  } else if (equalizer2Active) {
    runEqualizer2();
  } else if (equalizer3Active) {
    runEqualizer3();
  }
}


void serialEvent() {
  static const char header[] = "NORA_";  // Expected header with space
  static const int HEADER_LENGTH = 5;    // Length of "NORA "
  static int headerIndex = 0;            // Tracks current position in header
  static bool headerComplete = false;    // Tracks if header is fully matched

  while (Serial.available()) {
    char inChar = Serial.read();
    #if DEBUG_SERIAL
      Serial.print(F("Received char: '"));
      Serial.print(inChar);
      Serial.println(F("'"));
    #endif

    // If header is not yet complete, check for header characters
    if (!headerComplete) {
      // Convert input character to uppercase for case-insensitive comparison (except for space)
      char inCharUpper = (headerIndex < 4) ? toupper(inChar) : inChar;

      // Check if current character matches expected header character
      if (headerIndex < HEADER_LENGTH && inCharUpper == header[headerIndex]) {
        inputString += inChar;
        headerIndex++;
        #if DEBUG_SERIAL
          Serial.print(F("Header match at position "));
          Serial.print(headerIndex);
          Serial.print(F(": '"));
          Serial.print(inChar);
          Serial.println(F("'"));
        #endif

        // If full header is matched
        if (headerIndex == HEADER_LENGTH) {
          #if DEBUG_SERIAL
            Serial.println(F("Valid header 'NORA ' detected!"));
          #endif
          headerComplete = true; // Start collecting data
        }
      } else {
        // If header matching fails, reset but check for a new 'N'
        if (headerIndex > 0) {
          #if DEBUG_SERIAL
            Serial.print(F("Invalid header character at position "));
            Serial.print(headerIndex + 1);
            Serial.print(F(": expected '"));
            Serial.print(header[headerIndex]);
            Serial.print(F("', got '"));
            Serial.print(inChar);
            Serial.println(F("'"));
          #endif
          // Slide back to check for a new 'N'
          if (toupper(inChar) == 'N') {
            inputString = "N";
            headerIndex = 1;
          } else {
            inputString = "";
            headerIndex = 0;
          }
        } else if (toupper(inChar) == 'N') {
          // Start of a potential new header
          inputString = "N";
          headerIndex = 1;
          #if DEBUG_SERIAL
            Serial.println(F("Potential new header started with 'N'"));
          #endif
        } else {
          // Ignore non-'N' characters when not matching header
          #if DEBUG_SERIAL
            Serial.print(F("Ignoring non-header char: '"));
            Serial.print(inChar);
            Serial.println(F("'"));
          #endif
        }
      }
    } else {
      // Header is complete, collect data
      inputString += inChar;
      // Check for end of command
      if (inChar == '\n' || inChar == '\r') {
        inputdata = inputString.substring(HEADER_LENGTH);
        inputdata.trim();
        #if DEBUG_SERIAL
          Serial.print(F("Valid data after NORA: '"));
          Serial.print(inputdata);
          Serial.println(F("'"));
        #endif
        // Validate command length
        if (inputdata.length() > 0 && inputdata.length() <= 50) {
          inputdataComplete = true;
        } else {
          #if DEBUG_SERIAL
            Serial.println(F("Invalid command length after NORA"));
          #endif
        }
        inputString = "";
        headerIndex = 0;
        headerComplete = false; // Reset for next command
      }
    }

    // Prevent buffer overflow
    if (inputString.length() > 40) {
      #if DEBUG_SERIAL
        Serial.println(F("Input buffer overflow - clearing!"));
      #endif
      inputString = "";
      headerIndex = 0;
      headerComplete = false;
    }
  }
}