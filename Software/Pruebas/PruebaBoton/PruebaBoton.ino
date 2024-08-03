#include "BluetoothSerial.h"

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

bool GetIsPress(int p) {
  bool actualState = digitalRead(p);
  bool state = (previousState != actualState) && actualState == flank;
  previousState = actualState;
  delay(100);
  return state;
}

void setup() {
  SerialBT.begin("Halcon");
  pinMode(18, INPUT_PULLUP);
  pinMode(32, INPUT_PULLUP);
}

void loop() {
  if (millis() > currentTimeButton + TICK_START) {
    if (GetIsPress(18)) SerialBT.println("Press1");
    if (GetIsPress(32)) SerialBT.println("Press2");
  }
}
