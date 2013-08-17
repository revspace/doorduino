#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "OneWire.h"
#include "ds1961.h"

// commands used in the DS1961
#define CMD_WRITE_SCRATCHPAD     0x0F
#define CMD_COMPUTE_NEXT_SECRET  0x33
#define CMD_COPY_SCRATCHPAD      0x55
#define CMD_LOAD_FIRST_SECRET    0x5A
#define CMD_REFRESH_SCRATCHPAD   0xA3
#define CMD_READ_AUTH_PAGE       0xA5
#define CMD_READ_SCRATCHPAD      0xAA
#define CMD_READ_MEMORY          0xF0

// memory ranges
#define MEM_DATA_PAGE_0          0x00
#define MEM_DATA_PAGE_1          0x20
#define MEM_DATA_PAGE_2          0x40
#define MEM_DATA_PAGE_3          0x60
#define MEM_SECRET               0x80
#define MEM_IDENTITY             0x90

// timing (ms)
#define T_CSHA                   2     // actually 1.5
#define T_PROG                   10


DS1961::DS1961(OneWire *oneWire)
{
  ow = oneWire;
}

static bool ResetAndSelect(OneWire *ow, const uint8_t id[8])
{
  if (!ow->reset()) {
    return false;
  }
  ow->select((uint8_t *) id);
  
  return true;
}

static bool WriteScratchPad(OneWire *ow, const uint8_t id[8], uint16_t addr, const uint8_t data[8])
{
  uint8_t buf[11];
  uint8_t crc[2];
  int len = 0;

  // reset and select
  if (!ResetAndSelect(ow, id)) {
    return false;
  }

  // perform write scratchpad command
  buf[len++] = CMD_WRITE_SCRATCHPAD;
  buf[len++] = (addr >> 0) & 0xFF;    // 2 byte target address
  buf[len++] = (addr >> 8) & 0xFF;    // 2 byte target address
  memcpy(buf + len, data, 8);
  len += 8;
  ow->write_bytes(buf, len);
  ow->read_bytes(crc, 2);

  return ow->check_crc16(buf, len, crc);
}

static bool RefreshScratchPad(OneWire *ow, const uint8_t id[8], uint16_t addr, const uint8_t data[8])
{
  uint8_t buf[11];
  uint8_t crc[2];
  int len = 0;

  // reset and select
  if (!ResetAndSelect(ow, id)) {
    return false;
  }

  // perform refresh scratchpad command
  buf[len++] = CMD_REFRESH_SCRATCHPAD;
  buf[len++] = (addr >> 0) & 0xFF;    // 2 byte target address
  buf[len++] = (addr >> 8) & 0xFF;    // 2 byte target address
  memcpy(buf + len, data, 8);
  len += 8;
  ow->write_bytes(buf, len);
  ow->read_bytes(crc, 2);

  return ow->check_crc16(buf, len, crc);
}

static bool ReadScratchPad(OneWire *ow, const uint8_t id[8], uint16_t *addr, uint8_t *es, uint8_t data[8])
{
  uint8_t buf[12];
  uint8_t crc[2];
  int len = 0;

  // reset and select
  if (!ResetAndSelect(ow, id)) {
    return false;
  }

  // send read scratchpad command
  buf[len++] = CMD_READ_SCRATCHPAD;
  ow->write_bytes(buf, len);

  // get TA0/1 and ES
  ow->read_bytes(buf + len, 3);
  len += 3;
  *addr = (buf[2] << 8) | buf[1];
  *es = buf[3];

  // get data
  ow->read_bytes(buf + len, 8);
  len += 8;
  memcpy(data, buf + 4, 8);

  // check CRC
  ow->read_bytes(crc, 2);
  return ow->check_crc16(buf, len, crc);
}

static bool CopyScratchPad(OneWire *ow, const uint8_t id[8], uint16_t addr, uint8_t es, const uint8_t mac[20])
{
  uint8_t buf[4];
  int len = 0;
  uint8_t status;

  // reset and select
  if (!ResetAndSelect(ow, id)) {
    return false;
  }

  // send copy scratchpad command + arguments
  buf[len++] = CMD_COPY_SCRATCHPAD;
  buf[len++] = (addr >> 0) & 0xFF;    // 2 byte target address
  buf[len++] = (addr >> 8) & 0xFF;    // 2 byte target address
  buf[len++] = es;                    // es
  ow->write_bytes(buf, len, 1);       // write and keep powered

  // wait while MAC is calculated
  delay(T_CSHA);

  // send MAC
  ow->write_bytes(mac, 20);
  
  // wait 10 ms
  delay(T_PROG);
  ow->depower();
  
  // check final status byte
  status = ow->read();
  return (status == 0xAA);
}

