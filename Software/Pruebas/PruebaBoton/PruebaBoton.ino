#include "BluetoothSerial.h"

#define PIN_BUTTON 5
unsigned long currentTimeButton = 0;
#define TICK_START 1000

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;

bool flank = HIGH;
bool previousState;
void SetFlank(bool f) {
  flank = f;
  previousState = !flank;
}

bool GetIsPress() {
  bool actualState = digitalRead(PIN_BUTTON);
  bool state = (previousState != actualState) && actualState == flank;
  previousState = actualState;
  delay(100);
  return state;
}

void setup() {
  SerialBT.begin("Aldosivi");
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (millis() > currentTimeButton + TICK_START) {
    if (GetIsPress()) SerialBT.println("Press");
  }
}
