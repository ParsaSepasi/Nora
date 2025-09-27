#include <esp_now.h>
#include <WiFi.h>
#include <FastLED.h>
#include "pins.h"
#include "state.h"
#include "functions.h"

// ESP-NOW Configuration
uint8_t slaveMACAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Replace with actual Slave MAC
esp_now_peer_info_t peerInfo;
bool espNowInitialized = false;

// Callback for send status
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
  peerInfo.channel = 1;
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
  uint8_t crc = crc8((uint8_t *)cmd.c_str(), cmd.length());
  char crcStr[3];
  sprintf(crcStr, "%02X", crc);
  String fullCmd = cmd + "_" + crcStr;
  esp_err_t res = esp_now_send(slaveMACAddress, (uint8_t *)fullCmd.c_str(), fullCmd.length());
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

  // FastLED Init
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.addLeds<LED_TYPE, BOX_PIN, COLOR_ORDER>(box_leds, NUM_BOX_LEDS);
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
  serialEvent();
  if (inputdataComplete) {
    int firstUnderscore = inputdata.indexOf('_');
    String component = (firstUnderscore > 0) ? inputdata.substring(0, firstUnderscore) : "";
    component = toLowerCaseString(component);

    // همیشه پردازش محلی انجام بده
    handleSerialCommand(inputdata);

    // اگر clock یا magicbl باشه، به Slave هم بفرست
    if (component == "clock" || component == "magicbl") {
      if (sendCommandToSlave(inputdata)) {
        Serial.println("Forwarded to Slave and processed locally in Master");
      } else {
        Serial.println("Forward Fail");
      }
    }
    inputdataComplete = false;
  }

  // LED Update برای magicl (GPIO 21)
  if (RainbowActive || EqualizeActive || StaticActive) {
    Serial.print(F("Updating magicl: "));
    Serial.print(F("ledMode: "));
    Serial.print(ledMode);
    Serial.print(F(", RainbowActive: "));
    Serial.println(RainbowActive);
    if (ledMode == "rainbow" && RainbowActive) {
      runRainbow();
    } else if (ledMode == "equalize" && EqualizeActive) {
      runEqualize();
    } else if (ledMode == "static" && StaticActive) {
      runStatic();
    }
  }

  // LED Update برای magicbl (GPIO 22)
  if (boxRainbowActive || boxEqualizeActive || boxStaticActive) {
    Serial.print(F("Updating magicbl: "));
    Serial.print(F("boxRainbowActive: "));
    Serial.println(boxRainbowActive);
    if (boxRainbowActive) {
      runBOXRainbow();
    } else if (boxEqualizeActive) {
      runBOXEqualize();
    } else if (boxStaticActive) {
      runBOXStatic();
    }
  }

  // اعمال تغییرات LED
  FastLED.setBrightness(brightnessLevel * 85 + 50);
  FastLED.show();

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
  static const char header[] = "NORA_";
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