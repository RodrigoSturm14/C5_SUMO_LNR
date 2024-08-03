
#include <DistanceSensors.h>
#include "BluetoothSerial.h"

#define PIN_SHARP_LEFT 25
#define PIN_SHARP_CENTER_LEFT 35
#define PIN_SHARP_CENTER 34
#define PIN_SHARP_CENTER_RIGHT 39
#define PIN_SHARP_RIGHT 36
int distance_left = 0;
int distance_center_left = 0;
int distance_center = 0;
int distance_center_right = 0;
int distance_right = 0;

unsigned long currentTime = 0;
#define TICK_DEBUG 500

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

Isensor *sharp_left = new Sharp_GP2Y0A02(PIN_SHARP_LEFT);
Isensor *sharp_center_left = new Sharp_GP2Y0A02(PIN_SHARP_CENTER_LEFT);

Isensor *sharp_center = new Sharp_GP2Y0A02(PIN_SHARP_CENTER);

Isensor *sharp_center_right = new Sharp_GP2Y0A02(PIN_SHARP_CENTER_RIGHT);
Isensor *sharp_right = new Sharp_GP2Y0A02(PIN_SHARP_RIGHT);

void setup() {
  Serial.begin(115200);
  SerialBT.begin("Halcon");
}

void printRead(){
  if (millis() > currentTime + TICK_DEBUG)
  {
    currentTime = millis();
    SerialBT.print("Left Distance: ");
    SerialBT.println(distance_left);
    SerialBT.print("Left Center Distance: ");
    SerialBT.println(distance_center_left);
    SerialBT.print("center Distance: ");
    SerialBT.println(distance_center);
    SerialBT.print("right Center Distance: ");
    SerialBT.println(distance_center_right);
    SerialBT.print("right Distance: ");
    SerialBT.println(distance_right);
    SerialBT.println("-----------------------");
  }
}

void loop() {
  
  
  distance_left = sharp_left->SensorRead();
  distance_center_left = sharp_center_left->SensorRead();
  distance_center = sharp_center->SensorRead();
  distance_center_right = sharp_center_right->SensorRead(); 
  distance_right  = sharp_right->SensorRead();
 
  printRead();
}
