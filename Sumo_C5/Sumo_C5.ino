//Librerias


#define PIN_SHARP_LEFT 25
#define PIN_SHARP_CENTER_LEFT 33
#define PIN_SHARP_CENTER_RIGHT 32
#define PIN_SHARP_RIGHT 35

//#define SHARP_1 35  // LEFT SIDE
//#define SHARP_2 32  // LEFT
//#define SHARP_3 26  // CNTER
//#define SHARP_4 27  // RIGHT
//#define SHARP_5 25  // RIGHT SIDE

// Variables limites para lectura de sharps
//#define DISTANCIA_MIN 0
//#define DISTANCIA_MAX 35

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
int movimiento = 0;

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

class Motor {
private:
  // atributos/variables usados por los metodos/funciones de la clase
  int pin_a;
  int pin_b;
  int ch_a;
  int ch_b;
  int frecuencia = 1000;
  int resolucion = 8;

public:
  // metodos/funciones
  Motor(int pin_a, int pin_b, int ch_a, int ch_b);
  void Forward(int vel);
  void Backward(int vel);
  void Stop();
};

// Constructor
Motor::Motor(int pin_a_in, int pin_b_in, int ch_a_in, int ch_b_in) {
  pin_a = pin_a_in;
  pin_b = pin_b_in;
  ch_a = ch_a_in;
  ch_b = ch_b_in;

  ledcSetup(ch_a, frecuencia, resolucion);
  ledcSetup(ch_b, frecuencia, resolucion);
  ledcAttachPin(pin_a, ch_a);
  ledcAttachPin(pin_b, ch_b);
}

// Metodos motores
void Motor::Forward(int vel) {
  ledcWrite(ch_a, vel);
  ledcWrite(ch_b, 0);
}
void Motor::Backward(int vel) {
  ledcWrite(ch_a, 0);
  ledcWrite(ch_b, vel);
}
void Motor::Stop() {
  ledcWrite(ch_a, 0);
  ledcWrite(ch_b, 0);
}

//Declaracion de objetos motores por constructor
Motor *leftmotor = new Motor(PIN_MOTOR_IZQUIERDO_1, PIN_MOTOR_IZQUIERDO_2, CANAL_PWM_IZQUIERDO_1, CANAL_PWM_IZQUIERDO_2);
Motor *rightmotor = new Motor(PIN_MOTOR_DERECHO_1, PIN_MOTOR_DERECHO_2, CANAL_PWM_DERECHO_1, CANAL_PWM_DERECHO_2);
// --- Sensores Centro
Sharp *sharpCenterLeft = new Sharp(PIN_SHARP_CENTER_LEFT);
Sharp *sharpCenterRight = new Sharp(PIN_SHARP_CENTER_RIGHT);
// --- Sensores Costados
Sharp *sharpLeft = new Sharp(PIN_SHARP_LEFT);
Sharp *sharpRight = new Sharp(PIN_SHARP_RIGHT);

float sharpReadings() {
  distSharpCenterLeft = sharpCenterLeft->SharpDist();
  distSharpCenterRight = sharpCenterRight->SharpDist();

  distSharpLeft = sharpLeft->SharpDist();
  distSharpRight = sharpRight->SharpDist();
  // Tatami?
}

int estadoMaquina() {
  // Atacar cuando los sensores del centro detectan rival
  if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight <= DIST_LECTURA_MAX) movimiento = 1;
  // Atacar y girar un poco a la izquierda cuando el sensor izquierdo detecta el rival
  else if (distSharpCenterLeft <= DIST_LECTURA_MAX && distSharpCenterRight > DIST_LECTURA_MAX) movimiento = 2;
  // Atacar y girar un poco a la derecha cuando el sensor derecho detecta el rival
  else if (distSharpCenterRight <= DIST_LECTURA_MAX && distSharpCenterLeft > DIST_LECTURA_MAX) movimiento = 3;
  // Girar a la izquierda cuando el sensor izquierdo detecta rival hasta que el sensor centro derecho detecte el rival
  else if (distSharpLeft <= DIST_LECTURA_MAX && distSharpRight > DIST_LECTURA_MAX) movimiento = 4;
  // Girar a la derecha cuando el sensor derecho detecta rival hasta que el sensor centro izquierdo detecte el rival
  else if (distSharpRight <= DIST_LECTURA_MAX && distSharpLeft > DIST_LECTURA_MAX) movimiento = 5;
  // Busqueda del robot
  else movimiento = 6;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Estrategia: Sumo ataque");
}

void loop() {
  // Lectura de sharps
  sharpReadings();
  // Seleccion del movimiento del robot
  estadoMaquina();

  switch (movimiento) {
    case 1:
      leftmotor->Forward(200);
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
