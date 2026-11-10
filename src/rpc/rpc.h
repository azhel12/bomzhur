/**
 * @file rpc.h
 *
 * @brief Implements RPC transport and method table
*/

#ifndef RPC_RPC_H_
#define RPC_RPC_H_

#include "../../boot_rules.h"
#include "../config.h"
#include "../leds.h"
#include "../settings/settings.h"

#include "../ota/ota.h"
#include "../rpc/presets/settings.hpp"
#include "../rpc/presets/system.hpp"
#include "../rpc/presets/ota.hpp"

#include <span>

#include <zhele/delay.h>
#include <zhele/dma.h>

class rpc {
    /// @brief Reboot is scheduled
    static inline bool _reboot_pending = false;

    /**
     * @brief Schedules a reset for after the RPC answer has left the wire
     *
     * @param [in] timeout Requested delay in milliseconds, unused
     *
     * @par Returns
     *  Nothing
    */
    static inline void deferred_reboot(uint32_t timeout) {
        _reboot_pending = true;
    }

    /**
     * @brief led.on / led.off request
    */
    struct led_request_t {
        /// @brief LED number; the default is out of range, so a missing "n" fails
        uint32_t n = bomzhur::leds::count;
    };

    /// @brief Result of a successful led.on / led.off
    static constexpr std::string_view led_ok_response = R"("OK")";

    /**
     * @brief led.on handler: turns the requested LED on
    */
    static inline bomzhur::rpc::request_result_t rpc_led_on(std::string_view params, std::span<char> response, unsigned& response_size) {
        auto request = bomzhur::read_json<led_request_t>(params);

        if (!bomzhur::leds::valid(request.n))
            return bomzhur::rpc::request_result_t::invalid_params;

        bomzhur::leds::on(request.n);

        std::copy(led_ok_response.begin(), led_ok_response.end(), response.begin());
        response_size = led_ok_response.size();
        return bomzhur::rpc::request_result_t::ok;
    }

    /**
     * @brief led.off handler: turns the requested LED off
    */
    static inline bomzhur::rpc::request_result_t rpc_led_off(std::string_view params, std::span<char> response, unsigned& response_size) {
        auto request = bomzhur::read_json<led_request_t>(params);

        if (!bomzhur::leds::valid(request.n))
            return bomzhur::rpc::request_result_t::invalid_params;

        bomzhur::leds::off(request.n);

        std::copy(led_ok_response.begin(), led_ok_response.end(), response.begin());
        response_size = led_ok_response.size();
        return bomzhur::rpc::request_result_t::ok;
    }

    /// @brief OTA module
    using updater = bomzhur::ota<APP_SIZE, APP0_START, APP1_START>;
    /// @brief RPC server declaration
    using server = bomzhur::rpc::rpc_server<
        config::rpc_device_address,

        bomzhur::rpc::presets::settings<bomzhur::settings>::get_method,
        bomzhur::rpc::presets::settings<bomzhur::settings>::set_method,

        bomzhur::rpc::presets::system<deferred_reboot>::all_methods,
        bomzhur::rpc::presets::ota<updater>::all_methods,

        bomzhur::rpc::rpc_method<"led.on", rpc_led_on>,
        bomzhur::rpc::rpc_method<"led.off", rpc_led_off>
    >;

    static_assert(bomzhur::settings::max_dump_size <= server::result_capacity(config::rpc_tx_buffer_size),
        "Settings dump does not fit the RPC TX buffer");

    /// @brief A complete frame is sitting in _rx_buffer
    static inline volatile bool _frame_received = false;
    /// @brief Size of the received frame
    static inline volatile unsigned _rx_data_size = 0;

    /// @brief USART RX buffer
    alignas(alignof(uint32_t)) static inline uint8_t _rx_buffer[config::rpc_rx_buffer_size];
    /// @brief USART TX buffer
    alignas(alignof(uint32_t)) static inline uint8_t _tx_buffer[config::rpc_tx_buffer_size];

public:
    /**
     * @brief Initialize RPC API module
     *
     * @par Returns
     *  Nothing
    */
    static inline void init() {
        server::init();
        rpc_uart_transport_init();
    }

