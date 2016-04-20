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

// Disclaimer: This code is extremely hacky.

extern "C" {
  #include "user_interface.h"
}
#include "SPI.h"
#include "font.h"

/* Wiring
 *  
 *  PCB  Meaning   Wemos
 *  16   +5V       +3V3
 *  15   +5V       +3V3
 *  14   SRCK      D5
 *  13   GND       GND
 *  12   RCK       D8
 *  11   GND       GND
 *  10   ???
 *   9   OUT_E     D0
 *   8   A0        D1
 *   7   A1        D2
 *   6   A2        D3
 *   5   A3        D4
 *   4   NC
 *   3   NC
 *   2   SEROUT    Could be used to determine the number of boards attached
 *   1   SERIN     D7
 */
 
uint8_t framebuffer[24][16];

// Pin definitions
#define RCK   D8
#define OUT_E D0
#define A0    D1
#define A1    D2
#define A2    D3
#define A3    D4

// Timer stuff
#define TIMER_FREQ 5000000
#define DISPLAY_FREQ 1000
#define TIMER_TICKS TIMER_FREQ / DISPLAY_FREQ
uint8_t scanLine = 0;

// Macros
#define pulse(pin) ({ digitalWrite(pin, HIGH); digitalWrite(pin, LOW); })
#define output_on() ( GPOS = (1 << OUT_E) )
#define output_off() ( GPOC = (1 << OUT_E) );

/**
 * Setups the whole stuff
 */
void setup() {
  setupOutputs();
  setupSPI();
  clearPixels();
  setupInterrupts();
  Serial.begin(115200);

  // test
  renderString("Hey there!\0", 96);

  dumpFrameToSerial();
}

/**
 * Configure necessary pins as outputs
 */
void setupOutputs() {
  pinMode(RCK, OUTPUT);
  pinMode(OUT_E, OUTPUT);
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
}

/**
 * Configure SPI
 */
void setupSPI() {
  SPI.begin();
  SPI.setFrequency(30000000);
}

void setupInterrupts() {
  timer1_isr_init();
  timer1_attachInterrupt(&interruptHandler);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
  timer1_write(TIMER_TICKS);
}

/**
 * Main loop
 */
void loop() {
  // Do nothing!
}

/**
 * Clears all pixels
 */
void clearPixels() {
  for (int x=0; x<24; x++) {
    for (int y=0; y<16; y++) {
      framebuffer[x][y] = 0;
    }
  }
}

void interruptHandler() {
  pulse(RCK);
  output_off();
  digitalWrite(A0, (scanLine - 1) & 1);
  digitalWrite(A1, (scanLine - 1) & 2);
  digitalWrite(A2, (scanLine - 1) & 4);
  digitalWrite(A3, (scanLine - 1) & 8);

  for (int x=0; x<24; x++) {
    SPI.transfer(framebuffer[x][scanLine]);
  }
  output_on();
  scanLine++;
  if (scanLine >= 16) scanLine = 0;
}
void setPixel(uint8_t x, uint8_t y) {
  framebuffer[x >> 3][y] |= 1 << (x % 8);
}

void clearPixel(uint8_t x, uint8_t y) {
  framebuffer[x >> 3][y] &= ~(1 << (x % 8));
}

void renderChar(char c, uint8_t x) {
  uint32_t start = (c - 32) * 32;
  Serial.print("Rendering ");
  Serial.println(c);
  
  /*
   * 3 possibilities
   * 
   * 1. Perfectly aligned
   * x x x x x x x x   X X X
   * 
   * 2. Spreading two bytes
   *           x x x   x x x  x  x  X  X  X
   *           
   * 3. Spreading three bytes
   *             x x   x x x  x  x  x  X  X    X 
   * 0 1 2 3 4 5 6 7 | 8 9 10 11 12 13 14 15 | 16 17 18 19 20 21 22 23
   * Byte 1          | Byte 2                | Byte 3
   */
   // x & 7 == x % 8
   uint8_t mod = x & 7;
   
  if (mod == 0) { // case 1
    for (int y=15; y>=0; y--) {
      framebuffer[(x >> 3)][y] |= font[start];
      framebuffer[(x >> 3) + 1][y] |= font[start+1];
      start += 2;
    }
    
  } else if ((mod > 0) && (mod < 6)) { // case 2
    for (int y=15; y>=0; y--) {
      framebuffer[(x >> 3)][y] |= font[start] >> mod;
      framebuffer[(x >> 3) + 1][y] |= (font[start] << (8 - mod)) | (font[start+1] >> mod);
      start += 2;
    }
    /*
    framebuffer[(x >> 3)][15] |= font[start] >> mod;
    framebuffer[(x >> 3) + 1][15] |= (font[start] << (8 - mod)) | (font[start+1] >> mod); */

    // ...
  } else {
    for (int y=15; y>=0; y--) {
      framebuffer[(x >> 3)][y] |= font[start] >> mod;
      framebuffer[(x >> 3) + 1][y] |= (font[start] << (8 - mod)) | (font[start+1] >> mod);
      framebuffer[(x >> 3) + 2][y] |= (font[start+1] << (8 - mod));
      start += 2;
    }
    /*
    framebuffer[(x >> 3)][15] |= font[start] >> mod;
    framebuffer[(x >> 3) + 1][15] |= (font[start] << (8 - mod)) | (font[start+1] >> mod);
    framebuffer[(x >> 3) + 2][15] |= (font[start+1] << (8 - mod)); */
  }
}

void renderString (char s[], uint8_t x) {
  while(*s) {
    renderChar(*s++, x);
    x += 11;
    if (x > 192) {
      return;
    }
  }
}
void dumpFrameToSerial() {
  for (int y=15; y>=0; y--) {
    for (int x=0; x<24; x++) {
      for (int z=7; z>=0; z--) {
        if (framebuffer[x][y] & (1 << z)) {
          Serial.print('X');
        } else {
          Serial.print('-');
        }
      }
    }
    Serial.print('\n');
  }
}