static bool ReadAuthPage(OneWire *ow, const uint8_t id[8], uint16_t addr, uint8_t data[32], uint8_t mac[20])
{
  uint8_t buf[36];
  uint8_t crc[2];
  uint8_t status;
  int len = 0;

  // reset and select
  if (!ResetAndSelect(ow, id)) {
    return false;
  }

  // send command
  buf[len++] = CMD_READ_AUTH_PAGE;
  buf[len++] = (addr >> 0) & 0xFF;
  buf[len++] = (addr >> 8) & 0xFF;
  ow->write_bytes(buf, len);

  // read data part + 0xFF
  ow->read_bytes(buf + len, 33);
  len += 33;
  if (buf[35] != 0xFF) {
    return false;
  }
  ow->read_bytes(crc, 2);
  if (!ow->check_crc16(buf, len, crc)) {
    return false;
  }
  memcpy(data, buf + 3, 32);

  // read mac part
  delay(T_CSHA);
  ow->read_bytes(mac, 20);
  ow->read_bytes(crc, 2);
  if (!ow->check_crc16(mac, 20, crc)) {
    return false;
  }

  // check final status byte
  status = ow->read();
  return (status == 0xAA);
}

static bool LoadFirstSecret(OneWire *ow, const uint8_t id[8], uint16_t addr, uint8_t es)
{
  uint8_t status;
  
  // reset and select
  if (!ResetAndSelect(ow, id)) {
    return false;
  }

  // write auth code
  ow->write(CMD_LOAD_FIRST_SECRET);
  ow->write((addr >> 0) & 0xFF);
  ow->write((addr >> 8) & 0xFF);
  ow->write(es, 1);
  delay(T_PROG);
  ow->depower();
  
  status = ow->read();
  return (status == 0xAA);
}

static bool ReadMemory(OneWire *ow, const uint8_t id[8], int addr, int len, uint8_t data[])
{
  // reset and select
  if (!ResetAndSelect(ow, id)) {
     return false;
  }
  
  // write command/addr
  ow->write(CMD_READ_MEMORY);
  ow->write((addr >> 0) & 0xFF);
  ow->write((addr >> 8) & 0xFF);
  
  // read data
  ow->read_bytes(data, len);
  
  return true;
}

bool DS1961::ReadAuthWithChallenge(const uint8_t id[8], uint16_t addr, const uint8_t challenge[3], uint8_t data[32], uint8_t mac[20])
{
  uint8_t scratchpad[8];

  // put the challenge in the scratchpad
  memset(scratchpad, 0, sizeof(scratchpad));
  memcpy(scratchpad + 4, challenge, 3);
  if (!WriteScratchPad(ow, id, addr, scratchpad)) {
//        Serial.println("WriteScratchPad failed!");
    return false;
  }

  // perform the authenticated read
  if (!ReadAuthPage(ow, id, addr, data, mac)) {
//        Serial.println("ReadAuthPage failed!");
    return false;
  }

  return true;
}

bool DS1961::WriteSecret(const uint8_t id[8], const uint8_t secret[8])
{
  uint16_t addr;
  uint8_t es;
  uint8_t data[8];
  
  // write secret to scratch pad
  if (!WriteScratchPad(ow, id, MEM_SECRET, secret)) {
//    Serial.println("WriteScratchPad failed!");
    return false;
  }
  
  // read scratch pad for auth code
  if (!ReadScratchPad(ow, id, &addr, &es, data)) {
//    Serial.println("ReadScratchPad failed!");
    return false;
  }
  if (!LoadFirstSecret(ow, id, addr, es)) {
//    Serial.println("LoadFirstSecret failed!");
    return false;
  }

  return true;
}

/*
 * Writes 8 bytes of data to specified address
 */
bool DS1961::WriteData(const uint8_t id[8], int addr, const uint8_t data[8], const uint8_t mac[20])
{
  uint8_t spad[8];
  uint16_t ad;
  uint8_t es;
  
  // write data into scratchpad
  if (!WriteScratchPad(ow, id, addr, data)) {
    Serial.println("WriteScratchPad failed!");
    return false;
  }
  
  // read scratch pad for auth code
  if (!ReadScratchPad(ow, id, &ad, &es, spad)) {
    Serial.println("ReadScratchPad failed!");
    return false;
  }
  
  // copy scratchpad to EEPROM
  if (!CopyScratchPad(ow, id, ad, es, mac)) {
    Serial.println("CopyScratchPad failed!");
    return false;
  }
  
  // refresh scratchpad
  if (!RefreshScratchPad(ow, id, addr, data)) {
    Serial.println("RefreshScratchPad failed!");
    return false;
  }
  
  // re-write with load first secret
  if (!LoadFirstSecret(ow, id, addr, es)) {
    Serial.println("LoadFirstSecret failed!");
    return false;
  }
  
  return true;
}

