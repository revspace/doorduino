#include <OneWire.h>
#include "ds1961.h"
#include "hexutil.c"
/*

  5V -----\/\/\-----+
          R=4k7     |
  10 ---------------+------------o   o-----1wire----+
                                                    |
  In case of relay:        +-----o   o-----relay    |
                           |               with     |
  11 -----+-----\/\/\-----K  (npn)         diode    |
          |     R=1k       |                 |      |
          |                |               +12V     |
          |                |                        |
          +-----\/\/\------+-----GND                |
                R=100k                              |
                                                    |
  In case of door opener:  +---+-o   o-----+---+    |
                           |   |           |   |    |
  11 ------+----\/\/\-----K    |           V   3    |
           |    R=1k       |   |           -   3    |
  K=2N2222 |               +--K'           |   |    |
  K'=TIP41C|                   |     +12V--+---+    |
           |                   |                    |
           +----\/\/\----------+-----GND            |
                R=100k                              |
                                                    |
  12 -----\/\/\------------------o   o------>|------+
          R=220                             red     |
                                                    |
  13 -----\/\/\------------------o   o------>|------+
          R=220                            green    |
                                                    |
 GND ----------------------------o   o--------------+
                                                    |
  Optional:                                  |      |
                                           -----    |
   9 ----------------------------o   o-----o   o----+

*/


#define PIN_LED_GREEN 13
#define PIN_LED_RED   12
#define PIN_UNLOCK    11
#define PIN_1WIRE     10
#define PIN_BUTTON     9

#define OFF    0
#define GREEN  1
#define RED    2
#define YELLOW (GREEN | RED)

OneWire ds(PIN_1WIRE);
DS1961  sha(&ds);

byte addr[8];

void led (byte color) {
  digitalWrite(PIN_LED_GREEN, color & GREEN);
  digitalWrite(PIN_LED_RED,   color & RED);
}

void setup () {
  Serial.begin(57600);
  Serial.println("RESET");
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED,   OUTPUT);
  pinMode(PIN_UNLOCK,    OUTPUT);
  pinMode(PIN_BUTTON,    INPUT);
  digitalWrite(PIN_BUTTON, HIGH);

  led(YELLOW);
}

void loop () {
  char challenge[3];
  static unsigned long keepalive = 0;
  
  ds.reset_search();
  if (ds.search(addr)) {
    if (OneWire::crc8(addr, 7) != addr[7]) return;
    Serial.print("<");
    for (byte i = 0; i < 8; i++) {
      if (addr[i] < 16) Serial.print("0");
      Serial.print(addr[i], HEX);
    }
    Serial.println(">");
    delay(20);  // ivm bouncen van ibutton
  }
  if (digitalRead(PIN_BUTTON) == LOW) {
    Serial.println("<BUTTON>");
  }
  
  unsigned int m = millis() % 3000;
  bool have_comm = (keepalive && millis() - 5000 < keepalive);
  led( 
    ((m > 2600 && m <= 2700) || (m > 2900))
    ? (have_comm ? OFF : RED)
    : (have_comm ? YELLOW : OFF)
  );

  if (!Serial.available()) return;
  char c = Serial.read();
  
  if (c == 'A') {
    // XXX Wat als een challenge ooit "A" bevat?
    Serial.println("ACCESS");
    led(GREEN);
    digitalWrite(PIN_UNLOCK, HIGH);
    delay(6000);
    digitalWrite(PIN_UNLOCK, LOW);
    led(YELLOW);
    keepalive = millis();
  }
  else if (c == 'N') {
    Serial.println("NO ACCESS");
    led(RED);
    delay(10000);
    led(YELLOW);
    keepalive = millis();
  } else if (c == 'C') {
    led(OFF);
    if (Serial.readBytes(challenge, 3) != 3) return;
    ibutton_challenge(addr, (byte*) challenge);
  } else if (c == 'K') {
    keepalive = millis();
    Serial.println("<K>");
  }
  while (Serial.available()) Serial.read();
}

void ibutton_challenge(byte* id, byte* challenge) {
  uint8_t data[32];
  uint8_t mac[20];
  if (!sha.ReadAuthWithChallenge(id, 0, challenge, data, mac)) {
    Serial.println("CHALLENGE ERROR");
    return;
  }
  Serial.print("<");
  hexdump(data, 32);
  Serial.print(" ");
  hexdump(mac, 20);
  Serial.println(">");  
}

void hexdump(byte* string, int size) {
  for (int i = 0; i < size; i++) {
    Serial.print(string[i] >> 4, HEX);
    Serial.print(string[i] & 0xF, HEX);
  }
}
