#include <iostream>
#include "number.h"
#include <cstring>
#include <cstdio>
#include <cstdint>

int2025_t ConvertToPositive(const int2025_t& value) {
  int2025_t result;

  for (int i = 0; i < kArraySize; ++i) {
    result.parts[i] = ~value.parts[i];
  }

  uint16_t carry = 1;
  for (int i = kLastByteIdx; i >= 0; --i) {
    uint16_t sum = result.parts[i] + carry;
    result.parts[i] = sum & kLowByteMask;
    carry = sum >> kByteShift8;
  }

  return result;
}

int2025_t GetAbsoluteValue(const int2025_t& value) {
  return (value.parts[0] & kSignBit) ? ConvertToPositive(value) : value;
}

int2025_t from_int(int32_t i) {
  int2025_t result;

  if (i < 0) {
    std::memset(result.parts, kLowByteMask, sizeof(result.parts));
  } else {
    std::memset(result.parts, 0, sizeof(result.parts));
  }

  result.parts[kLastByteIdx] = i & kLowByteMask;
  result.parts[kLastByteIdx - 1] = (i >> kByteShift8) & kLowByteMask;
  result.parts[kLastByteIdx - 2] = (i >> kByteShift16) & kLowByteMask;
  result.parts[kLastByteIdx - 3] = (i >> kByteShift24) & kLowByteMask;

  return result;
}

void MultiplyBy10(int2025_t& num) {
  uint16_t carry = 0;

  for (int i = kLastByteIdx; i >= 0; --i) {
    uint16_t product = num.parts[i] * kDecimalBase + carry;
    num.parts[i] = product & kLowByteMask;
    carry = product >> kByteShift8;
  }
}

void AddDigit(int2025_t& num, uint8_t digit) {
  uint16_t carry = digit;

  for (int i = kLastByteIdx; i >= 0 && carry > 0; --i) {
    uint16_t sum = num.parts[i] + carry;
    num.parts[i] = sum & kLowByteMask;
    carry = sum >> kByteShift8;
  }
}

int2025_t from_string(const char* buff) {
  int2025_t result;
  std::memset(result.parts, 0, sizeof(result.parts));

  bool is_negative = false;
  int start = 0;

  if (buff[0] == '-') {
    is_negative = true;
    start = 1;
  } else if (buff[0] == '+') {
    start = 1;
  }

  for (int i = start; buff[i] != '\0'; ++i) {
    MultiplyBy10(result);
    AddDigit(result, buff[i] - '0');
  }

  if (is_negative) {
    for (int i = 0; i < kArraySize; ++i) {
      result.parts[i] = ~result.parts[i];
    }

    uint16_t carry = 1;
    for (int i = kLastByteIdx; i >= 0 && carry; --i) {
      uint16_t sum = result.parts[i] + carry;
      result.parts[i] = sum & kLowByteMask;
      carry = sum >> kByteShift8;
    }
  }

  return result;
}

int2025_t operator+(const int2025_t& lhs, const int2025_t& rhs) {
  int2025_t result;
  uint16_t carry = 0;

  for (int i = kLastByteIdx; i >= 0; --i) {
    uint16_t sum = lhs.parts[i] + rhs.parts[i] + carry;
    result.parts[i] = sum & kLowByteMask;
    carry = sum >> kByteShift8;
  }

  return result;
}

int2025_t operator-(const int2025_t& lhs, const int2025_t& rhs) {
  int2025_t result = rhs;

  for (int i = 0; i < kArraySize; ++i) {
    result.parts[i] = ~result.parts[i];
  }

  uint16_t carry = 1;
  for (int i = kLastByteIdx; i >= 0; --i) {
    uint16_t sum = result.parts[i] + carry;
    result.parts[i] = sum & kLowByteMask;
    carry = sum >> kByteShift8;
  }

  return lhs + result;
}

int2025_t operator*(const int2025_t& lhs, const int2025_t& rhs) {
  int2025_t result;
  std::memset(result.parts, 0, sizeof(result.parts));

  bool lhs_neg = (lhs.parts[0] & kSignBit) != 0;
  bool rhs_neg = (rhs.parts[0] & kSignBit) != 0;

  int2025_t abs_lhs = GetAbsoluteValue(lhs);
  int2025_t abs_rhs = GetAbsoluteValue(rhs);

  for (int i = kLastByteIdx; i >= 0; --i) {
    uint16_t carry = 0;

    for (int j = kLastByteIdx; j >= 0; --j) {
      int pos = i + j - kLastByteIdx;

      if (pos < 0) continue;
      if (pos > kLastByteIdx) break;

      uint32_t product = (uint32_t)abs_lhs.parts[i] * (uint32_t)abs_rhs.parts[j];
      uint32_t sum = product + (uint32_t)result.parts[pos] + carry;

      result.parts[pos] = sum & kLowByteMask;
      carry = (sum >> kByteShift8) & 0xFFFF;
    }

    if (carry > 0) {
      int pos = i - kArraySize;

      while (pos >= 0 && carry > 0) {
        uint32_t sum = (uint32_t)result.parts[pos] + carry;
        result.parts[pos] = sum & kLowByteMask;
        carry = sum >> kByteShift8;
        pos--;
      }
    }
  }

  if (lhs_neg != rhs_neg) {
    result = ConvertToPositive(result);
  }

  return result;
}

