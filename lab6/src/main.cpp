#undef __INT24_MAX__
#include <Arduino.h>

#define PIN_RED_CLK   2  // Ribbon Pin 1
#define PIN_ROW_RESET 3  // Ribbon Pin 2
#define PIN_GRN_DAT   4  // Ribbon Pin 3
#define PIN_LATCH     5  // Ribbon Pin 4
#define PIN_RED_DAT   6  // Ribbon Pin 5
#define PIN_GRN_CLK   7  // Ribbon Pin 6

void setup() {
  pinMode(PIN_RED_CLK, OUTPUT);
  pinMode(PIN_ROW_RESET, OUTPUT);
  pinMode(PIN_GRN_DAT, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_RED_DAT, OUTPUT);
  pinMode(PIN_GRN_CLK, OUTPUT);
}

void loop() {
  // 1. Force the Row Token / Blanking Pin LOW (Active)
  // This should theoretically unblank the screen and feed a '1' into the 164
  digitalWrite(PIN_ROW_RESET, LOW);

  // 2. Blast 1s (ON) into both Red and Green data lines
  digitalWrite(PIN_RED_DAT, HIGH);
  digitalWrite(PIN_GRN_DAT, HIGH);

  // 3. Rapidly clock the data and latch it continuously
  // We aren't counting to 480 or multiplexing 14 rows. 
  // We are just violently shoving data into the registers.
  digitalWrite(PIN_RED_CLK, HIGH);
  digitalWrite(PIN_GRN_CLK, HIGH);

  digitalWrite(PIN_LATCH, HIGH);

  digitalWrite(PIN_RED_CLK, LOW);
  digitalWrite(PIN_GRN_CLK, LOW);

  digitalWrite(PIN_LATCH, LOW);

  // A tiny delay just to prevent locking up the Uno
  delayMicroseconds(10);
}

