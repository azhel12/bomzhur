/**
 * @file leds.h
 *
 * @brief Demo payload: three LEDs driven over RPC
*/

#ifndef LEDS_H_
#define LEDS_H_

#include "config.h"

namespace bomzhur {
    /**
     * @brief Implements LED control
    */
    class leds {
        using pins = config::led_pins;

    public:
        /// @brief Number of controllable LEDs
        static constexpr unsigned count = config::led_count;

        /**
         * @brief Configure the LED pins as outputs, all off
         *
         * @par Returns
         *  Nothing
        */
        static void init() {
            pins::Enable();
            pins::SetConfiguration<pins::Configuration::Out>();
            pins::SetDriverType<pins::DriverType::PushPull>();
            pins::Write(0);
        }

        /**
         * @brief Checks that an LED number is in range
         *
         * @param [in] number LED number
         *
         * @retval true Number addresses an existing LED
         * @retval false Number is out of range
        */
        static constexpr bool valid(uint32_t number) {
            return number < count;
        }

        /**
         * @brief Turns one LED on
         *
         * @param [in] number LED number, must be valid()
         *
         * @par Returns
         *  Nothing
        */
        static void on(uint32_t number) {
            pins::Set(1u << number);
        }

        /**
         * @brief Turns one LED off
         *
         * @param [in] number LED number, must be valid()
         *
         * @par Returns
         *  Nothing
        */
        static void off(uint32_t number) {
            pins::Clear(1u << number);
        }
    };
}

#endif //! LEDS_H_
