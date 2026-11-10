/**
 * @file settings_base.h
 * 
 * @brief Contains settings_base definitions
*/

#ifndef SETTINGS_IMPL_SETTINGS_BASE_H_
#define SETTINGS_IMPL_SETTINGS_BASE_H_

#include "../../json/json.h"
#include "../../utils/crc32.h"
#include <zhele/flash.h>

#include <reflect/reflect>

#include <cstdint>
#include <string_view>

namespace bomzhur {
    template<typename _whole_settings, unsigned settings_flash_page>
    inline char* settings_base<_whole_settings, settings_flash_page>::dump_section_to_json(std::string_view section, char* where) {
        char* end = nullptr;

        reflect::for_each([&](auto field) {
            if (reflect::member_name<field>(_data) == section) {
                end = write_json(where, reflect::get<field>(_data));
            }
        }, _data);

        return end;
    }

    template<typename _whole_settings, unsigned settings_flash_page>
    inline bool settings_base<_whole_settings, settings_flash_page>::set_from_json(std::string_view section, std::string_view data, std::span<char> scratch) {
        bool found = false;

        reflect::for_each([&](auto field) {
            if (reflect::member_name<field>(_data) == section) {
                update_json(data, reflect::get<field>(_data));
                found = true;
            }
        }, _data);

        if (!found)
            return false;

        auto json_end = write_json(scratch.data(), _data);
        *json_end++ = '\0';

        // 4 bytes length + 4 bytes crc32 + JSON
        settings_flash_header_t header = {
            .length = static_cast<uint32_t>(json_end - scratch.data()),
            .crc32 = crc32::calculate(std::span(reinterpret_cast<const uint8_t*>(scratch.data()), json_end - scratch.data()))
        };

        auto page = reinterpret_cast<uint8_t*>(Zhele::Flash::PageAddress(settings_flash_page));

        return Zhele::Flash::ErasePage(settings_flash_page)
            && Zhele::Flash::WriteFlash(page, &header, sizeof(header))
            && Zhele::Flash::WriteFlash(page + sizeof(header), scratch.data(), header.length);
    }

    template<typename _whole_settings, unsigned settings_flash_page>
    inline bool settings_base<_whole_settings, settings_flash_page>::load() {
        auto header = reinterpret_cast<const settings_flash_header_t*>(Zhele::Flash::PageAddress(settings_flash_page));
        if (header->length > Zhele::Flash::PageSize(settings_flash_page) - sizeof(settings_flash_header_t))
            return false;
        
        auto stored = std::span(reinterpret_cast<const uint8_t*>(Zhele::Flash::PageAddress(settings_flash_page) + sizeof(settings_flash_header_t)), header->length);
        uint32_t actual_crc32 = crc32::calculate(stored);
        if (header->crc32 != actual_crc32)
            return false;

        update_json(std::string_view(reinterpret_cast<const char*>(stored.data()), stored.size()), _data);
        return true;
    }

    template<typename _whole_settings, unsigned settings_flash_page>
    inline _whole_settings settings_base<_whole_settings, settings_flash_page>::_data;
}
#endif //!SETTINGS_IMPL_SETTINGS_BASE_H_
