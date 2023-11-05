// Librerias
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>  //Oled
#include <EngineController.h>  //Motores
#include <AnalogSensor.h>      //libreria para sensores analogicos( sensores tatami)
#include <DistanceSensors.h>   //libreria para sensores
// #include <Button.h>
#include "BluetoothSerial.h"  //Bluetooh

// debug
#define DEBUG_SHARP 1
#define DEBUG_STATE 1
#define DEBUG_ANALOG 0
#define TICK_DEBUG_STATE 800
#define TICK_DEBUG_ANALOG 800
#define TICK_DEBUG_SHARP 800

#define TICK_BUTTON_STATE 600
#define TICK_GIRO_INICIO 50
#define TICK_INICIO 4950

unsigned long currentTimeSharp = 0;
unsigned long currentTimeState = 0;
unsigned long currentTimeAnalog = 0;

int analog;
// Oled
#define SCREEN_WIDTH 128  // OLED width,  in pixels
#define SCREEN_HEIGHT 64  // OLED height, in pixels

// Pines sensores de Distancia
#define PIN_SHARP_LEFT 25
#define PIN_SHARP_CENTER_LEFT 33
#define PIN_SHARP_CENTER 34
#define PIN_SHARP_CENTER_RIGHT 32
#define PIN_SHARP_RIGHT 35

// Pines motores y canales PWM
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

#define TICK_GIRO_IZQUIERDA_45 80    // 100
#define TICK_GIRO_IZQUIERDA_90 110   // [130] buen giro a 90
#define TICK_GIRO_IZQUIERDA_135 190  // 180

#define TICK_GIRO_DERECHA_45 110   // 90   ---> los giros hacia la derecha se pueden pasar de los grados para que busqueda_mejorada los corrija hacia la izquierda
#define TICK_GIRO_DERECHA_90 140   // [130] buen giro a 90
#define TICK_GIRO_DERECHA_135 200  // 180
#define TICK_ADELANTE 160          // 120

#define TICK_START 1000
#define MAX_MODE 8
#define MIN_MODE 0
bool stateStart = 1;
unsigned long currentTimeButton = 0;
int count_estrategia = 0;
String estrategia = "";

// Variables de pantalla OLED
#define SCREEN_WIDTH 128  // OLED width,  in pixels
#define SCREEN_HEIGHT 64  // OLED height, in pixels
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Velocidades Sumo
#define VEL_MAX 200
#define VEL_GIRO 145
#define VEL_EJE_BUSQUEDA 145
#define VEL_CORRECCION_IZQ_IZQ 190
#define VEL_CORRECCION_IZQ_DER 200
#define VEL_CORRECCION_DER_IZQ 200
#define VEL_CORRECCION_DER_DER 190
#define VEL_GIRO_BUSQUEDA_MEJORADA_IZQ 90  // 130
#define VEL_GIRO_BUSQUEDA_MEJORADA_DER 80  // 100
// Variables distancia de sensores sharp
#define DIST_LECTURA_MAX 40  // sami = 35
int distSharpCenterLeft = 0;
int distSharpCenter = 0;
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
  double distancia_cm = 187754 * pow(adc, -1.183);  // REGULAR LA POTENCIA PARA OBETENER BUENA PRESICION
  return (distancia_cm);
  delay(100);
}

// configuramos el Serial Bluetooth
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;

// --- Sensores Centro
Sharp *sharpCenterLeft = new Sharp(PIN_SHARP_CENTER_LEFT);
Sharp *sharpCenter = new Sharp(PIN_SHARP_CENTER);
Sharp *sharpCenterRight = new Sharp(PIN_SHARP_CENTER_RIGHT);
// --- Sensores Costados
Sharp *sharpLeft = new Sharp(PIN_SHARP_LEFT);
Sharp *sharpRight = new Sharp(PIN_SHARP_RIGHT);

// Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

IEngine *rightEngine = new Driver_DRV8825(PIN_MOTOR_DERECHO_1, PIN_MOTOR_DERECHO_2, CANAL_PWM_DERECHO_1, CANAL_PWM_DERECHO_2);
IEngine *leftEngine = new Driver_DRV8825(PIN_MOTOR_IZQUIERDO_1, PIN_MOTOR_IZQUIERDO_2, CANAL_PWM_IZQUIERDO_1, CANAL_PWM_IZQUIERDO_2);
EngineController *Aldosivi = new EngineController(rightEngine, leftEngine);

