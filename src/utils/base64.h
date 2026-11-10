/**
 * @file base64.h
 *
 * @brief Base64 decoder for binary payloads carried inside JSON
*/

#ifndef UTILS_BASE64_H_
#define UTILS_BASE64_H_

#include <cstdint>
#include <span>
#include <string_view>

namespace bomzhur {
    /**
     * @brief Decodes one base64 character
     *
     * @param [in] symbol Character to decode
     *
     * @returns 6-bit value, or -1 if the character is not part of the alphabet
    */
    inline int base64_symbol(char symbol) {
        if ('A' <= symbol && symbol <= 'Z') return symbol - 'A';
        if ('a' <= symbol && symbol <= 'z') return symbol - 'a' + 26;
        if ('0' <= symbol && symbol <= '9') return symbol - '0' + 52;
        if (symbol == '+') return 62;
        if (symbol == '/') return 63;

        return -1;
    }

    /**
     * @brief Decodes a base64 string
     *
     * @param [in] source Base64 text, with or without '=' padding
     * @param [out] destination Output buffer
     *
     * @returns Number of decoded bytes, or -1 on malformed input or overflow
    */
    inline int base64_decode(std::string_view source, std::span<uint8_t> destination) {
        uint32_t accumulator = 0;
        unsigned bits = 0;
        unsigned written = 0;

        for (char symbol : source) {
            if (symbol == '=')
                break;

            int value = base64_symbol(symbol);
            if (value < 0)
                return -1;

            accumulator = (accumulator << 6) | static_cast<uint32_t>(value);
            bits += 6;

            if (bits >= 8) {
                bits -= 8;
                if (written >= destination.size())
                    return -1;
                destination[written++] = static_cast<uint8_t>((accumulator >> bits) & 0xFF);
            }
        }

        return static_cast<int>(written);
    }
}

#endif //! UTILS_BASE64_H_
