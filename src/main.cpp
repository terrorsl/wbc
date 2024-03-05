#include <Arduino.h>
#include"wbc.h"

WaterBoardCounter wbc;

void setup()
{
  wbc.setup();
}

void loop() {
  wbc.loop();
}