// AnalogSensor *rightTatami = new AnalogSensor(PIN_SENSOR_TATAMI_DER);
// AnalogSensor *LeftTatami = new AnalogSensor(PIN_SENSOR_TATAMI_IZQ);

// Isensor *sharpRight = new Sharp_GP2Y0A02(PIN_SENSOR_DISTANCIA_DERECHO);
// Isensor *sharpLeft = new Sharp_GP2Y0A02(PIN_SENSOR_DISTANCIA_IZQUIERDO);

// Button *stat = new  Button(PIN_PULSADOR_START_1);
// Button *strategy = new  Button(PIN_PULSADOR_ESTRATEGIA_2, false);
bool flank = HIGH;
bool previousState;
void SetFlank(bool f) {
  flank = f;
  previousState = !flank;
}

bool GetIsPress(int pin) {
  /*
  bool actualState = digitalRead(PIN_PULSADOR_START_1);
  bool state = (previousState != actualState) && actualState == flank;
  previousState = actualState;
  delay(100);
  return state;
  */
  return digitalRead(pin);
}

// AnalogSensor *ldrSensor = new AnalogSensor(PIN_SENSOR_LDR);

void sharpReadings() {
  distSharpCenterLeft = sharpCenterLeft->SharpDist();
  distSharpCenter = sharpCenter->SharpDist();
  distSharpCenterRight = sharpCenterRight->SharpDist();
  distSharpLeft = sharpLeft->SharpDist();
  distSharpRight = sharpRight->SharpDist();
  // Tatami?
}
// Funciones para imprimir las lecturas de los sensores por el serial Bluetooth
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
    SerialBT.print("Center Distance: ");
    SerialBT.println(distSharpCenter);
    SerialBT.print("right Center Distance: ");
    SerialBT.println(distSharpCenterRight);
    SerialBT.print("right Distance: ");
    SerialBT.println(distSharpRight);
    SerialBT.println("-----------------------");
  }
}

// Enum de estados de movimiento de robot
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

enum movimietos_predefinidos {
  NONE,
  GIRO_IZQUIERDA_45,
  GIRO_IZQUIERDA_90,
  GIRO_IZQUIERDA_135,
  GIRO_DERECHA_45,
  GIRO_DERECHA_90,
  GIRO_DERECHA_135,
  MESSI_IZQ,
  MESSI_DER
};

void printEstrategia() {
  estrategia = "";
  switch (count_estrategia) {
    case NONE:
      estrategia = "NONE";
      break;
    case GIRO_IZQUIERDA_45:
      estrategia = "IZQ-45";
      break;
    case GIRO_IZQUIERDA_90:
      estrategia = "IZQ-90";
      break;
    case GIRO_IZQUIERDA_135:
      estrategia = "IZQ-135";
      break;
    case GIRO_DERECHA_45:
      estrategia = "DER-45";
      break;
    case GIRO_DERECHA_90:
      estrategia = "DER-90";
      break;
    case GIRO_DERECHA_135:
      estrategia = "DER-135";
      break;
    case MESSI_IZQ:  // AMAGUE_ADELANTE_IZQUIERDA
      estrategia = "MESSI-IZQ";
      break;
    case MESSI_DER:  // AMAGUE_ADELANTE_DERECHA
      estrategia = "MESSI-DER";
      break;
  }
  if (DEBUG_STATE) {
    SerialBT.print("Estrategia: ");
    SerialBT.println(estrategia);
    SerialBT.println("---------");
  }
}

