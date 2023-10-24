//Librerias
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>  //Oled
#include <EngineController.h>  //Motores
#include <AnalogSensor.h>      //libreria para sensores analogicos( sensores tatami)
#include <DistanceSensors.h>   //libreria para sensores
//#include <Button.h>
#include "BluetoothSerial.h"  //Bluetooh

//debug
#define DEBUG_SHARP 0
#define DEBUG_STATE 1
#define DEBUG_ANALOG 0
#define TICK_DEBUG_STATE 1500
#define TICK_DEBUG_ANALOG 1500
#define TICK_DEBUG_SHARP 1500

#define TICK_BUTTON_STATE 500
#define TICK_GIRO_INICIO 50
#define TICK_INICIO 4900

unsigned long currentTimeState = 0;
int analog;
//Oled
#define SCREEN_WIDTH 128  // OLED width,  in pixels
#define SCREEN_HEIGHT 64  // OLED height, in pixels

//Pines sensores de Distancia

//Pines motores y canales PWM
#define PIN_MOTOR_IZQUIERDO_1 17
#define PIN_MOTOR_IZQUIERDO_2 16
#define PIN_MOTOR_DERECHO_1 26
#define PIN_MOTOR_DERECHO_2 27

#define CANAL_PWM_IZQUIERDO_1 1
#define CANAL_PWM_IZQUIERDO_2 2
#define CANAL_PWM_DERECHO_1 3
#define CANAL_PWM_DERECHO_2 4

// Pulsadores de Inicio y Estrategias
#define PIN_PULSADOR_START_1 5
#define PIN_PULSADOR_ESTRATEGIA_2 4

#define TICK_GIRO_IZQUIERDA_45 100
#define TICK_GIRO_IZQUIERDA_90 150
#define TICK_GIRO_DERECHA_45 100
#define TICK_GIRO_DERECHA_90 150
#define TICK_ADELANTE 100

#define TICK_START 1000
#define MAX_MODE 6
#define MIN_MODE 1
bool stateStart = 1;
unsigned long currentTimeButton = 0;
int count_estrategia = 0;

// Variables de pantalla OLED
#define SCREEN_WIDTH 128  // OLED width,  in pixels
#define SCREEN_HEIGHT 64  // OLED height, in pixels
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Velocidades Sumo
#define VEL_MAX 255
#define VEL_BAJA 150
#define VEL_CORRECCION 90
#define VEL_GIRO 110
#define VEL_EJE_BUSQUEDA 110
#define VEL_CORRECCION_IZQ_IZQ 240
#define VEL_CORRECCION_IZQ_DER 255
#define VEL_CORRECCION_DER_IZQ 255
#define VEL_CORRECCION_DER_DER 240
#define VEL_GIRO_BUSQUEDA_MEJORADA_IZQ 90  // 130
#define VEL_GIRO_BUSQUEDA_MEJORADA_DER 80  // 100

//configuramos el Serial Bluetooth
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;

//Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

IEngine *rightEngine = new Driver_DRV8825(PIN_MOTOR_DERECHO_1, PIN_MOTOR_DERECHO_2, CANAL_PWM_DERECHO_1, CANAL_PWM_DERECHO_2);
IEngine *leftEngine = new Driver_DRV8825(PIN_MOTOR_IZQUIERDO_1, PIN_MOTOR_IZQUIERDO_2, CANAL_PWM_IZQUIERDO_1, CANAL_PWM_IZQUIERDO_2);
EngineController *Aldosivi = new EngineController(rightEngine, leftEngine);

bool GetIsPress() {
  return digitalRead(PIN_PULSADOR_START_1);
}

enum movimietos_predefinidos {
  NONE,
  GIRO_IZQUIERDA_45,
  GIRO_IZQUIERDA_90,
  GIRO_DERECHA_45,
  GIRO_DERECHA_90,
  AMAGUE_ADELANTE_DERECHA,
  AMAGUE_ADELANTE_IZQUIERDA
};

