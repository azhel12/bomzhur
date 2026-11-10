/**
 * @file main.cpp
 *
 * @brief Contains first-stage bootloader
*/

#include "../boot_rules.h"
#include "../src/utils/crc32.h"

#include <ch32v00x.h>

#include <cstdint>
#include <span>

/// @brief Application 0 address
static constexpr uint32_t app0_address = APP0_START;
/// @brief Application 1 address
static constexpr uint32_t app1_address = APP1_START;

/// @brief Application 0 update counter
#define APP0_UPDATE_NUMBER (*reinterpret_cast<uint32_t*>(app0_address + UPDATE_COUNTER_APP_OFFSET))
#define APP0_UPDATE_NUMBER_INVERSED (*reinterpret_cast<uint32_t*>(app0_address + UPDATE_COUNTER_APP_OFFSET + sizeof(uint32_t)))

/// @brief Application 1 update counter
#define APP1_UPDATE_NUMBER (*reinterpret_cast<uint32_t*>(app1_address + UPDATE_COUNTER_APP_OFFSET))
#define APP1_UPDATE_NUMBER_INVERSED (*reinterpret_cast<uint32_t*>(app1_address + UPDATE_COUNTER_APP_OFFSET + sizeof(uint32_t)))

/**
 * @brief Start given application
 *
 * @param startup_address Application entry point
*/
__attribute__((naked, noreturn)) void start_application(uint32_t startup_address) {
    __asm volatile("jr a0");
}

int main()
{
    // Validate both applications
    uint32_t app0_calculated_crc32 = bomzhur::crc32::calculate(std::span(reinterpret_cast<const uint8_t*>(app0_address), APP_SIZE));
    uint32_t app0_expected_crc32 = *reinterpret_cast<const uint32_t*>(app0_address + APP_CRC32_OFFSET);
    bool app0_valid = (app0_calculated_crc32 == app0_expected_crc32) && (APP0_UPDATE_NUMBER == ~APP0_UPDATE_NUMBER_INVERSED);

    uint32_t app1_calculated_crc32 = bomzhur::crc32::calculate(std::span(reinterpret_cast<const uint8_t*>(app1_address), APP_SIZE));
    uint32_t app1_expected_crc32 = *reinterpret_cast<const uint32_t*>(app1_address + APP_CRC32_OFFSET);
    bool app1_valid = (app1_calculated_crc32 == app1_expected_crc32) && (APP1_UPDATE_NUMBER == ~APP1_UPDATE_NUMBER_INVERSED);

    uint32_t app_address_for_run = app0_address;

    // Choose valid application
    if (app0_valid && app1_valid) {
        app_address_for_run = (APP1_UPDATE_NUMBER > APP0_UPDATE_NUMBER)
            ? app1_address
            : app0_address;
    } else if (app0_valid) {
        app_address_for_run = app0_address;
    } else if (app1_valid) {
        app_address_for_run = app1_address;
    }

    start_application(app_address_for_run);
}
