/**
 * @file main.cpp
 *
 * @brief Contains program entry point
*/

#include "config.h"

#include "leds.h"
#include "rpc/rpc.h"
#include "settings/settings.h"

#include <zhele/clock.h>
#include <zhele/flash.h>

/**
 * @brief Configure clocks to max speed
 *
 * @par Returns
 *  Nothing
*/
void configure_clock();

int main()
{
    configure_clock();
    bomzhur::leds::init();
    bomzhur::settings::load();
    rpc::init();

    for (;;) {
        rpc::poll();
    }
}

void configure_clock()
{
    using namespace Zhele::Clock;

    // Flash latch
    Zhele::Flash::ConfigureFrequence(config::sysclock_freq);

    // Park on HSI before touching the PLL, then HSI (24 MHz) x2 = 48 MHz.
    SysClock::SelectClockSource<SysClock::Internal>();
    AhbClock::SetPrescaler<AhbClock::Div1>();

    PllClock::Disable();
    PllClock::SelectClockSource<PllClock::Internal>();
    PllClock::SetMultiplier<2>();

    SysClock::SelectClockSource<SysClock::Pll>();
}

extern "C" {
    __attribute__((interrupt("WCH-Interrupt-fast")))
    void RPC_USART_IRQ_HANDLER() {
        rpc::uart_transport_irq_handler();
    }

    __attribute__((interrupt("WCH-Interrupt-fast")))
    void RPC_USART_TX_DMA_IRQ_HANDLER() {
        config::rpc_usart_dma_tx::IrqHandler();
    }

    __attribute__((interrupt("WCH-Interrupt-fast")))
    void RPC_USART_RX_DMA_IRQ_HANDLER() {
        config::rpc_usart_dma_rx::IrqHandler();
    }
}
