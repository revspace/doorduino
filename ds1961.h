#ifndef _DS1961_H_
#define _DS1961_H_

#include <stdbool.h>
#include <stdint.h>

#include "OneWire.h"

class DS1961 {

public:
  DS1961(OneWire *oneWire);

  bool WriteSecret(const uint8_t id[8], const uint8_t secret[8]);
  bool ReadAuthWithChallenge(const uint8_t id[8], uint16_t addr, const uint8_t challenge[3], uint8_t data[32], uint8_t mac[20]);
  bool WriteData(const uint8_t id[8], int addr, const uint8_t data[8], const uint8_t mac[20]);

private:
  OneWire *ow;

};

#endif /* _DS1961_H_ */
