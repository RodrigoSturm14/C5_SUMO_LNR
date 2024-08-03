//librerias
#include "BluetoothSerial.h"
#include <EngineController.h>

#define PIN_RIGHT_ENGINE_IN1 17
#define PIN_RIGHT_ENGINE_IN2 16
#define PIN_LEFT_ENGINE_IN1 26
#define PIN_LEFT_ENGINE_IN2 27 
#define PWM_CHANNEL_RIGHT_IN1 1
#define PWM_CHANNEL_RIGHT_IN2 2
#define PWM_CHANNEL_LEFT_IN1 3
#define PWM_CHANNEL_LEFT_IN2 4
int speed = 200;

//configuramos el Serial Bluetooth
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;

IEngine *rightEngine = new Driver_DRV8825(PIN_RIGHT_ENGINE_IN1, PIN_RIGHT_ENGINE_IN2, PWM_CHANNEL_RIGHT_IN1, PWM_CHANNEL_RIGHT_IN2);
IEngine *leftEngine = new Driver_DRV8825(PIN_LEFT_ENGINE_IN1, PIN_LEFT_ENGINE_IN2, PWM_CHANNEL_LEFT_IN1, PWM_CHANNEL_LEFT_IN2);
EngineController *Ryo = new EngineController(rightEngine, leftEngine);
void setup()
{
    SerialBT.begin("Test Motores");
}

void loop()
{
    Ryo->Forward(speed);
    SerialBT.println("Forward ");
    delay(3000);
    Ryo->Backward(speed);
    SerialBT.println("Backward ");
    delay(3000);
    Ryo->Left(speed);
    SerialBT.println("Left ");
    delay(3000);
    Ryo->Right(speed);
    SerialBT.println("Right ");
    delay(3000);
    Ryo->Stop();
    SerialBT.println("Stop ");
    delay(3000);
  
}
