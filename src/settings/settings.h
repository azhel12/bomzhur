/**
 * @file settings.h
 *
 * @brief Implements settings module
*/

#ifndef SETTINGS_SETTINGS_H_
#define SETTINGS_SETTINGS_H_

#include "settings_base.h"

#include "../config.h"

namespace bomzhur {
    /**
     * @brief Whole device settings struct
    */
    struct settings_t {
        struct example_settings_t {
            uint32_t val1 = config::default_example_value;
        } sec1;
    };

    /**
     * @brief Implements device settings
    */
    class settings : public settings_base<settings_t, config::settings_flash_page> {
    public:
        /**
         * @brief Returns example setting value
         *
         * @returns Example value
        */
        static inline uint32_t get_example_value() {
            return _data.sec1.val1;
        }
    };
}
#endif //! SETTINGS_SETTINGS_H_
