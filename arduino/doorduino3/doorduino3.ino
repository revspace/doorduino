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

byte id[8];

void led (byte color) {
  digitalWrite(PIN_LED_GREEN, color & GREEN);
  digitalWrite(PIN_LED_RED,   color & RED);
}

void setup () {
  Serial.begin(115200);
  Serial.println("RESET");
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED,   OUTPUT);
  pinMode(PIN_UNLOCK,    OUTPUT);
  pinMode(PIN_BUTTON,    INPUT);
  digitalWrite(PIN_BUTTON, HIGH);

  led(YELLOW);
}

static bool connected = false;
static unsigned long error_flash;

void error () {
  connected = false;
  error_flash = millis() + 200;
}

void loop () {
  char challenge[3];
  
  static unsigned long keepalive = 0;

  if (! (connected && ds.reset())) {  // read ds.reset() as ds.still_connected()
    connected = false;
    ds.reset_search();
    if (ds.search(id)) {
      if (OneWire::crc8(id, 7) != id[7]) return;
      connected = true;
      led(OFF);
      Serial.print("<");
      for (byte i = 0; i < 8; i++) {
        if (id[i] < 16) Serial.print("0");
        Serial.print(id[i], HEX);
      }
      Serial.println(">");
    }
  }
  
  int button = 0;
  for (; button < 10 && !digitalRead(PIN_BUTTON); button++) delay(1);
  if (button >= 10) {
    Serial.println("<BUTTON>");
  }

  if (!connected && error_flash && error_flash < millis()) {
    error_flash = 0;
    for (int i = 0; i < 5; i++) {
      led(OFF);
      delay(50);
      led(YELLOW);
      delay(25);
    }
  }

  if (connected) {
    led(OFF);
  } else {
    unsigned int m = millis() % 3000;
    bool have_comm = (keepalive && millis() - 5000 < keepalive);
    led( 
      ((m > 2600 && m <= 2700) || (m > 2900))
      ? (have_comm ? OFF : RED)
      : (have_comm ? YELLOW : OFF)
    );
  }

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
      error_flash = 0;
    }
    else if (c == 'N') {
      Serial.println("NO ACCESS");
      led(RED);
      delay(delay_noaccess);
      led(YELLOW);
      keepalive = millis();
      error_flash = 0;
    } else if (c == 'C') {
      led(OFF);

      unsigned char page[1];

      if (Serial.readBytes(page, 1) != 1) return;
      if (Serial.readBytes(challenge, 3) != 3) return;

      if (! ibutton_challenge(page[0], (byte*) challenge)) {
        Serial.println("CHALLENGE ERROR");
        if (!ds.reset()) error();
        return;
      }
    } else if (c == 'X') {
      led(OFF);

      unsigned char page[1];
      char newdata[8];
      char mac[20];
      unsigned char offset[1];
      if (Serial.readBytes(page, 1) != 1) return;
      if (Serial.readBytes(challenge, 3) != 3) return;
      if (Serial.readBytes(offset, 1) != 1) return;
      if (Serial.readBytes(newdata, 8) != 8) return;
      if (Serial.readBytes(mac, 20) != 20) return;

      if (! sha.WriteData(NULL, page[0] * 32 + offset[0], (uint8_t*) newdata, (uint8_t*) mac)) {
        Serial.println("EEPROM WRITE ERROR");
        error();
        return;
      }
      if (! ibutton_challenge(page[0], (byte*) challenge)) {
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

bool ibutton_challenge(byte page, byte* challenge) {
  uint8_t data[32];
  uint8_t mac[20];
  
  if (! sha.ReadAuthWithChallenge(NULL, page * 32, challenge, data, mac)) {
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
