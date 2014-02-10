#include <Entropy.h>
#include <OneWire.h>
#include "ds1961.h"
#include "hexutil.c"

/*

  5V -----\/\/\-----+
          R=4k7     |
  10 ---------------+------------o   o-----1wire----+
                                                    |
                                                    |
  12 -----\/\/\------------------o   o------>|------+
          R=220                             red     |
                                                    |
  13 -----\/\/\------------------o   o------>|------+
          R=220                            green    |
                                                    |
 GND ----------------------------o   o--------------+

*/


#define PIN_LED_GREEN 13
#define PIN_LED_RED   12
#define PIN_1WIRE     10

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
  Serial.begin(9600);
  Serial.println("RESET");
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED,   OUTPUT);
  Entropy.Initialize();
}

void loop () {
  
  byte secret[8];
  int i;
  
  for (i = 0; i < 8; i++) {
    secret[i] = Entropy.random(256);
  }
  
  while (1) { /* yo dawg */
  
    ds.reset_search();
    if (ds.search(addr)) {
      if (OneWire::crc8(addr, 7) != addr[7]) return;
      delay(100);
      hexdump(addr, 8);
      Serial.print(":");
  
      if (!sha.WriteSecret(addr, secret)) {
        Serial.println("ERROR");
        led(RED);
      } else {
        hexdump(secret, 8);
        Serial.println("");
        led(GREEN);
      }
      delay(5000);
      
      break;
    }
    
    led( millis() % 150 > 120 ? OFF : RED
    );
  }

}

void hexdump(byte* string, int size) {
  for (int i = 0; i < size; i++) {
    Serial.print(string[i] >> 4, HEX);
    Serial.print(string[i] & 0xF, HEX);
  }
}