    /**
     * @brief Handle a received frame and any pending deferred reboot
     *
     * @par Returns
     *  Nothing
    */
    static inline void poll() {
        if (_frame_received) {
            _frame_received = false;

            unsigned answer_size = 0;
            if (server::handle_rx(std::span(_rx_buffer, _rx_data_size), _tx_buffer, answer_size)) {
                transmit(std::span(_tx_buffer, answer_size));
            }

            config::rpc_usart::EnableAsyncRead(_rx_buffer, sizeof(_rx_buffer), rx_buffer_full_handler);

            if (_reboot_pending) {
                Zhele::delay_ms<config::reboot_delay_ms>();
                NVIC_SystemReset();
            }
        }
    }

    /**
     * @brief USART interrupt handler
     *
     * @par Returns
     *  Nothing
    */
    static inline void uart_transport_irq_handler() {
        auto source = config::rpc_usart::InterruptSource();

        if (source & config::rpc_usart::InterruptFlags::IdleInt) {
            // USART v1 clears IDLE only by a STATR read followed by a DATAR read
            [[maybe_unused]] volatile uint32_t dummy = config::rpc_usart::Regs()->STATR;
            dummy = config::rpc_usart::Regs()->DATAR;

            config::rpc_usart::DmaRx::Disable();

            _rx_data_size = sizeof(_rx_buffer) - config::rpc_usart::DmaRx::RemainingTransfers();
            _frame_received = true;
        }

        if (source & config::rpc_usart::InterruptFlags::TxCompleteInt) {
            config::rpc_usart_rs485_de_pin::Clear();
            config::rpc_usart::DisableInterrupt(config::rpc_usart::InterruptFlags::TxCompleteInt);
            config::rpc_usart::ClearInterruptFlag(config::rpc_usart::InterruptFlags::TxCompleteInt);
        }
    }

private:
    /**
     * @brief Configure USART, RS485 driver-enable pin and DMA
     *
     * @par Returns
     *  Nothing
    */
    static inline void rpc_uart_transport_init() {
        using namespace config;

        if constexpr (std::is_same_v<rpc_usart_rx_pin, Zhele::IO::NullPin>) {
            rpc_usart::Init(rpc_usart_baud, rpc_usart::UsartMode::Default | rpc_usart::UsartMode::HalfDuplex);
            rpc_usart::SelectTxRxPins<rpc_usart_tx_pin, rpc_usart_tx_pin>();
        } else {
            rpc_usart::Init(rpc_usart_baud);
            rpc_usart::SelectTxRxPins<rpc_usart_tx_pin, rpc_usart_rx_pin>();
        }

        rpc_usart_rs485_de_pin::Port::Enable();
        rpc_usart_rs485_de_pin::template SetConfiguration<rpc_usart_rs485_de_pin::Port::Configuration::Out>();
        rpc_usart_rs485_de_pin::template SetDriverType<rpc_usart_rs485_de_pin::Port::DriverType::PushPull>();
        rpc_usart_rs485_de_pin::Clear();

        Zhele::Dma1::Enable();

        rpc_usart::ClearAllInterruptFlags();
        rpc_usart::EnableInterrupt(rpc_usart::InterruptFlags::IdleInt);
        NVIC_EnableIRQ(USART1_IRQn);

        rpc_usart::EnableAsyncRead(_rx_buffer, sizeof(_rx_buffer), rx_buffer_full_handler);
    }

    /**
     * @brief Send an answer, holding the RS485 driver enabled for its duration
     *
     * @param [in] data Data to send
     *
     * @par Returns
     *  Nothing
    */
    static inline void transmit(std::span<const uint8_t> data) {
        config::rpc_usart_rs485_de_pin::Set();
        config::rpc_usart::WriteAsync(data.data(), data.size(), tx_dma_complete_handler);
    }

    /**
     * @brief DMA finished feeding the USART
    */
    static inline void tx_dma_complete_handler(void* data, unsigned size, bool success) {
        config::rpc_usart::EnableInterrupt(config::rpc_usart::InterruptFlags::TxCompleteInt);
    }

    /**
     * @brief Frame longer than the RX buffer
    */
    static inline void rx_buffer_full_handler(void* data, unsigned size, bool success) {
        config::rpc_usart::EnableAsyncRead(_rx_buffer, sizeof(_rx_buffer), rx_buffer_full_handler);
    }
};

#endif //! RPC_RPC_H_
