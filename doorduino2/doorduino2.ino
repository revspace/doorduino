#include <OneWire.h>
#include "ds1961.h"
#include "hexutil.c"
/*

  5V -----\/\/\-----+
          R=2k      |
  10 ---------------+------------o   o-----1wire----+
                                                    |
                            +----o   o-----door     |
                            |              opener   |
                         |--+                |      |
  11 -----\/\/\---+------| NPN mosfet        |      |
          R=150   |      |--+ (e.g. irf540)  |      |
                  /         |              +12V     |
                  \         |                       |
                  / R=10k   |                       |
                  \         |                       |
                  |         |                       |
 GND -------------+---------+----o   o--------------+
                                                    |
                                                    |
  12 -----\/\/\------------------o   o------>|------+
          R=220                             red     |
                                                    |
  13 -----\/\/\------------------o   o------>|------+
          R=220                            green    |
                                                    |
  Optional:                                  |      |
                                           -----    |
   9 ----------------------------o   o-----o   o----+
                    |
  5V -----\/\/\-----+
          R=1k

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

const int delay_access   =  6000;
const int delay_noaccess = 10000;

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

static bool connected = false;

void error () {
  connected = false;
  for (int i = 0; i < 5; i++) {
    led(OFF);
    delay(100);
    led(YELLOW);
    delay(50);
  }
}

void loop () {
  char challenge[3];

  static unsigned long keepalive = 0;

  if (! (connected && ds.reset())) {  // read ds.reset() as ds.still_connected()
    connected = false;
    ds.reset_search();
    if (ds.search(addr)) {
      if (OneWire::crc8(addr, 7) != addr[7]) return;
      connected = true;
      Serial.print("<");
      for (byte i = 0; i < 8; i++) {
        if (addr[i] < 16) Serial.print("0");
        Serial.print(addr[i], HEX);
      }
      Serial.println(">");
    }
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

  while (Serial.available()) {
    char c = Serial.read();

    if (c == 'A') {
      // XXX Wat als een challenge ooit "A" bevat?
      Serial.println("ACCESS");
      led(GREEN);
      digitalWrite(PIN_UNLOCK, HIGH);
      delay(delay_access);
      digitalWrite(PIN_UNLOCK, LOW);
      led(YELLOW);
      keepalive = millis();
    }
    else if (c == 'N') {
      Serial.println("NO ACCESS");
      led(RED);
      delay(delay_noaccess);
      led(YELLOW);
      keepalive = millis();
    } else if (c == 'C') {
      led(OFF);

      if (Serial.readBytes(challenge, 3) != 3) return;

      if (! ibutton_challenge(addr, (byte*) challenge)) {
        Serial.println("CHALLENGE ERROR");
        error();
        return;
      }
    } else if (c == 'X') {
      led(OFF);

      char newdata[8];
      char mac[20];
      unsigned char offset[1];
      if (Serial.readBytes(challenge, 3) != 3) return;
      if (Serial.readBytes(offset, 1) != 1) return;
      if (Serial.readBytes(newdata, 8) != 8) return;
      if (Serial.readBytes(mac, 20) != 20) return;

      if (! sha.WriteData(addr, offset[0], (uint8_t*) newdata, (uint8_t*) mac)) {
        Serial.println("EEPROM WRITE ERROR");
       error();
       return;
      }
      if (! ibutton_challenge(addr, (byte*) challenge)) {
        Serial.println("EXTENDED CHALLENGE ERROR");
        error();
        return;
      }
    } else if (c == 'K') {
      keepalive = millis();
      Serial.println("<K>");
    }
  }

  //while (Serial.available()) Serial.read();
}

bool ibutton_challenge(byte* id, byte* challenge) {
  uint8_t data[32];
  uint8_t mac[20];

  if (! sha.ReadAuthWithChallenge(id, 0, challenge, data, mac)) {
    return false;
  }
  Serial.print("<");
  hexdump(data, 32);
  Serial.print(" ");
  hexdump(mac, 20);
  Serial.println(">");  
  return true;
}

void hexdump(byte* string, int size) {
  for (int i = 0; i < size; i++) {
    Serial.print(string[i] >> 4, HEX);
    Serial.print(string[i] & 0xF, HEX);
  }
}
