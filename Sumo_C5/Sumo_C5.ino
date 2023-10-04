//Librerias
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> //Oled
#include <EngineController.h> //Motores
#include <AnalogSensor.h> //libreria para sensores analogicos( sensores tatami)
#include <DistanceSensors.h> //libreria para sensores
#include <Button_pullup.h>
#include "BluetoothSerial.h" //Bluetooh

//debug
#define DEBUG_SHARP 0
#define DEBUG_TATAMI 0
#define DEBUG_STATE 0
#define DEBUG_ANALOG 0
#define TICK_DEBUG_STATE 500
#define TICK_DEBUG_ANALOG 500
#define TICK_DEBUG_BUTTON 500
#define TICK_DEBUG_SHARP 500
unsigned long currentTimeSharp = 0;
unsigned long currentTimeTatami = 0;
unsigned long currentTimeState = 0;
unsigned long currentTimeAnalog = 0;

int analog;
//Oled
#define SCREEN_WIDTH 128 // OLED width,  in pixels
#define SCREEN_HEIGHT 64 // OLED height, in pixels

//Pines sensores de Distancia
#define PIN_SHARP_LEFT 25
#define PIN_SHARP_CENTER_LEFT 33
#define PIN_SHARP_CENTER_RIGHT 32
#define PIN_SHARP_RIGHT 35

//Pines motores y canales PWM
#define PIN_MOTOR_IZQUIERDO_1 26
#define PIN_MOTOR_IZQUIERDO_2 27
#define PIN_MOTOR_DERECHO_1 16
#define PIN_MOTOR_DERECHO_2 17

#define CANAL_PWM_IZQUIERDO_1 1
#define CANAL_PWM_IZQUIERDO_2 2
#define CANAL_PWM_DERECHO_1 3
#define CANAL_PWM_DERECHO_2 4

// Pulsadores de Inicio y Estrategias
#define PIN_PULSADOR_START_1 5
#define PIN_PULSADOR_ESTRATEGIA_2 4

// Velocidades Sumo
#define VEL_MAX 255
#define VEL_BAJA 150
#define VEL_GIRO_BUSQUEDA 110

// Variables distancia de sensores sharp
#define DIST_LECTURA_MAX 35 // sami = 35
int distSharpCenterLeft = 0;
int distSharpCenterRight = 0;
int distSharpLeft = 0;
int distSharpRight = 0;
class Sharp {
private:
  int pin;
  int n = 3;

public:
  Sharp(int p);
  double SharpDist();
};

Sharp::Sharp(int p) {
  pin = p;
  pinMode(pin, INPUT);
}

double Sharp::SharpDist() {
  long suma = 0;
  for (int i = 0; i < n; i++)  // Realizo un promedio de "n" valores
  {
    suma = suma + analogRead(pin);
  }
  float adc = suma / n;
  // float distancia_cm = 17569.7 * pow(adc, -1.2062);
  if (adc < 400)
    adc = 400;
  // float distancia_cm = 2076.0 / (adc - 11.0);
  // Formula para el sensor GP2Y0A60SZLF
  // https://www.instructables.com/How-to-setup-a-Pololu-Carrier-with-Sharp-GP2Y0A60S/
  double distancia_cm = 187754 * pow(adc, -1.183);  //REGULAR LA POTENCIA PARA OBETENER BUENA PRESICION
  return (distancia_cm);
  delay(100);
}

// --- Sensores Centro
Sharp *sharpCenterLeft = new Sharp(PIN_SHARP_CENTER_LEFT);
Sharp *sharpCenterRight = new Sharp(PIN_SHARP_CENTER_RIGHT);
// --- Sensores Costados
Sharp *sharpLeft = new Sharp(PIN_SHARP_LEFT);
Sharp *sharpRight = new Sharp(PIN_SHARP_RIGHT);

//Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

IEngine *rightEngine = new Driver_DRV8825(PIN_MOTOR_DERECHO_1, PIN_MOTOR_DERECHO_2, CANAL_PWM_DERECHO_1, CANAL_PWM_DERECHO_2);
IEngine *leftEngine = new Driver_DRV8825(PIN_MOTOR_IZQUIERDO_1, PIN_MOTOR_IZQUIERDO_2, CANAL_PWM_IZQUIERDO_1, CANAL_PWM_IZQUIERDO_2);
EngineController *Aldosivi = new EngineController(rightEngine, leftEngine);