void movimientoPredefinido() {
  switch (count_estrategia) {
    case NONE:
      movimiento = BUSQUEDA_MEJORADA;
      break;
    case GIRO_IZQUIERDA_45:
      Aldosivi->Left(VEL_MAX, VEL_MAX);
      delay(TICK_GIRO_IZQUIERDA_45);
      break;
    case GIRO_IZQUIERDA_90:
      Aldosivi->Left(VEL_MAX, VEL_MAX);
      delay(TICK_GIRO_IZQUIERDA_90);
      break;
    case GIRO_IZQUIERDA_135:
      Aldosivi->Left(VEL_MAX, VEL_MAX);
      delay(TICK_GIRO_IZQUIERDA_135);
      break;
    case GIRO_DERECHA_45:
      Aldosivi->Right(VEL_MAX, VEL_MAX);
      delay(TICK_GIRO_DERECHA_45);
      break;
    case GIRO_DERECHA_90:
      Aldosivi->Right(VEL_MAX, VEL_MAX);
      delay(TICK_GIRO_DERECHA_90);
      break;
    case GIRO_DERECHA_135:
      Aldosivi->Right(VEL_MAX, VEL_MAX);
      delay(TICK_GIRO_DERECHA_135);
      break;
    case MESSI_IZQ:
      Aldosivi->Forward(VEL_MAX, VEL_MAX);
      delay(TICK_ADELANTE);
      Aldosivi->Left(VEL_MAX, VEL_MAX);
      delay(210);
      break;
    case MESSI_DER:
      Aldosivi->Forward(VEL_MAX, VEL_MAX);
      delay(TICK_ADELANTE);
      Aldosivi->Right(VEL_MAX, VEL_MAX);
      delay(220);
      break;
  }
  if (DEBUG_STATE) {
    printEstrategia();
  }
}

