// #include <FastLED.h>
// #include "pins.h"
// #include "state.h"
// #include "functions.h"

// void setup() {
//   Serial.begin(115200);
//   for (int i = 0; i < NUM_PINS; i++) {
//     pinMode(GPIOPins[i], OUTPUT);
//     digitalWrite(GPIOPins[i], LOW);
//   }
//   inputString.reserve(100);  // Reserve memory for inputString
//   FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
//   FastLED.setBrightness(customBrightness);
//   FastLED.clear(true);

//   normal_mode();
//   currentPalette = RainbowColors_p;
//   currentBlending = LINEARBLEND;
//   pinMode(READINGLIGHT, OUTPUT);    // Explicitly set pin 17 as output
//   digitalWrite(READINGLIGHT, LOW);  // Initial state
//   RainbowActive = false;
//   EqualizeActive = false;
//   StaticActive = false;
//   boxRainbowActive = false;
//   boxEqualizeActive = false;
//   boxStaticActive = false;
//   ledMode = "off";
// }

// /*void loop() {
//   serialEvent();  // Manual call for ESP32 (not serialEventRun)
//   if (inputdataComplete) {
//     handleSerialCommand(inputdata);
//     if (RainbowActive) {
//       runRainbow();
//     } else if (EqualizeActive) {
//       runEqualize();
//     } else if (StaticActive) {
//       runStatic();
//     } else if (boxRainbowActive) {
//       runBOXRainbow();
//     } else if (boxEqualizeActive) {
//       runBOXEqualize();
//     } else if (boxStaticActive) {
//       runBOXStatic();
//     }
//     if (ledMode != "off") {
//       FastLED.setBrightness(brightnessLevel * 85 + 50);
//       FastLED.show();
//     }
//     inputdataComplete = false;
//   } else if (relayActive && millis() - relayOnTime >= 8000) {
//     digitalWrite(OPEN_BOX, LOW);
//     digitalWrite(CLOSE_BOX, LOW);
//     relayActive = false;
//     Serial.println(F("Relay auto off after 8s"));
//   } else if (RainbowActive) {
//     runRainbow();
//   } else if (EqualizeActive) {
//     runEqualize();
//   } else if (StaticActive) {
//     runStatic();
//   } else if (boxRainbowActive) {
//     runBOXRainbow();
//   } else if (boxEqualizeActive) {
//     runBOXEqualize();
//   } else if (boxStaticActive) {
//     runBOXStatic();
//   }
// }*/
// void loop() {
//   serialEvent();  // Manual call for ESP32
//   if (inputdataComplete) {
//     handleSerialCommand(inputdata);
//     if (ledMode != "off") {
//       if (ledMode == "rainbow" && RainbowActive) {
//         runRainbow();
//       } else if (ledMode == "equalize" && EqualizeActive) {
//         runEqualize();
//       } else if (ledMode == "static" && StaticActive) {
//         runStatic();
//       } else if (ledMode == "rainbow" && boxRainbowActive) {
//         runBOXRainbow();
//       } else if (ledMode == "equalize" && boxEqualizeActive) {
//         runBOXEqualize();
//       } else if (ledMode == "static" && boxStaticActive) {
//         runBOXStatic();
//       }
//       FastLED.setBrightness(brightnessLevel * 85 + 50);
//       FastLED.show();
//     } else {
//       FastLED.clear(true);
//       FastLED.show();
//     }
//     inputdataComplete = false;
//   } else if (relayActive && millis() - relayOnTime >= 8000) {
//     digitalWrite(OPEN_BOX, LOW);
//     digitalWrite(CLOSE_BOX, LOW);
//     relayActive = false;
//     Serial.println(F("Relay auto off after 8s"));
//   }
//   // دیباگ اختیاری (موقتاً کامنت کنید)
//   // Serial.print(F("RainbowActive: "));
//   // Serial.println(RainbowActive);
// }

// void serialEvent() {
//   static const char header[] = "NORA_";  // Expected header with space
//   static const int HEADER_LENGTH = 5;    // Length of "NORA "
//   static int headerIndex = 0;            // Tracks current position in header
//   static bool headerComplete = false;    // Tracks if header is fully matched

//   while (Serial.available()) {
//     char inChar = Serial.read();
// #if DEBUG_SERIAL
//     Serial.print(F("Received char: '"));
//     Serial.print(inChar);
//     Serial.println(F("'"));
// #endif

//     // If header is not yet complete, check for header characters
//     if (!headerComplete) {
//       // Convert input character to uppercase for case-insensitive comparison (except for space)
//       char inCharUpper = (headerIndex < 4) ? toupper(inChar) : inChar;

