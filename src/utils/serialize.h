/**
 * @file serialize.h
 * 
 * @brief Contains simple binary serialize functions
*/

#ifndef UTILS_SERIALIZE_H_
#define UTILS_SERIALIZE_H_

namespace bomzhur {
    /**
     * @brief Serialize value to binary
     * 
     * @param [out] destination Output data pointer
     * @param [in] value Value to serialize
     * 
     * @returns Number of bytes written
    */
    inline unsigned binary_serialize(uint8_t* destination, auto value) {
        if constexpr (sizeof(value) == 1) {
            destination[0] = value;
        } else if constexpr (sizeof(value) == 2) {
            destination[0] = static_cast<uint8_t>(static_cast<uint16_t>(value) & 0x00ffu);
            destination[1] = static_cast<uint8_t>((static_cast<uint16_t>(value) >> std::numeric_limits<uint8_t>::digits) & 0x00ffu);
        } else if constexpr (sizeof(value) == 4) {
            binary_serialize(destination, static_cast<uint16_t>(static_cast<uint32_t>(value) & 0xffffu));
            binary_serialize(destination + 2, static_cast<uint16_t>((static_cast<uint32_t>(value) >> std::numeric_limits<uint16_t>::digits) & 0xffffu));
        }

        return sizeof(value);
    }

    /**
     * @brief Deserialize value from binary
     * 
     * @tparam _type Target type
     * 
     * @param [in] source Source data pointer
     * 
     * @returns Deserialized value
    */
    template<typename _type>
    inline _type binary_deserialize(const uint8_t* source) {
        if constexpr (sizeof(_type) == 1) {
            return source[0];
        } else if constexpr (sizeof(_type) == 2) {
            return static_cast<_type>(static_cast<uint16_t>(source[0]) | (static_cast<uint16_t>(source[1]) << std::numeric_limits<uint8_t>::digits));
        } else if constexpr (sizeof(_type) == 4) {
            return static_cast<_type>(static_cast<uint32_t>(binary_deserialize<uint16_t>(source)) | (static_cast<uint32_t>(binary_deserialize<uint16_t>(source + 2)) << std::numeric_limits<uint16_t>::digits));
        }

        return 0;
    }
}

#endif //! UTILS_SERIALIZE_H_