#include <FastLED.h>
#include "pins.h"
#include "state.h"
#include "functions.h"

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(GPIOPins[i], OUTPUT);
    digitalWrite(GPIOPins[i], false);
  }
  inputString.reserve(100);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(customBrightness);
  FastLED.clear(true);

  normal_mode();
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
}

void loop() {
  serialEvent();
  if (inputdataComplete) {
    String command = inputdata;
    command.trim();
    Serial.print("Processing command: "); Serial.println(command);  // دیباگ
    handleSerialCommand(command);
    inputdata = "";
    inputdataComplete = false;
  } else if (Serial.available()) {
    char c = Serial.read();
    if (c == 'R') {
      open_box();
      close_box();
    }
  } else if (relayActive && millis() - relayOnTime >= 8000) {
    digitalWrite(OPEN_BOX, false);
    digitalWrite(CLOSE_BOX, false);
    relayActive = false;
    Serial.println("Relay auto off after 8s");
  } else if (equalizer1Active) {
    runEqualizer1();
  } else if (equalizer2Active) {
    runEqualizer2();
  } else if (equalizer3Active) {
    runEqualizer3();
  }
}

// void serialEvent(){
//   while (Serial.available()){
//     char inChar = (char)Serial.read();

//     if(inChar >= 32 && inChar<=126){
//       inputString += inChar;
//     } else if (inChar =='\n' || inChar == '\r'){
//       stringComplete = true;
//     }
//   }
// }


void serialEvent() {
  while (Serial.available()) {
    char inChar = Serial.read();
    inputString += inChar;
    Serial.print("Received char: '"); Serial.print(inChar); Serial.println("'");  // دیباگ هر کاراکتر

    // اگر طول inputString کمتر از 4 باشه، منتظر بمون (هدر ناقصه)
    if (inputString.length() < HEADER_LENGTH) {
      continue;
    }

    // چک کردن هدر "NORA" (4 بایت اول)
    String header = inputString.substring(0, HEADER_LENGTH);
    if (header == HEADER_NORA) {
      Serial.println("Valid header 'NORA' detected!");  // دیباگ

      // استخراج داده‌ها (از بایت 5 تا انتها، تا \n یا \r)
      inputdata = inputString.substring(HEADER_LENGTH);  // از بعد از هدر
      inputdata.trim();  // حذف فضاهای اضافی

      // چک کردن پایان داده با \n یا \r
      if (inputString.endsWith("\n") || inputString.endsWith("\r")) {
        Serial.print("Valid data after NORA: '"); Serial.print(inputdata); Serial.println("'");  // دیباگ
        inputdataComplete = true;
        inputString = "";  // خالی کردن بافر
      } else {
        Serial.println("Waiting for end of line (\n or \r)...");  // دیباگ
      }
    } else {
      Serial.println("Not valid header! Expected 'NORA', got: '" + header + "'");  // پیام خطا
      inputString = "";  // خالی کردن بافر
    }

    // جلوگیری از سرریز بافر (اختیاری، برای ایمنی)
    if (inputString.length() > 100) {
      Serial.println("Input buffer overflow - clearing!");
      inputString = "";
    }
  }
}