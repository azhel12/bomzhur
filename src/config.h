/**
 * @file config.h
 *
 * @brief Contains global project configs
*/

#ifndef CONFIG_H_
#define CONFIG_H_

#include <zhele/dma.h>
#include <zhele/iopins.h>
#include <zhele/pinlist.h>
#include <zhele/usart.h>

namespace config {
    /// @brief Target HCLK: HSI (24 MHz) through the fixed x2 PLL
    static constexpr unsigned sysclock_freq = 48000000;

    /**
     * RPC transport: USART1 over RS485.
     *
     * Zhele wires only the reset-default USART1 pin pair for CH32V00x
     * (TX/PD5, RX/PD6) with DMA1 channel 4 (TX) and channel 5 (RX) — the DMA
     * channels are baked into Zhele::Usart1, unlike the STM32 port where they
     * are template parameters.
     *
     * USART v1 has no hardware driver-enable output, so the RS485 DE pin is an
     * ordinary GPIO: raised before the DMA transfer starts and lowered from the
     * transmission-complete interrupt (see rpc::uart_transport_irq_handler).
     */
    using rpc_usart = Zhele::Usart1;
    using rpc_usart_dma_tx = rpc_usart::DmaTx;
    using rpc_usart_dma_rx = rpc_usart::DmaRx;
    using rpc_usart_tx_pin = Zhele::IO::Pd5;
    using rpc_usart_rx_pin = Zhele::IO::Pd6;
    // DE sits on PA2 (SOP-8 pin 3). PD4 would have been the natural choice, but on
    // this package it is bonded to the same physical pin as PD5 and PD1.
    using rpc_usart_rs485_de_pin = Zhele::IO::Pa2;
    static constexpr auto rpc_usart_baud = 115200;

    /**
     * Buffers live in the 2 KB of SRAM the part has, together with the stack
     * (0x280 in the linker scripts) — keep an eye on the total when growing them.
     * The RX buffer also caps the ota.write chunk size.
     */
    static constexpr auto rpc_rx_buffer_size = 256;
    static constexpr auto rpc_tx_buffer_size = 256;
    static constexpr uint8_t rpc_device_address = 4;

    #define RPC_USART_IRQ_HANDLER USART1_IRQHandler
    #define RPC_USART_TX_DMA_IRQ_HANDLER DMA1_Channel4_IRQHandler
    #define RPC_USART_RX_DMA_IRQ_HANDLER DMA1_Channel5_IRQHandler

    /**
     * @brief How long sys.reboot waits before resetting
    */
    static constexpr unsigned reboot_delay_ms = 50;

    /**
     * @brief Demo LEDs
    */
    using led_pins = Zhele::IO::PinList<Zhele::IO::Pc1, Zhele::IO::Pc2, Zhele::IO::Pc4>;
    static constexpr unsigned led_count = 3;

    /// @brief Flash page used to persist settings (see boot_rules.h layout)
    static constexpr unsigned settings_flash_page = 1;

    /// @brief Demo setting, replace with the real device settings
    static constexpr unsigned default_example_value = 42;
}

#endif //! CONFIG_H_
