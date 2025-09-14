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
  serialEvent();  // Manual call for ESP32

  if (inputdataComplete) {
    String command = inputdata;
    //command.trim();
    #if DEBUG_SERIAL
      Serial.print(F("Processing command: "));
      Serial.println(command);
    #endif
    handleSerialCommand(command);
    inputdata = "";
    inputdataComplete = false;
  } else if (Serial.available()) {
    char c = Serial.read();
    #if DEBUG_SERIAL
      Serial.print(F("Received char: "));
      Serial.println(c);
    #endif
    /*if (c == 'R') {
      open_box();
      close_box();
    }*/
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

// void serialEvent() {
//   while (Serial.available()) {
//     char inChar = Serial.read();
//     inputString += inChar;

//     #if DEBUG_SERIAL
//       Serial.print(F("Received char: '"));
//       Serial.print(inChar);
//       Serial.println(F("'"));
//     #endif

//     if (inputString.length() < HEADER_LENGTH) {
//       continue;  // Header incomplete, wait for more
//     }

//     String header = inputString.substring(0, HEADER_LENGTH);
//     if (header == HEADER_NORA) {
//       #if DEBUG_SERIAL
//         Serial.println(F("Valid header 'NORA ' detected!"));
//       #endif

//       inputdata = inputString.substring(HEADER_LENGTH);
//       inputdata.trim();

//       if (inputString.endsWith("\n") || inputString.endsWith("\r")) {
//         #if DEBUG_SERIAL
//           Serial.print(F("Valid data after NORA: '"));
//           Serial.print(inputdata);
//           Serial.println(F("'"));
//         #endif
//         inputdataComplete = true;
//         inputString = "";
//         // while (Serial.available()) {
//         //   Serial.read();
//       }else {
//         #if DEBUG_SERIAL
//           Serial.println(F("Waiting for end of line (\\n or \\r)..."));
//         #endif
//       }
//     } else {
//       // Split the print to avoid F() + String concatenation
//       Serial.print(F("Not valid header! Expected 'NORA ', got: '"));
//       Serial.print(header);
//       Serial.println(F("'"));
//       inputString = "";
//       /*while (Serial.available()) {
//         char discarded = Serial.read();
//       }*/
//     }

//     // Prevent buffer overflow
//     if (inputString.length() > 30) {
//       #if DEBUG_SERIAL
//         Serial.println(F("Input buffer overflow - clearing!"));
//       #endif
//       inputString = "";
//       while (Serial.available()) {
//         Serial.read();
//       }
//     }
//   }
// }

void serialEvent() {
  static const char header[] = "NORA ";  // Expected header with space
  static const int HEADER_LENGTH = 5;    // Length of "NORA "
  static int headerIndex = 0;            // Tracks current position in header

  while (Serial.available()) {
    char inChar = Serial.read();
    #if DEBUG_SERIAL
      Serial.print(F("Received char: '"));
      Serial.print(inChar);
      Serial.println(F("'"));
    #endif

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

        // Extract data after header
        inputdata = inputString.substring(HEADER_LENGTH);
        inputdata.trim();

        // Check for end of command
        if (inChar == '\n' || inChar == '\r' || inputString.endsWith("\n") || inputString.endsWith("\r")) {
          #if DEBUG_SERIAL
            Serial.print(F("Valid data after NORA: '"));
            Serial.print(inputdata);
            Serial.println(F("'"));
          #endif
          inputdataComplete = true;
          inputString = "";
          headerIndex = 0;  // Reset for next command
        } else {
          #if DEBUG_SERIAL
            Serial.println(F("Waiting for end of line (\\n or \\r)..."));
          #endif
        }
      }
    } else if (headerIndex < HEADER_LENGTH) {
      // Invalid header character: reset and flush Serial buffer
      #if DEBUG_SERIAL
        Serial.print(F("Invalid header character at position "));
        Serial.print(headerIndex + 1);
        Serial.print(F(": expected '"));
        Serial.print(header[headerIndex]);
        Serial.print(F("', got '"));
        Serial.print(inChar);
        Serial.println(F("'"));
      #endif
      inputString = "";
      headerIndex = 0;
      while (Serial.available()) {
        char discarded = Serial.read();
        #if DEBUG_SERIAL
          Serial.print(F("Discarded char: '"));
          Serial.print(discarded);
          Serial.println(F("'"));
        #endif
      }
      #if DEBUG_SERIAL
        Serial.println(F("Serial buffer flushed due to invalid header"));
      #endif
      return;  // Exit to avoid processing more characters
    } else {
      // After header, collect data until newline
      inputString += inChar;
      if (inChar == '\n' || inChar == '\r') {
        inputdata = inputString.substring(HEADER_LENGTH);
        inputdata.trim();
        #if DEBUG_SERIAL
          Serial.print(F("Valid data after NORA: '"));
          Serial.print(inputdata);
          Serial.println(F("'"));
        #endif
        inputdataComplete = true;
        inputString = "";
        headerIndex = 0;  // Reset for next command
      } else {
        #if DEBUG_SERIAL
          Serial.println(F("Waiting for end of line (\\n or \\r)..."));
        #endif
      }
    }

    // Prevent buffer overflow
    if (inputString.length() > 100) {
      #if DEBUG_SERIAL
        Serial.println(F("Input buffer overflow - clearing!"));
      #endif
      inputString = "";
      headerIndex = 0;
      while (Serial.available()) {
        char discarded = Serial.read();
        #if DEBUG_SERIAL
          Serial.print(F("Discarded char: '"));
          Serial.print(discarded);
          Serial.println(F("'"));
        #endif
      }
      #if DEBUG_SERIAL
        Serial.println(F("Serial buffer flushed due to overflow"));
      #endif
    }
  }
}