//       // Check if current character matches expected header character
//       if (headerIndex < HEADER_LENGTH && inCharUpper == header[headerIndex]) {
//         inputString += inChar;
//         headerIndex++;
// #if DEBUG_SERIAL
//         Serial.print(F("Header match at position "));
//         Serial.print(headerIndex);
//         Serial.print(F(": '"));
//         Serial.print(inChar);
//         Serial.println(F("'"));
// #endif

//         // If full header is matched
//         if (headerIndex == HEADER_LENGTH) {
// #if DEBUG_SERIAL
//           Serial.println(F("Valid header 'NORA ' detected!"));
// #endif
//           headerComplete = true;  // Start collecting data
//         }
//       } else {
//         // If header matching fails, reset but check for a new 'N'
//         if (headerIndex > 0) {
// #if DEBUG_SERIAL
//           Serial.print(F("Invalid header character at position "));
//           Serial.print(headerIndex + 1);
//           Serial.print(F(": expected '"));
//           Serial.print(header[headerIndex]);
//           Serial.print(F("', got '"));
//           Serial.print(inChar);
//           Serial.println(F("'"));
// #endif
//           // Slide back to check for a new 'N'
//           if (toupper(inChar) == 'N') {
//             inputString = "N";
//             headerIndex = 1;
//           } else {
//             inputString = "";
//             headerIndex = 0;
//           }
//         } else if (toupper(inChar) == 'N') {
//           // Start of a potential new header
//           inputString = "N";
//           headerIndex = 1;
// #if DEBUG_SERIAL
//           Serial.println(F("Potential new header started with 'N'"));
// #endif
//         } else {
// // Ignore non-'N' characters when not matching header
// #if DEBUG_SERIAL
//           Serial.print(F("Ignoring non-header char: '"));
//           Serial.print(inChar);
//           Serial.println(F("'"));
// #endif
//         }
//       }
//     } else {
//       // Header is complete, collect data
//       inputString += inChar;
//       // Check for end of command
//       if (inChar == '\n' || inChar == '\r') {
//         inputdata = inputString.substring(HEADER_LENGTH);
//         inputdata.trim();
// #if DEBUG_SERIAL
//         Serial.print(F("Valid data after NORA: '"));
//         Serial.print(inputdata);
//         Serial.println(F("'"));
// #endif
//         // Validate command length
//         if (inputdata.length() > 0 && inputdata.length() <= 50) {
//           inputdataComplete = true;
//         } else {
// #if DEBUG_SERIAL
//           Serial.println(F("Invalid command length after NORA"));
// #endif
//         }
//         inputString = "";
//         headerIndex = 0;
//         headerComplete = false;  // Reset for next command
//       }
//     }

//     // Prevent buffer overflow
//     if (inputString.length() > 40) {
// #if DEBUG_SERIAL
//       Serial.println(F("Input buffer overflow - clearing!"));
// #endif
//       inputString = "";
//       headerIndex = 0;
//       headerComplete = false;
//     }
//   }
// }

// Nora_v2.ino - Master ESP32 for Nora Project with ESP-NOW
// Reads serial input for clock and magicB commands, forwards to Slave via ESP-NOW
// Fixed for ESP32 Arduino Core 3.3.0 compatibility
// Integrates ESP-NOW functionality, corrected callback signatures

#include <esp_now.h>
#include <WiFi.h>
#include <FastLED.h>
#include "pins.h"
#include "state.h"
#include "functions.h"

// ESP-NOW Configuration
uint8_t slaveMACAddress[] = {0x24, 0x0A, 0xC4, 0x00, 0x01, 0x10}; // Replace with actual Slave MAC
esp_now_peer_info_t peerInfo;
bool espNowInitialized = false;

// Callback for send status (fixed for 3.3.0)
void OnDataSent(const wifi_tx_info_t *send_info, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// Callback for received data
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  Serial.print("Recv from Slave: ");
  for (int i = 0; i < len; i++) {
    Serial.print((char)data[i]);
  }
  Serial.println();
}

// Initialize ESP-NOW
bool initESPNow() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Fail");
    return false;
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  memcpy(peerInfo.peer_addr, slaveMACAddress, 6);
  peerInfo.channel = 1; // Use default channel
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Peer Add Fail");
    return false;
  }
  Serial.println("ESP-NOW Ready");
  return true;
}

