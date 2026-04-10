#pragma once

#include <cstdint>

class HammingCodec {
public:
    uint16_t EncodeByte(uint8_t byte);
    uint8_t DecodeCodeWord(uint16_t code, bool& corrected, bool& uncorrected);
};