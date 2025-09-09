#pragma once
#include <Arduino.h>

void GPIO(int GPIOIndex, bool state);
void handleSerialCommand(String command);
void parseRGBCommand(String rgbString);

void runEqualizer1();
void runEqualizer2();
void runEqualizer3();

void sound_system_off();
void normal_mode();
void party_mode();

void open_box();
void close_box();

void run_led_wake_word();
