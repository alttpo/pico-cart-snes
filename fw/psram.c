#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "psram.pio.h"
#include "cart.h"

#if 0
void __time_critical_func(pio_spi_write8_blocking)(PIO pio, uint sm, const uint8_t *src, size_t len) {
    size_t tx_remain = len, rx_remain = len;
    // Do 8 bit accesses on FIFO, so that write data is byte-replicated. This
    // gets us the left-justification for free (for MSB-first shift-out)
    io_rw_8 *txfifo = (io_rw_8 *) &pio->txf[sm];
    io_rw_8 *rxfifo = (io_rw_8 *) &pio->rxf[sm];
    while (tx_remain || rx_remain) {
        if (tx_remain && !pio_sm_is_tx_fifo_full(pio, sm)) {
            *txfifo = *src++;
            --tx_remain;
        }
        if (rx_remain && !pio_sm_is_rx_fifo_empty(pio, sm)) {
            (void) *rxfifo;
            --rx_remain;
        }
    }
}

void __time_critical_func(pio_spi_read8_blocking)(PIO pio, uint sm, uint8_t *dst, size_t len) {
    size_t tx_remain = len, rx_remain = len;
    io_rw_8 *txfifo = (io_rw_8 *) &pio->txf[sm];
    io_rw_8 *rxfifo = (io_rw_8 *) &pio->rxf[sm];
    while (tx_remain || rx_remain) {
        if (tx_remain && !pio_sm_is_tx_fifo_full(pio, sm)) {
            *txfifo = 0;
            --tx_remain;
        }
        if (rx_remain && !pio_sm_is_rx_fifo_empty(pio, sm)) {
            *dst++ = *rxfifo;
            --rx_remain;
        }
    }
}

void __time_critical_func(pio_spi_write8_read8_blocking)(PIO pio, uint sm, uint8_t *src, uint8_t *dst,
                                                         size_t len) {
    size_t tx_remain = len, rx_remain = len;
    io_rw_8 *txfifo = (io_rw_8 *) &pio->txf[sm];
    io_rw_8 *rxfifo = (io_rw_8 *) &pio->rxf[sm];
    while (tx_remain || rx_remain) {
        if (tx_remain && !pio_sm_is_tx_fifo_full(pio, sm)) {
            *txfifo = *src++;
            --tx_remain;
        }
        if (rx_remain && !pio_sm_is_rx_fifo_empty(pio, sm)) {
            *dst++ = *rxfifo;
            --rx_remain;
        }
    }
}
#endif

static inline bool __time_critical_func(psram_is_completed)(PIO pio) {
    return pio_interrupt_get(pio, 0);
}

static inline void __time_critical_func(psram_clear_completion)(PIO pio) {
    pio_interrupt_clear(pio, 0);
}

static inline void __time_critical_func(psram_wait_for_completion)(PIO pio) {
    while (!psram_is_completed(pio)) tight_loop_contents();
    psram_clear_completion(pio);
}

static inline void __time_critical_func(psram_clr_ce)(PIO pio, uint sm) {
    gpio_put(PCS_B_PSRAM_CE, false);
}

static inline void __time_critical_func(psram_set_ce)(PIO pio, uint sm) {
    gpio_put(PCS_B_PSRAM_CE, true);
    gpio_put(PCS_B_PSRAM_CE, true);
    gpio_put(PCS_B_PSRAM_CE, true);
    gpio_put(PCS_B_PSRAM_CE, true);
}

static inline void __time_critical_func(psram_cmd0)(PIO pio, uint sm, uint32_t cmd) {
    psram_clr_ce(pio, sm);
    pio_sm_put_blocking(pio, sm, 0x07000000UL | (cmd << 8));
    psram_wait_for_completion(pio);
    psram_set_ce(pio, sm);
}

void __time_critical_func(psram_reset)(PIO pio, uint sm) {
    psram_cmd0(pio, sm, 0x66UL); // reset enable
    psram_cmd0(pio, sm, 0x99UL); // reset
}

void __time_critical_func(psram_read_eid)(PIO pio, uint sm) {
#if 0
    uint8_t cmd[13] = {
        // read eid command
        0x9F,
        // address (doesnt matter):
        0xAA, 0xAA, 0xAA,
        // dummy values to align with eid size:
        0x00
    };
    uint8_t buf[13] = { 0 };
    uint8_t *eid = &buf[4];

    gpio_put(PCS_B_PSRAM_CE, false);
    pio_spi_write8_read8_blocking(pio, sm, cmd, buf, 13);
    gpio_put(PCS_B_PSRAM_CE, true);

    printf(
        "%02x %02x %02x%02x%02x%02x%02x%02x\n",
        eid[0], eid[1], eid[2], eid[3], eid[4], eid[5], eid[6], eid[7]
    );
#else
    psram_clr_ce(pio, sm);
    // write 32 bits, read 96 bits, cmd 0x9F, address 0xAA_AA_AA
    pio_sm_put_blocking(pio, sm, ((32UL-1UL) << 24) | ((64UL-1UL) << 16) | 0x9FAAUL);
    pio_sm_put_blocking(pio, sm, 0xAA550000UL);
    // read in 8-bits from each FIFO element:
    for (int i = 0; i < 8; i++) {
       uint32_t data = pio_sm_get_blocking(pio, sm);
       printf("read %08x\n", data);
    }
    printf("wait_for_completion\n");
    psram_wait_for_completion(pio);
    psram_set_ce(pio, sm);
#endif
}
