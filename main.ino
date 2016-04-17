/* ---------------------------------------------------------------------------
** This software controls two ANNAX 41.4301.0470 LED Panel (total 192x16 LEDs)
** This software will not work on ATmega328/168/8 boards. It is meant to be
** run on an ESP8266
**
** Reverse Engineering has been done to identify the connecting pins.
** The PCB pin numbering refers to the female connector's pin numbering.
**
** Author: Fabian Lüpke
** -------------------------------------------------------------------------*/

#include "SPI.h"

/* Wiring
 *  
 *  PCB  Meaning   Wemos
 *  16   +5V       +3V3
 *  15   +5V       +3V3
 *  14   SRCK      D5
 *  13   GND       GND
 *  12   RCK       D6
 *  11   GND       GND
 *  10   ???
 *   9   OUT_E     D0
 *   8   A0        D1
 *   7   A1        D2
 *   6   A2        D3
 *   5   A3        D4
 *   4   NC
 *   3   NC
 *   2   SEROUT
 *   1   SERIN     D7
 */

uint8_t framebuffer[24][16];
unsigned long temp_time;

#define pulse(pin) ({ digitalWrite(pin, LOW); digitalWrite(pin, HIGH); })

#define RCK   D8
#define OUT_E D0
#define A0    D1
#define A1    D2
#define A2    D3
#define A3    D4

void setup() {
  pinMode(RCK, OUTPUT);
  pinMode(OUT_E, OUTPUT);
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  SPI.begin();
  SPI.setFrequency(30000000); // The ESP can do more, but the shift registers in the display can't
  // test
  framebuffer[23][0] = 0b10101010;
  framebuffer[23][10] = 255;
}

void loop() {
  for (byte y=0; y<16; y++) {
    digitalWrite(OUT_E, HIGH);
    digitalWrite(A0, y & 1);
    digitalWrite(A1, y & 2);
    digitalWrite(A2, y & 4);
    digitalWrite(A3, y & 8);

    for (int x=0; x<24; x++) {
      SPI.transfer(framebuffer[x][y]);
    }

    pulse(RCK);
    digitalWrite(OUT_E, LOW);
    temp_time = micros();
    render();
    while (temp_time + 100 > micros()) { }
  }
}

void render() {
  
}
