//Librerias
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>  //Oled
#include <EngineController.h>  //Motores
#include <AnalogSensor.h>      //libreria para sensores analogicos( sensores tatami)
#include "BluetoothSerial.h"   //Bluetooh
#include <PS4Controller.h>

//debug
#define DEBUG_SHARP 0
#define DEBUG_STATE 0
#define DEBUG_ANALOG 0
#define TICK_DEBUG_STATE 1500
#define TICK_DEBUG_ANALOG 1500
#define TICK_DEBUG_SHARP 1500

#define TICK_GIRO_INICIO 50
#define TICK_INICIO 4900

unsigned long currentTimeSharp = 0;
unsigned long currentTimeState = 0;
unsigned long currentTimeAnalog = 0;

int analog;
//Oled
#define SCREEN_WIDTH 128  // OLED width,  in pixels
#define SCREEN_HEIGHT 64  // OLED height, in pixels

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
bool stateStart = 1;
unsigned long currentTimeButton = 0;
#define TICK_START 1000

// Variables de pantalla OLED
#define SCREEN_WIDTH 128  // OLED width,  in pixels
#define SCREEN_HEIGHT 64  // OLED height, in pixels
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Velocidades Sumo Radiocontrolado
#define VEL_MAX 250
#define VEL_MEDIA 170
#define VEL_BAJA 100
#define VEL_GIRO_MAX 150
#define VEL_GIRO_MEDIA 70
#define VEL_GIRO_BAJA 50
int velocity, turn_velocity;

/*#define VEL_CORRECCION 90
#define VEL_GIRO 110
#define VEL_EJE_BUSQUEDA 110
#define VEL_CORRECCION_IZQ_IZQ 240
#define VEL_CORRECCION_IZQ_DER 255
#define VEL_CORRECCION_DER_IZQ 255
#define VEL_CORRECCION_DER_DER 240
#define VEL_GIRO_BUSQUEDA_MEJORADA_IZQ 90  // 130
#define VEL_GIRO_BUSQUEDA_MEJORADA_DER 80  // 100
*/
#define MAX_ANALOG_VALUE 50

//configuramos el Serial Bluetooth
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;

//Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

IEngine *rightEngine = new Driver_DRV8825(PIN_MOTOR_DERECHO_1, PIN_MOTOR_DERECHO_2, CANAL_PWM_DERECHO_1, CANAL_PWM_DERECHO_2);
IEngine *leftEngine = new Driver_DRV8825(PIN_MOTOR_IZQUIERDO_1, PIN_MOTOR_IZQUIERDO_2, CANAL_PWM_IZQUIERDO_1, CANAL_PWM_IZQUIERDO_2);
EngineController *Aldosivi = new EngineController(rightEngine, leftEngine);

bool flank = HIGH;
bool previousState;
void SetFlank(bool f) {
  flank = f;
  previousState = !flank;
}

bool GetIsPress() {
  /*
  bool actualState = digitalRead(PIN_PULSADOR_START_1);
  bool state = (previousState != actualState) && actualState == flank;
  previousState = actualState;
  delay(100);
  return state;
  */
  return digitalRead(PIN_PULSADOR_START_1);
}

void sumo_rc_loop() {
  int y_axis_value = PS4.LStickY();
  int x_axis_value = PS4.RStickX();
  int l2 = PS4.L2Value();
  int r2 = PS4.R2Value();

  if (r2 > MAX_ANALOG_VALUE) {
    velocity = VEL_MAX;
    turn_velocity = VEL_GIRO_MAX;
  }

  if (l2 > MAX_ANALOG_VALUE) {
    velocity = VEL_BAJA;
    turn_velocity = VEL_GIRO_BAJA;
  }

  if (r2 < MAX_ANALOG_VALUE && l2 < MAX_ANALOG_VALUE) {
    velocity = VEL_MEDIA;
    turn_velocity = VEL_GIRO_MEDIA;
  }

  if (y_axis_value >= MAX_ANALOG_VALUE) {
    // Ir para adelante
    Aldosivi->Forward(velocity, velocity);
    /*
    leftmotor->Forward(velocity);
    rightmotor->Forward(velocity);
    */
    if (x_axis_value >= MAX_ANALOG_VALUE) {
      // Ir a la derecha
      Aldosivi->Forward(turn_velocity, velocity);
      /*
      leftmotor->Forward(velocity);
      rightmotor->Forward(turn_velocity);
      */
    }
    if (x_axis_value <= -(MAX_ANALOG_VALUE)) {
      // Ir a la izquierda
      Aldosivi->Forward(velocity, turn_velocity);
      /*
      leftmotor->Forward(turn_velocity);
      rightmotor->Forward(velocity);
      */
    }
  } else if (y_axis_value <= -(MAX_ANALOG_VALUE)) {
    // Ir para atras
    Aldosivi->Backward(velocity, velocity);
    /*
    leftmotor->Backward(velocity);
    rightmotor->Backward(velocity);
    */
    if (x_axis_value >= MAX_ANALOG_VALUE) {
      // Marcha atras - giro a la derecha
      Aldosivi->Backward(turn_velocity, velocity);
      /*
      leftmotor->Backward(velocity);
      rightmotor->Backward(turn_velocity);
      */
    }
    if (x_axis_value <= -(MAX_ANALOG_VALUE)) {
      // Marcha atras - giro a la izquierda
      Aldosivi->Backward(velocity, turn_velocity);
      /*
      leftmotor->Backward(turn_velocity);
      rightmotor->Backward(velocity);
      */
    }
  }

  else if (x_axis_value >= MAX_ANALOG_VALUE) {
    // Girar a la derecha
    Aldosivi->Forward(turn_velocity, velocity);
    /*
    leftmotor->Forward(velocity);
    rightmotor->Forward(turn_velocity);
    */
  }

  else if (x_axis_value <= -(MAX_ANALOG_VALUE)) {
    // Girar a la izquierda
    Aldosivi->Forward(velocity, turn_velocity);
    /*
    leftmotor->Forward(turn_velocity);
    rightmotor->Forward(velocity);
    */
  }

  else {
    // Quieto
    Aldosivi->Stop();
    /*
    leftmotor->Stop();
    rightmotor->Stop();
    */
  }
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("Aldosivi");
  Wire.begin();
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(PIN_PULSADOR_START_1, INPUT_PULLUP);
  PS4.begin();
  Aldosivi->Stop();
  // stateStart = GetIsPress();
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(4, 0);
  oled.println("INICIO");
  oled.setCursor(0, 9);
  oled.println("---------------------");
  oled.setCursor(0, 26);
  oled.println("Esperando conexion..");
  oled.display();

  while (!PS4.isConnected()) {
    if (DEBUG_STATE) {
      if (millis() > currentTimeState + TICK_DEBUG_STATE) {
        currentTimeState = millis();
        SerialBT.println("Esperando conexion..");
      }
    }
  }
  
  PS4.attach(sumo_rc_loop);
}

void loop() {
}
