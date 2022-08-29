#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "psram.pio.h"
#include "cart.h"

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

void __time_critical_func(psram_reset)(PIO pio, uint sm) {
    uint8_t cmd[2] = {
        0x66,
        0x99
    };

    gpio_put(PCS_B_PSRAM_CE, false);
    pio_spi_write8_blocking(pio, sm, &cmd[0], 1);
    gpio_put(PCS_B_PSRAM_CE, true);
    gpio_put(PCS_B_PSRAM_CE, true);
    gpio_put(PCS_B_PSRAM_CE, true);
    gpio_put(PCS_B_PSRAM_CE, true);
    gpio_put(PCS_B_PSRAM_CE, false);
    pio_spi_write8_blocking(pio, sm, &cmd[1], 1);
    gpio_put(PCS_B_PSRAM_CE, true);
}

void __time_critical_func(psram_read_eid)(PIO pio, uint sm) {
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
}
