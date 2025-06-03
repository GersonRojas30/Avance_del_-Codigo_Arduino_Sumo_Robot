#include <AFMotor.h>

#define Pulsador 16
#define Pulsador2 17

// Pines para sensor ultrasónico delantero
const int triggerPinDel = 10;
const int echoPinDel = 9;

// Pines para sensor ultrasónico trasero
const int triggerPinTras = 8;
const int echoPinTras = 7;

long distanciaDel = 0;
long distanciaTras = 0;

int estadoAC = 0;

AF_DCMotor motorA(1);
AF_DCMotor motorB(2);
AF_DCMotor motorC(3);
AF_DCMotor motorD(4);

// Variables para control sin bloqueo
unsigned long actionStartMillis = 0;
int duracionAccion = 0;

enum EstadoMovimiento {
  DETENER,
  AVANZAR,
  GIRAR_IZQUIERDA,
  GIRAR_DERECHA,
  RETROCEDER
};

EstadoMovimiento estadoActual = DETENER;
int velocidadActual = 0;

void setup() {
  Serial.begin(9600);
  pinMode(triggerPinDel, OUTPUT);
  pinMode(echoPinDel, INPUT);
  pinMode(triggerPinTras, OUTPUT);
  pinMode(echoPinTras, INPUT);
  pinMode(14, INPUT);  // Sensor línea derecha
  pinMode(15, INPUT);  // Sensor línea izquierda
  pinMode(Pulsador, INPUT);
  pinMode(Pulsador2, OUTPUT);
}

long medirDistancia(int triggerPin, int echoPin) {
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  long duracion = pulseIn(echoPin, HIGH, 30000); // Timeout 30ms para evitar bloqueo
  long distanciaCm = duracion > 0 ? (duracion / 2) / 29.154 : 999; // 999 = no detectado
  return distanciaCm;
}

void avanzar(int velocidad) {
  motorA.setSpeed(velocidad);
  motorA.run(FORWARD);
  motorB.setSpeed(velocidad);
  motorB.run(FORWARD);
  motorC.setSpeed(velocidad);
  motorC.run(FORWARD);
  motorD.setSpeed(velocidad);
  motorD.run(FORWARD);
  velocidadActual = velocidad;
  estadoActual = AVANZAR;
  actionStartMillis = millis();
}

void retroceder(int velocidad) {
  motorA.setSpeed(velocidad);
  motorA.run(BACKWARD);
  motorB.setSpeed(velocidad);
  motorB.run(BACKWARD);
  motorC.setSpeed(velocidad);
  motorC.run(BACKWARD);
  motorD.setSpeed(velocidad);
  motorD.run(BACKWARD);
  velocidadActual = velocidad;
  estadoActual = RETROCEDER;
  actionStartMillis = millis();
}

void girarIzquierda(int velocidad) {
  motorA.setSpeed(velocidad);
  motorA.run(BACKWARD);
  motorB.setSpeed(velocidad);
  motorB.run(BACKWARD);
  motorC.setSpeed(velocidad);
  motorC.run(FORWARD);
  motorD.setSpeed(velocidad);
  motorD.run(FORWARD);
  velocidadActual = velocidad;
  estadoActual = GIRAR_IZQUIERDA;
  actionStartMillis = millis();
}

void girarDerecha(int velocidad) {
  motorA.setSpeed(velocidad);
  motorA.run(FORWARD);
  motorB.setSpeed(velocidad);
  motorB.run(FORWARD);
  motorC.setSpeed(velocidad);
  motorC.run(BACKWARD);
  motorD.setSpeed(velocidad);
  motorD.run(BACKWARD);
  velocidadActual = velocidad;
  estadoActual = GIRAR_DERECHA;
  actionStartMillis = millis();
}

void detener() {
  motorA.setSpeed(0);
  motorA.run(RELEASE);
  motorB.setSpeed(0);
  motorB.run(RELEASE);
  motorC.setSpeed(0);
  motorC.run(RELEASE);
  motorD.setSpeed(0);
  motorD.run(RELEASE);
  estadoActual = DETENER;
  velocidadActual = 0;
}

void loop() {
  estadoAC = digitalRead(Pulsador);

  if (estadoAC == 1) {
    digitalWrite(Pulsador2, HIGH);

    // Leer sensores ultrasónicos uno por uno para evitar interferencia
    distanciaDel = medirDistancia(triggerPinDel, echoPinDel);
    delay(30); // pequeño retardo para evitar interferencia
    distanciaTras = medirDistancia(triggerPinTras, echoPinTras);

    unsigned long currentMillis = millis();

    // Revisar estado actual y duración
    switch (estadoActual) {
      case AVANZAR:
        if (currentMillis - actionStartMillis >= 100) {
          if (distanciaDel > 14) {
            girarDerecha(100);
          } else {
            avanzar(255);
            actionStartMillis = currentMillis;
          }
        }
        break;

      case GIRAR_DERECHA:
        if (currentMillis - actionStartMillis >= 300) {
          avanzar(255);
          actionStartMillis = currentMillis;
        }
        break;

      case RETROCEDER:
        if (currentMillis - actionStartMillis >= duracionAccion) {
          if (!digitalRead(14)) {
            girarIzquierda(255);
            duracionAccion = 600;
          } else if (!digitalRead(15)) {
            girarDerecha(255);
            duracionAccion = 600;
          } else {
            avanzar(255);
            duracionAccion = 0;
          }
          actionStartMillis = currentMillis;
        }
        break;

      case GIRAR_IZQUIERDA:
        if (currentMillis - actionStartMillis >= 600) {
          avanzar(255);
          actionStartMillis = currentMillis;
        }
        break;

      case DETENER:
      default:
        break;
    }

    // Sensores línea para evitar bordes
    if (!digitalRead(14) || !digitalRead(15)) {
      retroceder(255);
      duracionAccion = 400;
      actionStartMillis = currentMillis;
    }

    // También revisar sensor trasero para evitar chocar al retroceder
    if (distanciaTras <= 14 && estadoActual == RETROCEDER) {
      // Si hay obstáculo atrás cuando retrocede, girar para evitar
      girarIzquierda(255);
      duracionAccion = 600;
      actionStartMillis = currentMillis;
    }

    // Si no hay acción, iniciar avanzar o girar según sensor delantero
    if (estadoActual == DETENER) {
      if (distanciaDel <= 14) {
        avanzar(255);
        duracionAccion = 100;
      } else {
        girarDerecha(100);
        duracionAccion = 300;
      }
      actionStartMillis = currentMillis;
    }

    Serial.print("Delante: ");
    Serial.print(distanciaDel);
    Serial.print(" cm | Atrás: ");
    Serial.print(distanciaTras);
    Serial.println(" cm");

  } else {
    digitalWrite(Pulsador2, LOW);
    detener();
  }
}