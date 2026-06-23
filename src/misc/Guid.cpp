#include "misc/Guid.h"

#include <array>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>

namespace {
bool isHexDigit(char value) {
    return std::isxdigit(static_cast<unsigned char>(value)) != 0;
}

void appendByte(std::ostringstream& stream, std::uint8_t value) {
    stream << std::setw(2) << static_cast<int>(value);
}
}

std::string Guid::generate() {
    static std::random_device randomDevice;
    static std::mt19937_64 generator(randomDevice());
    static std::uniform_int_distribution<int> distribution(0, 255);

    std::array<std::uint8_t, 16> bytes{};
    for (std::uint8_t& byte : bytes) {
        byte = static_cast<std::uint8_t>(distribution(generator));
    }

    bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0F) | 0x40);
    bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3F) | 0x80);

    std::ostringstream stream;
    stream << std::hex << std::setfill('0');

    appendByte(stream, bytes[0]);
    appendByte(stream, bytes[1]);
    appendByte(stream, bytes[2]);
    appendByte(stream, bytes[3]);
    stream << "-";
    appendByte(stream, bytes[4]);
    appendByte(stream, bytes[5]);
    stream << "-";
    appendByte(stream, bytes[6]);
    appendByte(stream, bytes[7]);
    stream << "-";
    appendByte(stream, bytes[8]);
    appendByte(stream, bytes[9]);
    stream << "-";
    appendByte(stream, bytes[10]);
    appendByte(stream, bytes[11]);
    appendByte(stream, bytes[12]);
    appendByte(stream, bytes[13]);
    appendByte(stream, bytes[14]);
    appendByte(stream, bytes[15]);

    return stream.str();
}

bool Guid::isValid(const std::string& value) {
    if (value.size() != 36) {
        return false;
    }

    for (std::size_t index = 0; index < value.size(); ++index) {
        const bool shouldBeDash =
            index == 8 || index == 13 || index == 18 || index == 23;

        if (shouldBeDash) {
            if (value[index] != '-') {
                return false;
            }
        } else if (!isHexDigit(value[index])) {
            return false;
        }
    }

    return true;
}
