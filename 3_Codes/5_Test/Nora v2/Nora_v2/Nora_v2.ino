#include <FastLED.h>
#include "pins.h"
#include "state.h"
#include "functions.h"

void setup(){
  Serial.begin(115200);
  for (int i = 0; i < NUM_PINS; i++){
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

void loop(){
  if (stringComplete){
    String command = inputString;
    command.trim();
    handleSerialCommand(command);
    inputString = "";
    stringComplete = false;
  }
  else if (Serial.available()){
    char c =Serial.read();
    if(c == 'R'){
      open_box();
      close_box();
    }
  }
  else if(relayActive && millis() - relayOnTime >= 8000){
    digitalWrite(OPEN_BOX, false);
    digitalWrite(CLOSE_BOX, false);
    relayActive = false;
    Serial.println("Relay auto off after 8s");
  }
  else if (equalizer1Active) {runEqualizer1();}
  else if (equalizer2Active) {runEqualizer2();}
  else if (equalizer3Active) {runEqualizer3();}
}

void serialEvent(){
  while (Serial.available()){
    char inChar = (char)Serial.read();
    
    if(inChar >= 32 && inChar<=126){
      inputString += inChar;
    } else if (inChar =='\n' || inChar == '\r'){
      stringComplete = true;
    }
  }
}
