#pragma once
#include <cinttypes>
#include <iostream>

namespace {

constexpr int kArraySize = 254;
constexpr int kLastByteIdx = 253;
constexpr int kBitsPerByte = 8;
constexpr int kMaxBitIdx = 7;
constexpr uint8_t kLowByteMask = 0xFF;
constexpr uint8_t kSignBit = 0x80;
constexpr uint8_t kLowNibbleMask = 0x0F;
constexpr int kNibbleShift = 4;
constexpr int kByteShift8 = 8;
constexpr int kByteShift16 = 16;
constexpr int kByteShift24 = 24;
constexpr int kDecimalBase = 10;
constexpr int kByteValue = 256;

} // namespace
struct int2025_t {
    uint8_t parts[254];
};

static_assert(sizeof(int2025_t) <= 254,  "Size of int2025_t must be no higher than 254 bytes");

int2025_t from_int(int32_t i);

int2025_t from_string(const char* buff);

int2025_t operator+(const int2025_t& lhs, const int2025_t& rhs);

int2025_t operator-(const int2025_t& lhs, const int2025_t& rhs);

int2025_t operator*(const int2025_t& lhs, const int2025_t& rhs);

int2025_t operator/(const int2025_t& lhs, const int2025_t& rhs);

bool operator==(const int2025_t& lhs, const int2025_t& rhs);

bool operator!=(const int2025_t& lhs, const int2025_t& rhs);

std::ostream& operator<<(std::ostream& stream, const int2025_t& value);