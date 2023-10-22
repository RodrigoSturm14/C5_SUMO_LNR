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
#define DEBUG_STATE 0
#define DEBUG_ANALOG 0
#define TICK_DEBUG_STATE 1500
#define TICK_DEBUG_ANALOG 1500
#define TICK_DEBUG_SHARP 1500

#define TICK_GIRO_INICIO 50
#define TICK_GIRO_IZQUIERDA_45 200
#define TICK_GIRO_IZQUIERDA_90 500
#define TICK_GIRO_DERECHA_45 200
#define TICK_GIRO_DERECHA_90 500
#define TICK_INICIO 4900

unsigned long currentTimeSharp = 0;
unsigned long currentTimeState = 0;
unsigned long currentTimeAnalog = 0;

int analog;
//Oled
#define SCREEN_WIDTH 128  // OLED width,  in pixels
#define SCREEN_HEIGHT 64  // OLED height, in pixels

//Pines sensores de Distancia
#define PIN_SHARP_LEFT 25
#define PIN_SHARP_CENTER_LEFT 33
#define PIN_SHARP_CENTER_RIGHT 32
#define PIN_SHARP_RIGHT 35

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
// Variables seleccion de lado y giro predefinidos
#define CANT_MAX_ESTRATEGIAS 4
#define CANT_MIN_ESTRATEGIAS 1
int count_side = 0;
int count_degree = 0;

// Variables distancia de sensores sharp
#define DIST_LECTURA_MAX 60  // sami = 35
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

//configuramos el Serial Bluetooth
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;

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

//Button *stat = new  Button(PIN_PULSADOR_START_1);
//Button *strategy = new  Button(PIN_PULSADOR_ESTRATEGIA_2, false);
bool flank = HIGH;
bool previousState;
void SetFlank(bool f) {
  flank = f;
  previousState = !flank;
}

bool GetIsPress(int pin_pulsador) {
  /*
  bool actualState = digitalRead(PIN_PULSADOR_START_1);
  bool state = (previousState != actualState) && actualState == flank;
  previousState = actualState;
  delay(100);
  return state;
  */
  return digitalRead(pin_pulsador);
}


//AnalogSensor *ldrSensor = new AnalogSensor(PIN_SENSOR_LDR);

