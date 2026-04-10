#include "hamming_codec.h"
#include <cstdint> 
uint16_t HammingCodec::EncodeByte(uint8_t byte) {
    uint16_t code = 0;

    int d1 = (byte >> 0) & 1;
    int d2 = (byte >> 1) & 1;
    int d3 = (byte >> 2) & 1;
    int d4 = (byte >> 3) & 1;
    int d5 = (byte >> 4) & 1;
    int d6 = (byte >> 5) & 1;
    int d7 = (byte >> 6) & 1;
    int d8 = (byte >> 7) & 1;

    code |= (d1 << (3 - 1));
    code |= (d2 << (5 - 1));
    code |= (d3 << (6 - 1));
    code |= (d4 << (7 - 1));
    code |= (d5 << (9 - 1));
    code |= (d6 << (10 - 1));
    code |= (d7 << (11 - 1));
    code |= (d8 << (12 - 1));

    int p1 = d1 ^ d2 ^ d4 ^ d5 ^ d7;
    int p2 = d1 ^ d3 ^ d4 ^ d6 ^ d7;
    int p4 = d2 ^ d3 ^ d4 ^ d8;
    int p8 = d5 ^ d6 ^ d7 ^ d8;

    code |= (p1 << (1 - 1));
    code |= (p2 << (2 - 1));
    code |= (p4 << (4 - 1));
    code |= (p8 << (8 - 1));

    return code;
}

uint8_t HammingCodec::DecodeCodeWord(uint16_t code,
                                     bool& corrected,
                                     bool& uncorrected) {
    corrected = false;
    uncorrected = false;

    code &= 0x0FFF;

    int b1 = (code >> 0) & 1;
    int b2 = (code >> 1) & 1;
    int b3 = (code >> 2) & 1;
    int b4 = (code >> 3) & 1;
    int b5 = (code >> 4) & 1;
    int b6 = (code >> 5) & 1;
    int b7 = (code >> 6) & 1;
    int b8 = (code >> 7) & 1;
    int b9 = (code >> 8) & 1;
    int b10 = (code >> 9) & 1;
    int b11 = (code >> 10) & 1;
    int b12 = (code >> 11) & 1;

    int s1 = b1 ^ b3 ^ b5 ^ b7 ^ b9 ^ b11;
    int s2 = b2 ^ b3 ^ b6 ^ b7 ^ b10 ^ b11;
    int s4 = b4 ^ b5 ^ b6 ^ b7 ^ b12;
    int s8 = b8 ^ b9 ^ b10 ^ b11 ^ b12;

    int syndrome = s1 | (s2 << 1) | (s4 << 2) | (s8 << 3);

    if (syndrome != 0) {
        if (syndrome >= 1 && syndrome <= 12) {
            code ^= (1u << (syndrome - 1));
            corrected = true;
        } else {
            uncorrected = true;
        }
    }

    int d1 = (code >> (3 - 1)) & 1;
    int d2 = (code >> (5 - 1)) & 1;
    int d3 = (code >> (6 - 1)) & 1;
    int d4 = (code >> (7 - 1)) & 1;
    int d5 = (code >> (9 - 1)) & 1;
    int d6 = (code >> (10 - 1)) & 1;
    int d7 = (code >> (11 - 1)) & 1;
    int d8 = (code >> (12 - 1)) & 1;

    uint8_t byte = 0;
    byte |= (d1 << 0);
    byte |= (d2 << 1);
    byte |= (d3 << 2);
    byte |= (d4 << 3);
    byte |= (d5 << 4);
    byte |= (d6 << 5);
    byte |= (d7 << 6);
    byte |= (d8 << 7);

    return byte;
}