void printEstrategia() {
  String state = "";
  switch (count_estrategia) {
    case GIRO_IZQUIERDA_45:
      state = "GIRO_IZQUIERDA_45";
      break;
    case GIRO_IZQUIERDA_90:
      state = "GIRO_IZQUIERDA_90";
      break;
    case GIRO_DERECHA_45:
      state = "GIRO_DERECHA_45";
      break;
    case GIRO_DERECHA_90:
      state = "GIRO_DERECHA_90";
      break;
    case AMAGUE_ADELANTE_DERECHA:
      state = "AMAGUE_ADELANTE_DERECHA";
      break;
    case AMAGUE_ADELANTE_IZQUIERDA:
      state = "AMAGUE_ADELANTE_IZQUIERDA";
      break;
  }
  SerialBT.print("State: ");
  SerialBT.println(state);
  SerialBT.println("---------");
}

void movimientoPredefinido() {
  switch (count_estrategia) {
    case GIRO_IZQUIERDA_45:
      Aldosivi->Left(VEL_MAX, VEL_MAX);
      delay(TICK_GIRO_IZQUIERDA_45);
      break;
    case GIRO_IZQUIERDA_90:
      Aldosivi->Left(VEL_MAX, VEL_MAX);
      delay(TICK_GIRO_IZQUIERDA_90);
      break;
    case GIRO_DERECHA_45:
      Aldosivi->Right(VEL_MAX, VEL_MAX);
      delay(TICK_GIRO_DERECHA_45);
      break;
    case GIRO_DERECHA_90:
      Aldosivi->Right(VEL_MAX, VEL_MAX);
      delay(TICK_GIRO_DERECHA_90);
      break;
    case AMAGUE_ADELANTE_DERECHA:
      Aldosivi->Forward(VEL_MAX, VEL_MAX);
      delay(TICK_ADELANTE);
      Aldosivi->Right(VEL_MAX, VEL_MAX);
      delay(TICK_GIRO_DERECHA_90);
      break;
    case AMAGUE_ADELANTE_IZQUIERDA:
      Aldosivi->Forward(VEL_MAX, VEL_MAX);
      delay(TICK_ADELANTE);
      Aldosivi->Left(VEL_MAX, VEL_MAX);
      delay(TICK_GIRO_IZQUIERDA_90);
      break;
  }
  if (DEBUG_STATE) {
    printEstrategia();
  }
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("Aldosivi");
  Wire.begin();
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(PIN_PULSADOR_START_1, INPUT_PULLUP);
  pinMode(PIN_PULSADOR_ESTRATEGIA_2, INPUT_PULLDOWN);

  Aldosivi->Stop();
  // stateStart = GetIsPress();
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(4, 0);
  oled.println("INICIO");
  oled.display();

  while (GetIsPress() == true) {
    if (digitalRead(PIN_PULSADOR_ESTRATEGIA_2) == true) {
      if (millis() > currentTimeState + TICK_BUTTON_STATE) {
        currentTimeState = millis();
        count_estrategia++;
        if (count_estrategia > MAX_MODE) count_estrategia = MIN_MODE;
        oled.clearDisplay();
        oled.setCursor(0, 9);
        oled.println("---------------------");
        oled.setCursor(0, 26);
        oled.print("Estrategia: ");
        oled.println(count_estrategia);
        oled.display();
        if (DEBUG_STATE) {
          SerialBT.print("Estrategia: ");
          SerialBT.println(count_estrategia);
        }
      }
    }
  }

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(4, 0);
  oled.println("Pressed");
  oled.setCursor(0, 9);
  oled.println("---------------------");
  oled.setCursor(0, 26);
  oled.print("Estrategia: ");
  oled.println(count_estrategia);
  oled.display();

  if (DEBUG_STATE) SerialBT.println("Pressed");

  delay(TICK_INICIO);
  oled.clearDisplay();
  oled.display();

  movimientoPredefinido();

  Aldosivi->Stop();
}

void loop() {
}