void sharpReadings() {
  distSharpCenterLeft = sharpCenterLeft->SharpDist();
  distSharpCenterRight = sharpCenterRight->SharpDist();
  distSharpLeft = sharpLeft->SharpDist();
  distSharpRight = sharpRight->SharpDist();
  // Tatami?
}
//Funciones para imprimir las lecturas de los sensores por el serial Bluetooth
/*
char printAnalog(char t)
{
  if (millis() > currentTimeAnalog + TICK_DEBUG_ANALOG)
  {
    currentTimeAnalog = millis();
    SerialBT.print(t + " :");
    SerialBT.println(analog);
  }
}
*/
void printReadSensors() {
  if (millis() > currentTimeSharp + TICK_DEBUG_SHARP) {
    currentTimeSharp = millis();
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

enum estrategia {
  GIRO_IZQUIERDA_45,
  GIRO_IZQUIERDA_90,
  GIRO_DERECHA_45,
  GIRO_DERECHA_90
};

void giroPredefinido() {
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
      Aldosivi->Left(VEL_MAX, VEL_MAX);
      delay(TICK_GIRO_DERECHA_45);
      break;

    case GIRO_DERECHA_90:
      Aldosivi->Left(VEL_MAX, VEL_MAX);
      delay(TICK_GIRO_DERECHA_90);
      break;
    
    /*default:
    break;*/
  }
}

//Enum de estados de movimiento de robot
enum movimiento {
  INICIO,
  BUSQUEDA_MEJORADA,
  CORRECCION_IZQUIERDA,
  CORRECCION_DERECHA,
  TE_ENCONTRE_IZQUIERDA,
  TE_ENCONTRE_DERECHA,
  ATAQUE
};
// Variable que determina el movimiento del robot
int movimiento = INICIO;

void printStatus() {
  if (millis() > currentTimeState + TICK_DEBUG_STATE) {
    currentTimeState = millis();
    String state = "";
    switch (movimiento) {
      case INICIO:
        state = "INICIO";
        break;
      case BUSQUEDA_MEJORADA:
        state = "BUSQUEDA_MEJORADA";
        break;
      case CORRECCION_IZQUIERDA:
        state = "CORRECCION_IZQUIERDA";
        break;
      case CORRECCION_DERECHA:
        state = "CORRECCION_DERECHA";
        break;
      case TE_ENCONTRE_IZQUIERDA:
        state = "TE_ENCONTRE_IZQUIERDA";
        break;
      case TE_ENCONTRE_DERECHA:
        state = "TE_ENCONTRE_DERECHA";
        break;
      case ATAQUE:
        state = "ATAQUE";
        break;
    }

    SerialBT.print("State: ");
    SerialBT.println(state);
    SerialBT.print("|| boton : ");
    SerialBT.println(GetIsPress(PIN_PULSADOR_START_1));
    SerialBT.println("---------");
  }
}

void switchCase() {
  switch (movimiento) {

    case INICIO:
      Aldosivi->Stop();
      // stateStart = GetIsPress();
      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setTextColor(WHITE);
      oled.setCursor(4, 0);
      oled.println("INICIO");
      oled.display();
      if (DEBUG_STATE) SerialBT.println("INICIO");
      delay(2000);

      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setTextColor(WHITE);
      oled.setCursor(4, 0);
      oled.println("Elegir estrategia");
      oled.setCursor(0, 9);
      oled.println("---------------------");
      oled.setCursor(0, 26);
      oled.println("Estrategia: ");
      oled.display();
      // seleccionar lado y grado de giro
      while (GetIsPress(PIN_PULSADOR_START_1) == true) {
        if (GetIsPress(PIN_PULSADOR_ESTRATEGIAS_2) == false) {
          count_estrategia++;
          if (count_estrategia > CANT_MAX_ESTRATEGIAS) {
            count_estrategia = CANT_MIN_ESTRATEGIAS;
          }

          if (DEBUG_STATE) {
            SerialBT.print("Estrategia: ");
            SerialBT.println(count_estrategia);
          }
          // actualizar contador en la oled
          oled.setCursor(0, 26);
          oled.print("Estrategia: ");
          oled.println(count_estrategia);
          oled.display();
          delay(500);
        }
      }

      if (DEBUG_STATE) {
        SerialBT.println("Pressed");
        SerialBT.print("Estrategia seleccionada: ");
        SerialBT.println(count_estrategia);
      }

      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setTextColor(WHITE);
      oled.setCursor(4, 0);
      oled.println("Pressed");
      oled.display();
      oled.setCursor(0, 9);
      oled.println("---------------------");
      oled.setCursor(0, 26);
      oled.print("Giro a usar: ");
      oled.println(count_estrategia);
      oled.display();

       while (GetIsPress(PIN_PULSADOR_START_1) == true) {
        if (DEBUG_STATE) {
          if (millis() > currentTimeState + TICK_DEBUG_STATE) {
            currentTimeState = millis();
            SerialBT.println("Esperando confirmacion..");
          }
        }
      }

      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setTextColor(WHITE);
      oled.setCursor(4, 0);
      oled.println("Pressed");
      oled.display();
      if (DEBUG_STATE) SerialBT.println("Pressed");
      
      delay(TICK_INICIO);
      oled.clearDisplay();
      oled.display();
      // hacer giro predefinido antes de busqueda mejorada
      giroPredefinido();

      // giro izquierda
      /*Aldosivi->Left(VEL_MAX, VEL_MAX);
      delay(TICK_GIRO_INICIO);*/
      movimiento = BUSQUEDA_MEJORADA;
      // movimiento = MODO;
      break;

    case BUSQUEDA_MEJORADA:
      // Busqueda sobre propio eje
      Aldosivi->Left(VEL_EJE_BUSQUEDA, VEL_EJE_BUSQUEDA);

      // Busqueda moverse en circulo
      /*
      Aldosivi->Forward(VEL_GIRO_BUSQUEDA_MEJORADA_DER, VEL_GIRO_BUSQUEDA_MEJORADA_IZQ);
      */
      if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX) movimiento = ATAQUE;
      else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX) movimiento = BUSQUEDA_MEJORADA;
      else if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX) movimiento = CORRECCION_IZQUIERDA;
      else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX) movimiento = CORRECCION_DERECHA;
      else if (distSharpLeft <= DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX) movimiento = TE_ENCONTRE_IZQUIERDA;
      else if (distSharpLeft > DIST_LECTURA_MAX && distSharpRight <= DIST_LECTURA_MAX) movimiento = TE_ENCONTRE_DERECHA;
      break;

    case CORRECCION_IZQUIERDA:
      Aldosivi->Forward(VEL_CORRECCION_IZQ_DER, VEL_CORRECCION_IZQ_IZQ);
      if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX) movimiento = ATAQUE;
      else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX) movimiento = BUSQUEDA_MEJORADA;
      else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX) movimiento = CORRECCION_DERECHA;
      else if (distSharpLeft <= DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX) movimiento = TE_ENCONTRE_IZQUIERDA;
      else if (distSharpLeft > DIST_LECTURA_MAX && distSharpRight <= DIST_LECTURA_MAX) movimiento = TE_ENCONTRE_DERECHA;
      break;

    case CORRECCION_DERECHA:
      Aldosivi->Right(VEL_CORRECCION_DER_DER, VEL_CORRECCION_DER_IZQ);
      if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX) movimiento = ATAQUE;
      else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX) movimiento = BUSQUEDA_MEJORADA;
      else if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX) movimiento = CORRECCION_IZQUIERDA;
      else if (distSharpLeft <= DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX) movimiento = TE_ENCONTRE_IZQUIERDA;
      else if (distSharpLeft > DIST_LECTURA_MAX && distSharpRight <= DIST_LECTURA_MAX) movimiento = TE_ENCONTRE_DERECHA;
      break;

    case TE_ENCONTRE_IZQUIERDA:
      /* Giro para cronometrar
      Aldosivi->Left(VEL_GIRO_BUSQUEDA, VEL_GIRO_BUSQUEDA);
      delay(1500);
      */
      do {
        Aldosivi->Left(VEL_GIRO, VEL_GIRO);
      } while (sharpCenterRight->SharpDist() > DIST_LECTURA_MAX);
      if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX) movimiento = ATAQUE;
      else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX) movimiento = BUSQUEDA_MEJORADA;
      else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX) movimiento = CORRECCION_IZQUIERDA;
      else if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX) movimiento = CORRECCION_DERECHA;
      else if (distSharpLeft > DIST_LECTURA_MAX && distSharpRight <= DIST_LECTURA_MAX) movimiento = TE_ENCONTRE_DERECHA;
      break;

    case TE_ENCONTRE_DERECHA:
      /* Giro para cronometrar
      Aldosivi->Right(VEL_GIRO, VEL_GIRO);
      delay(1500);
      */
      do {
        Aldosivi->Right(VEL_GIRO, VEL_GIRO);
      } while (sharpCenterLeft->SharpDist() > DIST_LECTURA_MAX);

      if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX) movimiento = ATAQUE;
      else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX) movimiento = BUSQUEDA_MEJORADA;
      else if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX) movimiento = CORRECCION_IZQUIERDA;
      else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX) movimiento = CORRECCION_DERECHA;
      else if (distSharpLeft <= DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX) movimiento = TE_ENCONTRE_IZQUIERDA;
      break;

    case ATAQUE:
      Aldosivi->Forward(VEL_MAX, VEL_MAX);
      if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX) movimiento = BUSQUEDA_MEJORADA;
      else if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX) movimiento = CORRECCION_IZQUIERDA;
      else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX) movimiento = CORRECCION_DERECHA;
      else if (distSharpLeft <= DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX) movimiento = TE_ENCONTRE_IZQUIERDA;
      else if (distSharpLeft > DIST_LECTURA_MAX && distSharpRight <= DIST_LECTURA_MAX) movimiento = TE_ENCONTRE_DERECHA;
      break;
      /*
    case 9:
      do {
        leftmotor->Backward(180);
        rightmotor->Forward(180);
      } while (sharpCenterRight->SharpDist() > DIST_LECTURA_MAX);

      leftmotor->Forward(180);
      rightmotor->Forward(180);
      break;

    case 15:
      do {
        leftmotor->Forward(180);
        rightmotor->Backward(180);
      } while (sharpCenterLeft->SharpDist() > DIST_LECTURA_MAX);

      leftmotor->Forward(180);
      rightmotor->Forward(180);
      break;

    case 16:
      // Busqueda del robot
      leftmotor->Forward(175);
      rightmotor->Forward(100);
      break;
*/
  }
}
void setup() {
  Serial.begin(115200);
  SerialBT.begin("Aldosivi");
  Wire.begin();
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(PIN_PULSADOR_START_1, INPUT_PULLUP);
}

void loop() {
  // Lectura de sharps
  sharpReadings();
  // Seleccion del movimiento del robot
  switchCase();

  if (DEBUG_SHARP) {
    printReadSensors();
  }

  if (DEBUG_STATE) {
    printStatus();
  }

  /*if (DEBUG_ANALOG)
  {
    printAnalog();
  }
  */
}