//AnalogSensor *rightTatami = new AnalogSensor(PIN_SENSOR_TATAMI_DER);
//AnalogSensor *LeftTatami = new AnalogSensor(PIN_SENSOR_TATAMI_IZQ);

//Isensor *sharpRight = new Sharp_GP2Y0A02(PIN_SENSOR_DISTANCIA_DERECHO);
//Isensor *sharpLeft = new Sharp_GP2Y0A02(PIN_SENSOR_DISTANCIA_IZQUIERDO);

Button *start = new  Button(PIN_PULSADOR_START_1, true);
Button *strategy = new  Button(PIN_PULSADOR_ESTRATEGIA_2);


//AnalogSensor *ldrSensor = new AnalogSensor(PIN_SENSOR_LDR);

//Funciones para imprimir las lecturas de los sensores por el serial Bluetooth
int printAnalog(char t)
{
  if (millis() > currentTimeAnalog + TICK_DEBUG_ANALOG)
  {
    currentTimeAnalog = millis();
    SerialBT.print(t + " :");
    SerialBT.println(analog);
  }
}
void sharpReadings() {
  distSharpCenterLeft = sharpCenterLeft->SharpDist();
  distSharpCenterRight = sharpCenterRight->SharpDist();
  distSharpLeft = sharpLeft->SharpDist();
  distSharpRight = sharpRight->SharpDist();
  // Tatami?
}
void printReadSensors(){
  if (millis() > currentTimeSharp + TICK_DEBUG_SHARP)
  {
    SerialBT.print("Left Distance: ");
    SerialBT.println(distSharpLeft);
    SerialBT.print("Left Center Distance: ");
    SerialBT.println(distSharpCenterLeft);
    SerialBT.print("right Center Distance: ");
    SerialBT.println(distSharpCenterRight);
    SerialBT.print("right Distance: ");
    SerialBT.println(distSharpRight);
    SerialBT.println("-----------------------");
  }
}
void printState(){
  if (millis() > currentTimeState + TICK_DEBUG_STATE)
  {
    SerialBT.println("State :  ");
    SerialBT.println(movimiento);
  }
}

//Enum de estados de movimiento de robot
enum movimiento {
    INICIO,
    BUSQUEDA,
    BUSQUEDA_MEJORADA,
    CORRECCION_IZQUIERDA,
    CORRECCION_DERECHA,
    ATAQUE
};
// Variable que determina el movimiento del robot
int movimiento = INICIO;

int estadoMaquina() {
  if(stateStart) movimiento = BUSQUEDA;
  // Atacar cuando los sensores del centro detectan rival
  if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX) movimiento = 1;
  else if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX) movimiento = 2;
  else if (distSharpCenterRight <= DIST_LECTURA_MAX && distSharpCenterLeft > DIST_LECTURA_MAX) movimiento = 3;
  else if (distSharpLeft <= DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX) movimiento = 4;
  else if (distSharpRight <= DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX) movimiento = 5;
  else movimiento = 6;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Aldosivi");
}

void loop() {
  // Lectura de sharps
  sharpReadings();
  // Seleccion del movimiento del robot
  estadoMaquina();

  switch (movimiento) {
    case INICIO:
      stateStart = start->GetIsPress()
      if(stateStart) movimiento = BUSQUEDA;
      break;
    case BUSQUEDA:
      leftmotor->Backward(200);
      rightmotor->Forward(200);
      break;

    case 2:
      leftmotor->Forward(180);
      rightmotor->Forward(200);
      break;

    case 3:
      leftmotor->Forward(200);
      rightmotor->Forward(180);
      break;

    case 4:
      do {
        leftmotor->Backward(180);
        rightmotor->Forward(180);
      } while (sharpCenterRight->SharpDist() > DIST_LECTURA_MAX);

      leftmotor->Forward(180);
      rightmotor->Forward(180);
      break;

    case 5:
      do {
        leftmotor->Forward(180);
        rightmotor->Backward(180);
      } while (sharpCenterLeft->SharpDist() > DIST_LECTURA_MAX);

      leftmotor->Forward(180);
      rightmotor->Forward(180);
      break;

    case 6:
      // Busqueda del robot
      leftmotor->Forward(175);
      rightmotor->Forward(100);
      break;
  }
}