int2025_t operator/(const int2025_t& lhs, const int2025_t& rhs) {
  bool is_zero = true;

  for (int i = 0; i < kArraySize; ++i) {
    if (rhs.parts[i] != 0) {
      is_zero = false;
      break;
    }
  }

  if (is_zero) {
    int2025_t zero;
    std::memset(zero.parts, 0, sizeof(zero.parts));
    return zero;
  }

  bool lhs_neg = (lhs.parts[0] & kSignBit) != 0;
  bool rhs_neg = (rhs.parts[0] & kSignBit) != 0;

  int2025_t dividend = GetAbsoluteValue(lhs);
  int2025_t divisor = GetAbsoluteValue(rhs);
  int2025_t quotient;
  std::memset(quotient.parts, 0, sizeof(quotient.parts));
  int2025_t remainder;
  std::memset(remainder.parts, 0, sizeof(remainder.parts));

  for (int byte_idx = 0; byte_idx < kArraySize; ++byte_idx) {
    for (int bit_idx = kMaxBitIdx; bit_idx >= 0; --bit_idx) {
      uint8_t carry_bit = 0;

      for (int i = kLastByteIdx; i >= 0; --i) {
        uint8_t new_carry = (remainder.parts[i] >> kMaxBitIdx) & 0x01;
        remainder.parts[i] = (remainder.parts[i] << 1) | carry_bit;
        carry_bit = new_carry;
      }

      if (dividend.parts[byte_idx] & (1 << bit_idx)) {
        remainder.parts[kLastByteIdx] |= 0x01;
      }

      int cmp = 0;

      for (int i = 0; i < kArraySize; ++i) {
        if (remainder.parts[i] > divisor.parts[i]) {
          cmp = 1;
          break;
        }
        if (remainder.parts[i] < divisor.parts[i]) {
          cmp = -1;
          break;
        }
      }

      if (cmp >= 0) {
        uint16_t borrow = 0;

        for (int i = kLastByteIdx; i >= 0; --i) {
          int16_t diff = (int16_t)remainder.parts[i] - (int16_t)divisor.parts[i] - borrow;

          if (diff < 0) {
            remainder.parts[i] = diff + kByteValue;
            borrow = 1;
          } else {
            remainder.parts[i] = diff;
            borrow = 0;
          }
        }

        quotient.parts[byte_idx] |= (1 << bit_idx);
      }
    }
  }

  if (lhs_neg != rhs_neg) {
    quotient = ConvertToPositive(quotient);
  }

  return quotient;
}

bool operator==(const int2025_t& lhs, const int2025_t& rhs) {
  return std::memcmp(lhs.parts, rhs.parts, sizeof(lhs.parts)) == 0;
}

bool operator!=(const int2025_t& lhs, const int2025_t& rhs) {
  return std::memcmp(lhs.parts, rhs.parts, sizeof(lhs.parts)) != 0;
}

std::ostream& operator<<(std::ostream& stream, const int2025_t& value) {
  bool is_negative = (value.parts[0] & kSignBit) != 0;

  if (is_negative) {
    stream << '-';
  }

  stream << "0x";

  const char kHexDigits[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                               '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

  bool non_zero_found = false;
  uint8_t temp[kArraySize];

  for (int i = 0; i < kArraySize; ++i) {
    temp[i] = value.parts[i];
  }

  if (is_negative) {
    for (int i = 0; i < kArraySize; ++i) {
      temp[i] = ~temp[i];
    }

    int carry = 1;

    for (int i = kLastByteIdx; i >= 0 && carry; --i) {
      int sum = temp[i] + carry;
      temp[i] = sum & kLowByteMask;
      carry = sum >> kByteShift8;
    }
  }

  for (int i = 0; i < kArraySize; ++i) {
    uint8_t byte = temp[i];
    uint8_t first_half = (byte >> kNibbleShift);
    uint8_t second_half = byte & kLowNibbleMask;

    if (first_half != 0 || second_half != 0) {
      if (!non_zero_found && first_half == 0) {
        stream << kHexDigits[second_half];
      } else {
        stream << kHexDigits[first_half] << kHexDigits[second_half];
      }
      non_zero_found = true;
    } else if (non_zero_found) {
      stream << kHexDigits[first_half] << kHexDigits[second_half];
    }
  }

  if (!non_zero_found) {
    stream << '0';
  }

  return stream;
}