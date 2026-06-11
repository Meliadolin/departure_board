#ifndef CRC32_H
#define CRC32_H

#include <Arduino.h>

class CRC32 {
  private:
    uint32_t crc;
    
  public:
    CRC32() {
      crc = 0xFFFFFFFF;
    }
    
    void update(const char* data) {
      if (data == nullptr) return;
      while (*data) {
        crc ^= *data++;
        for (int i = 0; i < 8; i++) {
          if (crc & 1) {
            crc = (crc >> 1) ^ 0xEDB88320;
          } else {
            crc >>= 1;
          }
        }
      }
    }
    
    uint32_t finalize() {
      return ~crc;
    }
};

#endif
