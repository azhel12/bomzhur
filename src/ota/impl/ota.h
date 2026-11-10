/**
 * @file ota.h
 * 
 * @brief Contains ota definitions
*/

#ifndef OTA_IMPL_OTA_H_
#define OTA_IMPL_OTA_H_

#include "../../../boot_rules.h"

#include "../../utils/crc32.h"
#include <zhele/flash.h>

#include <cstdint>

namespace bomzhur {

    template<uint32_t _application_size, uint32_t _first_app_begin, uint32_t _second_app_begin>
    inline unsigned ota<_application_size, _first_app_begin, _second_app_begin>::get_current_app_number() {
        uint32_t this_method_address = reinterpret_cast<uint32_t>(get_current_app_number);
        return this_method_address < _second_app_begin ? 0 : 1;
    }
    
    template<uint32_t _application_size, uint32_t _first_app_begin, uint32_t _second_app_begin>
    inline bool ota<_application_size, _first_app_begin, _second_app_begin>::begin(uint32_t image_size, uint32_t crc32) {
        if (image_size != 2 * _slot_payload_size)
            return false;

        _params.current_write_address = reinterpret_cast<uint8_t*>(get_new_app_start());

        _params.received_size = 0;
        _params.last_erased_page = Zhele::Flash::AddressToPage(_params.current_write_address) - 1;

        return true;
    }

    template<uint32_t _application_size, uint32_t _first_app_begin, uint32_t _second_app_begin>
    inline bool ota<_application_size, _first_app_begin, _second_app_begin>::write_chunk(std::span<const uint8_t> data) {
        if (_params.current_write_address == nullptr)
            return false;

        uint32_t size = data.size();
        uint32_t chunk_begin = _params.received_size;
        uint32_t chunk_end = chunk_begin + size;

        if (chunk_end > 2 * _slot_payload_size || (chunk_begin < _slot_payload_size && chunk_end > _slot_payload_size))
            return false;

        _params.received_size = chunk_end;

        bool second_half = chunk_begin >= _slot_payload_size;
        if (second_half != (get_current_app_number() == 0))
            return true;

        bool is_last_chunk = (chunk_end % _slot_payload_size) == 0;

        if (is_last_chunk && size < UPDATE_COUNTER_SIZE)
            return false;

        // Check that this chunk affects next page and erase it if needed
        if (Zhele::Flash::AddressToPage(_params.current_write_address + size - 1) > _params.last_erased_page) {
            ++_params.last_erased_page;
            if(!Zhele::Flash::ErasePage(_params.last_erased_page))
                return false;
        }

        // Last chunk, avoid update counter writing
        if (is_last_chunk) {
            size -= UPDATE_COUNTER_SIZE;
        }

        if(!Zhele::Flash::WriteFlash(_params.current_write_address, data.data(), size)) {
            return false;
        }
        _params.current_write_address += size;

        if(is_last_chunk) {
            uint32_t update_counter[2];
            uint32_t current_app_begin = get_current_app_number() == 0 ? _first_app_begin : _second_app_begin;
            uint32_t new_flash_number = *reinterpret_cast<uint32_t*>(current_app_begin + UPDATE_COUNTER_APP_OFFSET) + 1;
            update_counter[0] = new_flash_number;
            update_counter[1] = ~new_flash_number;

            if(!Zhele::Flash::WriteFlash(_params.current_write_address, update_counter, sizeof(update_counter))) {
                return false;
            }
        }
        
        return true;
    }

    template<uint32_t _application_size, uint32_t _first_app_begin, uint32_t _second_app_begin>
    inline bool ota<_application_size, _first_app_begin, _second_app_begin>::end() {
        _params.current_write_address = nullptr;

        uint32_t calculated_crc32 = crc32::calculate(std::span(reinterpret_cast<const uint8_t*>(get_new_app_start()), _application_size));
        uint32_t expected_crc32 = *reinterpret_cast<const uint32_t*>(get_new_app_start() + APP_CRC32_OFFSET);

        return calculated_crc32 == expected_crc32;
    }

    template<uint32_t _application_size, uint32_t _first_app_begin, uint32_t _second_app_begin>
    inline uint32_t ota<_application_size, _first_app_begin, _second_app_begin>::get_new_app_start() {
        return get_current_app_number() == 0
            ? _second_app_begin
            : _first_app_begin;
    }

    /// @brief Updater data definition
    template<uint32_t _application_size, uint32_t _first_app_begin, uint32_t _second_app_begin>
    inline ota<_application_size, _first_app_begin, _second_app_begin>::params_t ota<_application_size, _first_app_begin, _second_app_begin>::_params;
} //! namespace bomzhur

#endif //! OTA_IMPL_OTA_H_
