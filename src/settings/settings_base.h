/**
 * @file settings_base.h
 * 
 * @brief Implements base features of settings module
*/

#ifndef SETTINGS_SETTINGS_BASE_H_
#define SETTINGS_SETTINGS_BASE_H_

#include "../json/json.h"

#include <zhele/flash.h>

#include <cstdint>
#include <span>
#include <string_view>

namespace bomzhur {
    /**
     * @brief Persistent settings stored as JSON in one flash page
     *
     * @tparam _whole_settings Settings struct, every top-level member is a section
     * @tparam settings_flash_page Flash page holding the settings
    */
    template<typename _whole_settings, unsigned settings_flash_page>
    class settings_base {
        /**
         * @brief Flash settings header
        */
        struct settings_flash_header_t {
            uint32_t length;
            uint32_t crc32;
        };

    public:
        /// @brief Settings struct type
        using data_t = _whole_settings;

        /// @brief Upper bound of the whole-settings dump, terminating '\0' included
        static constexpr unsigned max_dump_size = json_max_size<_whole_settings>() + 1;

        static_assert(json_token_count<_whole_settings>() <= max_json_tokens,
            "Settings do not fit max_json_tokens: they would silently fail to load");
        static_assert(sizeof(settings_flash_header_t) + max_dump_size <= Zhele::Flash::PageSize(settings_flash_page),
            "Settings dump does not fit the flash page");

        /**
         * @brief Dumps given section as JSON
         * 
         * @param [in] section Section name
         * @param [in, out] where Target buffer
         * 
         * @returns Pointer after last written byte, nullptr if there is no such section
        */
        static char* dump_section_to_json(std::string_view section, char* where);

        /**
         * @brief Update settings for given section and persist them
         * 
         * @param [in] section Section
         * @param [in] data Settings update JSON
         * @param [in] scratch Buffer for the whole-settings JSON dump, at least max_dump_size
         *
         * @retval true Settings updated and stored
         * @retval false No such section, or flash operation failed
        */
        static bool set_from_json(std::string_view section, std::string_view data, std::span<char> scratch);

        /**
         * @brief Load settings from flash
         * 
         * @retval true Stored settings loaded
         * @retval false Nothing valid stored, defaults are used
        */
        static bool load();

    protected:
        static _whole_settings _data;
    };
}

#include "impl/settings_base.h"

#endif //!SETTINGS_SETTINGS_BASE_H_
