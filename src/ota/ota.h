/**
 * @file ota.h
 * 
 * @brief Over-the-air firmware update into the inactive application slot.
 * Can be modified for other MCU. Interface is three methods: begin/write_chunk/end
*/

#ifndef OTA_OTA_H_
#define OTA_OTA_H_

#include "../../boot_rules.h"

#include <cstdint>
#include <span>

namespace bomzhur {
    /**
     * @brief Implements firmware update for the two-slot (A/B) scheme
     * 
     * @tparam _application_size App size in bytes
     * @tparam _first_app_begin First application begin address
     * @tparam _second_app_begin Second application begin address
    */
    template<uint32_t _application_size, uint32_t _first_app_begin, uint32_t _second_app_begin>
    class ota {
        /// @brief One half of the update file: image + CRC32 + update counter
        static constexpr uint32_t _slot_payload_size = _application_size + APP_CRC32_SIZE + UPDATE_COUNTER_SIZE;

        /// @brief Stored updater parameters
        struct params_t {
            /// @brief Received bytes count
            uint32_t received_size;
            /// @brief Current write address, nullptr when no update is in progress
            uint8_t* current_write_address;
            /// @brief Last erased page number
            uint8_t last_erased_page;
        };

    public:
        /**
         * @brief Return current application number (like the current bank on dual-bank MCUs)
         * 
         * @returns Current application (0 or 1)
        */
        static unsigned get_current_app_number();

        /**
         * @brief Begin update process (prepare for data write)
         * 
         * @param [in] image_size Image size in bytes
         * @param [in] crc32 Expected application CRC32 value (unused: half of the image is skipped, each slot carries its own CRC32 checked by end())
         * 
         * @retval true Ready to receive the image
         * @retval false Image size does not match the layout
        */
        static bool begin(uint32_t image_size, uint32_t crc32);

        /**
         * @brief Writes next data chunk
         * 
         * @param [in] data Chunk to store
         * 
         * @retval true Chunk stored (or deliberately skipped)
         * @retval false No update in progress, chunk out of place or flash operation failed
        */
        static bool write_chunk(std::span<const uint8_t> data);

        /**
         * @brief Finalize update process and check that update OK
         * 
         * @retval true Written image matches its stamped CRC32
         * @retval false Validation failed
        */
        static bool end();

    private:
        /**
         * @brief Return new (flashed) application start
         * 
         * @returns Target application start
        */
        static uint32_t get_new_app_start();

    private:
        static params_t _params;
    };
} //! namespace bomzhur

#include "impl/ota.h"

#endif //! OTA_OTA_H_
