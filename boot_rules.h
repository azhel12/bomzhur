/**
 * @file boot_rules.h
 *
 * @brief Layout shared by the bootloader (never updated!) and the applications
*/

#ifndef BOOT_RULES_H_
#define BOOT_RULES_H_

/// @brief App0 start address
#define APP0_START (0x00000800UL)
/// @brief App1 start address
#define APP1_START (0x00002400UL)
/// @brief Whole application slot size in bytes (APP + CRC32 + update counter)
#define APP_SLOT_SIZE (0x1C00U)
/// @brief Application size in bytes
#define APP_SIZE (0x1BF4U)
/// @brief Application CRC32 size
#define APP_CRC32_SIZE (4)
/// @brief Application update counter size (counter + inversed value)
#define UPDATE_COUNTER_SIZE (8)
/// @brief Application crc32 offset
#define APP_CRC32_OFFSET (0x1BF4)
/// @brief Application update counter offset
#define UPDATE_COUNTER_APP_OFFSET (0x1BF8)

#endif //! BOOT_RULES_H_
