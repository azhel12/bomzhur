/**
 * @file converters.h
 * 
 * @brief Contains simple lighweight converter between string and numbers (or other types in future)
*/

#ifndef UTILS_CONVERTERS_H_
#define UTILS_CONVERTERS_H_

#include <concepts>
#include <limits>
#include <cmath>
#include <cstdint>
#include <charconv>

namespace bomzhur {
    /**
     * @brief Convert unsigned to string
     * 
     * @param [out] buffer Buffer to write
     * @param [in] value Value to convert
    */
    inline char* convert_to_chars(char* buffer, uint32_t value) {
        if (value == 0) {
            *buffer = '0';
            return buffer + 1;
        }

        unsigned length = 0;
        while(value > 0) {
            unsigned digit = value % 10;
            buffer[length++] = digit + '0';
            value /= 10;
        }

        for(unsigned i = 0; i < length / 2; ++i) {
            std::swap(buffer[i], buffer[length - 1 - i]);
        }

        return buffer + length;
    }

    /**
     * @brief Parse string and extract unsigned value
     * 
     * @param [in] begin Input string start
     * @param [in] end Input string end
     * @param [out] value Parsed value
     * 
     * @returns Pointer to symbol when error was occured or end if parse success
    */
    inline const char* convert_from_chars(const char* begin, const char* end, uint32_t& value) {
        value = 0;

        while(begin != end) {
            if ('0' <= *begin && *begin <= '9') {
                value *= 10;
                value += *begin - '0';
            } else {
                return begin;
            }
            ++begin;
        }

        return begin;
    }

    template<std::unsigned_integral T>
    inline const char* convert_from_chars(const char* begin, const char* end, T& value) {
        uint32_t temp = 0;

        auto res = convert_from_chars(begin, end, temp);
        value = static_cast<std::decay_t<decltype(value)>>(temp);

        return res;
    }
}

#endif //! UTILS_CONVERTERS_H_