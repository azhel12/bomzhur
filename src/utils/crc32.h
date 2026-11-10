/**
 * @file crc32.h
 *
 * @brief Software CRC32 (CH32V003 has no CRC peripheral)
*/

#ifndef UTILS_CRC32_H_
#define UTILS_CRC32_H_

#include <cstdint>
#include <span>

namespace bomzhur {
    /**
     * @brief Reflected CRC-32 (poly 0xEDB88320, init 0xFFFFFFFF, xorout 0xFFFFFFFF)
    */
    class crc32 {
    public:
        /// @brief Polynom in reflected form, as the bit loop consumes it
        static constexpr uint32_t reflected_polynom = 0xEDB88320;

        /**
         * @brief Returns polynom value
         *
         * @returns Polynom value (non-reflected form)
        */
        static constexpr uint32_t polynom() {
            return 0x04C11DB7;
        }

        /**
         * @brief Returns intial value
         *
         * @returns CRC32 initial value
        */
        static constexpr uint32_t initial_value() {
            return 0xFFFFFFFF;
        }

        /**
         * @brief Calculates CRC32 checksum for given data
         *
         * @param [in] data Data to checksum
         *
         * @returns CRC32
        */
        static uint32_t calculate(std::span<const uint8_t> data) {
            uint32_t crc = initial_value();

            for (uint8_t byte : data) {
                crc ^= byte;

                for (unsigned bit = 0; bit < 8; ++bit) {
                    // Branchless: the mask is all ones when the low bit is set
                    crc = (crc >> 1) ^ (reflected_polynom & -(crc & 1));
                }
            }

            return ~crc;
        }
    };
}

#endif //! UTILS_CRC32_H_
