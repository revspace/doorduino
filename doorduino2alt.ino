#include <OneWire.h>


/*

  5V -----\/\/\-----+
          R=4k7     |
  10 ---------------+------------o   o-----1wire----+
                                                    |
                           +-----o   o-----strike   |
                           |                 |      |
  11 -----+-----\/\/\-----K  (npn)          12V     |
          |     R=1k       |                        |
          |                |                        |
          +-----\/\/\------+------------------------+
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
  pinMode(PIN_UNLOCK,    OUTPUT);
  pinMode(PIN_BUTTON,    INPUT);
  digitalWrite(PIN_BUTTON, HIGH);

  led(YELLOW);
}

void loop () {
  ds.reset_search();
  if (ds.search(addr)) {
    if (OneWire::crc8(addr, 7) != addr[7]) return;
    Serial.print("<");
    for (byte i = 0; i < 8; i++) {
      if (addr[i] < 16) Serial.print("0");
      Serial.print(addr[i], HEX);
    }
    Serial.println(">");
    delay(200);  // debounce
  }
  if (digitalRead(PIN_BUTTON) == LOW) {
    Serial.println("<BUTTON>");
    delay(500);  // debounce
  }
  
  unsigned int m = millis() % 3000;
  led( 
    ((m > 2600 && m <= 2700) || (m > 2900))
    ? OFF : YELLOW
  );

  if (!Serial.available()) return;
  char c = Serial.read();
  
  if (c == 'A') {
    Serial.println("ACCESS");
    led(GREEN);
    digitalWrite(PIN_UNLOCK, HIGH);
    delay(6000);
    digitalWrite(PIN_UNLOCK, LOW);
    led(YELLOW);
  }
  else if (c == 'N') {
    Serial.println("NO ACCESS");
    led(RED);
    delay(10000);
    led(YELLOW);
  }
  while (Serial.available()) Serial.read();
}
