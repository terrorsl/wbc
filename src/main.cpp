#include <Arduino.h>
#include"wbc.h"

WaterBoardCounter wbc;

void setup()
{
  wbc.setup();
  //wbc.init_wifi();
}

void loop() {
  wbc.loop();
}