// Send command to Slave with CRC
bool sendCommandToSlave(String cmd) {
  if (!espNowInitialized) {
    Serial.println("ESP-NOW Not Initialized");
    return false;
  }
  uint8_t crc = crc8((uint8_t*)cmd.c_str(), cmd.length());
  char crcStr[3];
  sprintf(crcStr, "%02X", crc);
  String fullCmd = cmd + "_" + crcStr;
  esp_err_t res = esp_now_send(slaveMACAddress, (uint8_t*)fullCmd.c_str(), fullCmd.length());
  Serial.print("Sent: ");
  Serial.println(fullCmd);
  return (res == ESP_OK);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Nora Master Starting...");

  // GPIO Init
  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(GPIOPins[i], OUTPUT);
    digitalWrite(GPIOPins[i], LOW);
  }
  inputString.reserve(100);

  // FastLED Init (local if needed)
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(customBrightness);
  FastLED.clear();

  // Default States
  normal_mode();
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
  pinMode(READINGLIGHT, OUTPUT);
  digitalWrite(READINGLIGHT, LOW);
  RainbowActive = false;
  EqualizeActive = false;
  StaticActive = false;
  boxRainbowActive = false;
  boxEqualizeActive = false;
  boxStaticActive = false;
  ledMode = "off";

  // ESP-NOW Init
  espNowInitialized = initESPNow();
  Serial.println("Master Ready - Send NORA_ commands");
}

void loop() {
  serialEvent();  // Read serial
  if (inputdataComplete) {
    // Check component
    int firstUnderscore = inputdata.indexOf('_');
    String component = (firstUnderscore > 0) ? inputdata.substring(0, firstUnderscore) : "";
    component = toLowerCaseString(component);

    if (component == "clock" || component == "magicbl") {
      // Forward to Slave
      if (sendCommandToSlave(inputdata)) {
        Serial.println("Forwarded to Slave");
      } else {
        Serial.println("Forward Fail");
      }
    } else {
      // Local process
      handleSerialCommand(inputdata);
      // LED Update (local)
      if (ledMode != "off") {
        if (ledMode == "rainbow" && RainbowActive) runRainbow();
        else if (ledMode == "equalize" && EqualizeActive) runEqualize();
        else if (ledMode == "static" && StaticActive) runStatic();
        else if (ledMode == "rainbow" && boxRainbowActive) runBOXRainbow();
        else if (ledMode == "equalize" && boxEqualizeActive) runBOXEqualize();
        else if (ledMode == "static" && boxStaticActive) runBOXStatic();
        FastLED.setBrightness(brightnessLevel * 85 + 50);
        FastLED.show();
      } else {
        FastLED.clear();
        FastLED.show();
      }
    }
    inputdataComplete = false;
  }

  // Relay Timeout
  if (relayActive && millis() - relayOnTime >= 8000) {
    digitalWrite(OPEN_BOX, LOW);
    digitalWrite(CLOSE_BOX, LOW);
    relayActive = false;
    Serial.println("Relay Off");
  }

  delay(10);
}

void serialEvent() {
  static const char header[] = "NORA_";  // Header (underscore, no space)
  static const int HEADER_LENGTH = 5;
  static int headerIndex = 0;
  static bool headerComplete = false;

  while (Serial.available()) {
    char inChar = Serial.read();
#if DEBUG_SERIAL
    Serial.print("Char: '");
    Serial.print(inChar);
    Serial.println("'");
#endif

    if (!headerComplete) {
      char inCharUpper = (headerIndex < 4) ? toupper(inChar) : inChar;
      if (headerIndex < HEADER_LENGTH && inCharUpper == header[headerIndex]) {
        inputString += inChar;
        headerIndex++;
        if (headerIndex == HEADER_LENGTH) {
          headerComplete = true;
#if DEBUG_SERIAL
          Serial.println("Header OK");
#endif
        }
      } else {
        if (headerIndex > 0) {
          if (toupper(inChar) == 'N') {
            inputString = "N";
            headerIndex = 1;
          } else {
            inputString = "";
            headerIndex = 0;
          }
        } else if (toupper(inChar) == 'N') {
          inputString = "N";
          headerIndex = 1;
        }
      }
    } else {
      inputString += inChar;
      if (inChar == '\n' || inChar == '\r') {
        inputdata = inputString.substring(HEADER_LENGTH);
        inputdata.trim();
#if DEBUG_SERIAL
        Serial.print("Data: '");
        Serial.print(inputdata);
        Serial.println("'");
#endif
        if (inputdata.length() > 0 && inputdata.length() <= 50) {
          inputdataComplete = true;
        }
        inputString = "";
        headerIndex = 0;
        headerComplete = false;
      }
    }

    if (inputString.length() > 40) {
      inputString = "";
      headerIndex = 0;
      headerComplete = false;
    }
  }
}