void switchCase() {
  switch (movimiento) {

    case INICIO:
      {
        Aldosivi->Stop();
        oled.clearDisplay();
        oled.setTextSize(1);
        oled.setTextColor(WHITE);
        oled.setCursor(4, 0);
        oled.println("INICIO");
        oled.display();

        while (GetIsPress(PIN_PULSADOR_START_1) == true) {

          if (DEBUG_SHARP) {
            sharpReadings();
            printReadSensors();
          }
          if (digitalRead(PIN_PULSADOR_ESTRATEGIA_2) == true) {
            if (millis() > currentTimeState + TICK_BUTTON_STATE) {
              currentTimeState = millis();
              count_estrategia++;
              if (count_estrategia > MAX_MODE)
                count_estrategia = MIN_MODE;
              printEstrategia();
              oled.clearDisplay();
              oled.setCursor(0, 9);
              oled.println("---------------------");
              oled.setCursor(0, 26);
              oled.print("Estrategia: ");
              oled.println(estrategia);
              oled.display();
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
        oled.println(estrategia);
        oled.display();

        if (DEBUG_STATE) SerialBT.println("Pressed");
        delay(TICK_INICIO);
        oled.clearDisplay();
        oled.display();

        movimientoPredefinido();
        /*
    Aldosivi->Left(VEL_MAX, VEL_MAX);
    delay(TICK_GIRO_INICIO);
    */
        movimiento = BUSQUEDA_MEJORADA;

        break;
      }

    case BUSQUEDA_MEJORADA:
      {
        Aldosivi->Left(VEL_EJE_BUSQUEDA, VEL_EJE_BUSQUEDA);

        // Busqueda moverse en circulo
        /*
    Aldosivi->Forward(VEL_GIRO_BUSQUEDA_MEJORADA_DER, VEL_GIRO_BUSQUEDA_MEJORADA_IZQ);
    */
        if (distSharpCenter <= DIST_LECTURA_MAX)
          movimiento = ATAQUE;
        // else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenter > DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX)
        // movimiento = BUSQUEDA_MEJORADA;
        if (distSharpCenter > DIST_LECTURA_MAX) {
          if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX)
            movimiento = CORRECCION_IZQUIERDA;
          else if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX)
            movimiento = CORRECCION_DERECHA;
          else if (distSharpLeft <= DIST_LECTURA_MAX)
            movimiento = TE_ENCONTRE_IZQUIERDA;
          else if (distSharpRight <= DIST_LECTURA_MAX)
            movimiento = TE_ENCONTRE_DERECHA;
        }

        break;
      }

    case CORRECCION_IZQUIERDA:
      {
        // Aldosivi->Forward(VEL_CORRECCION_IZQ_DER, VEL_CORRECCION_IZQ_IZQ);
        Aldosivi->Left(VEL_EJE_BUSQUEDA, VEL_EJE_BUSQUEDA);
        if (distSharpCenter <= DIST_LECTURA_MAX)
          movimiento = ATAQUE;
        if (distSharpCenter > DIST_LECTURA_MAX) {
          if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX)
            movimiento = BUSQUEDA_MEJORADA;
          else if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX)
            movimiento = CORRECCION_DERECHA;
          else if (distSharpRight > DIST_LECTURA_MAX && distSharpLeft <= DIST_LECTURA_MAX)
            movimiento = TE_ENCONTRE_IZQUIERDA;
          else if (distSharpRight <= DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX)
            movimiento = TE_ENCONTRE_DERECHA;
        }
        break;
      }

    case CORRECCION_DERECHA:
      {
        // Aldosivi->Forward(VEL_CORRECCION_DER_DER, VEL_CORRECCION_DER_IZQ);
        while (sharpCenter->SharpDist() > DIST_LECTURA_MAX) {
          Aldosivi->Right(VEL_EJE_BUSQUEDA, VEL_EJE_BUSQUEDA);
          sharpReadings();
          if (DEBUG_STATE)
            printStatus();
          if (DEBUG_SHARP)
            printReadSensors();
        }

        if (distSharpCenter <= DIST_LECTURA_MAX) movimiento = ATAQUE;
        if (distSharpCenter > DIST_LECTURA_MAX) {
          if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX) movimiento = BUSQUEDA_MEJORADA;
          else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX) movimiento = CORRECCION_IZQUIERDA;
          else if (distSharpRight > DIST_LECTURA_MAX && distSharpLeft <= DIST_LECTURA_MAX) movimiento = TE_ENCONTRE_IZQUIERDA;
          else if (distSharpRight <= DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX) movimiento = TE_ENCONTRE_DERECHA;
        }
        break;
      }

    case TE_ENCONTRE_IZQUIERDA:
      {
        while (sharpCenter->SharpDist() > DIST_LECTURA_MAX) {
          Aldosivi->Left(VEL_GIRO, VEL_GIRO);
          sharpReadings();
          if (DEBUG_STATE)
            printStatus();
          if (DEBUG_SHARP)
            printReadSensors();
        }

        if (distSharpCenter <= DIST_LECTURA_MAX) movimiento = ATAQUE;
        //else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenter > DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX)
        //  movimiento = BUSQUEDA_MEJORADA;
        //else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX)
        //  movimiento = CORRECCION_IZQUIERDA;
        //else if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX)
        //  movimiento = CORRECCION_DERECHA;
        //else if ( distSharpRight <= DIST_LECTURA_MAX)
        //  movimiento = TE_ENCONTRE_DERECHA;
        break;
      }

    case TE_ENCONTRE_DERECHA:
      {
        while (sharpCenter->SharpDist() > DIST_LECTURA_MAX) {
          Aldosivi->Right(VEL_GIRO, VEL_GIRO);
          sharpReadings();
          if (DEBUG_STATE)
            printStatus();
          if (DEBUG_SHARP)
            printReadSensors();
        }

        if (distSharpCenter <= DIST_LECTURA_MAX) movimiento = ATAQUE;
        //else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX)
        //  movimiento = CORRECCION_IZQUIERDA;
        //else if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX)
        //  movimiento = CORRECCION_DERECHA;
        //else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenter > DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX)
        //  movimiento = BUSQUEDA_MEJORADA;
        //else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX)
        //  movimiento = CORRECCION_IZQUIERDA;
        //else if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX)
        //  movimiento = CORRECCION_DERECHA;
        //else if (distSharpLeft <= DIST_LECTURA_MAX)
        //  movimiento = TE_ENCONTRE_IZQUIERDA;
        break;
      }

    case ATAQUE:
      {
        // Aldosivi->Forward(VEL_MAX, VEL_MAX);
        if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenter > DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX)
          movimiento = BUSQUEDA_MEJORADA;
        else if (distSharpCenterLeft > DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX)
          movimiento = CORRECCION_IZQUIERDA;
        else if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX)
          movimiento = CORRECCION_DERECHA;
        else if (distSharpLeft <= DIST_LECTURA_MAX)
          movimiento = TE_ENCONTRE_IZQUIERDA;
        else if (distSharpRight <= DIST_LECTURA_MAX)
          movimiento = TE_ENCONTRE_DERECHA;
        break;
      }
  }
}
void setup() {
  Serial.begin(115200);
  SerialBT.begin("Aldosivi");
  Wire.begin();
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(PIN_PULSADOR_START_1, INPUT_PULLUP);
  pinMode(PIN_PULSADOR_ESTRATEGIA_2, INPUT_PULLDOWN);
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
