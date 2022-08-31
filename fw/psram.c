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

static inline uint32_t __time_critical_func(psram_op)(uint32_t pc, uint32_t bits, bool want_irq) {
    // pc MUST be a valid PUBLIC offset e.g. spi_offset_write, spi_offset_read
    const uint32_t offs = 0; // TODO: get offset from initial SM setup

    return (want_irq << 31) | ((offs + pc) << (31-5)) | ((bits-1) << (31-10));
}

static inline void __time_critical_func(psram_cmd0)(PIO pio, uint sm, uint32_t cmd) {
    psram_clr_ce(pio, sm);
    // write 8 bits:
    pio_sm_put_blocking(pio, sm, psram_op(spi_offset_write, 8, true));
    pio_sm_put_blocking(pio, sm, cmd << 24);
    psram_wait_for_completion(pio);
    psram_set_ce(pio, sm);
}

void __time_critical_func(psram_reset)(PIO pio, uint sm) {
    psram_cmd0(pio, sm, 0x66UL); // reset enable
    psram_cmd0(pio, sm, 0x99UL); // reset
}

void __time_critical_func(psram_read_eid)(PIO pio, uint sm) {
    #if 0
    psram_clr_ce(pio, sm);
    // write 32 bits, cmd 0x9F, address 0xAA_AA_AA
    pio_sm_put_blocking(pio, sm, ((32UL) << 24) | (0x9FUL << 16) | 0xAAAAUL);
    // wait 1, read 64 bits
    pio_sm_put_blocking(pio, sm, 0x55000000UL | (1UL << 16) | ((64UL) << 8));

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

void __time_critical_func(psram_write)(PIO pio, uint sm) {
    psram_clr_ce(pio, sm);
    // write 32 bits, cmd 0x02, address 0x00_00_00:
    pio_sm_put_blocking(pio, sm, psram_op(spi_offset_write, 32, false));
    pio_sm_put_blocking(pio, sm, 0x02000000UL);
    // write 8 bits:
    pio_sm_put_blocking(pio, sm, psram_op(spi_offset_write,  8, true));
    pio_sm_put_blocking(pio, sm, 0xAA000000UL);
    psram_wait_for_completion(pio);
    psram_set_ce(pio, sm);
}

void __time_critical_func(psram_read)(PIO pio, uint sm) {
    psram_clr_ce(pio, sm);
    // write 32 bits, cmd 0x0B, address 0x00_00_00
    pio_sm_put_blocking(pio, sm, psram_op(spi_offset_write, 32, false));
    pio_sm_put_blocking(pio, sm, 0x0B000000UL);
    // wait 8 bits:
    pio_sm_put_blocking(pio, sm, psram_op(spi_offset_write,  8, false));
    pio_sm_put_blocking(pio, sm, 0x00000000UL);
    // read 8 bits:
    pio_sm_put_blocking(pio, sm, psram_op(spi_offset_read,   8,  true));
    for (int i = 0; i < 1; i++) {
        printf("await read\n");
        uint32_t data = pio_sm_get_blocking(pio, sm);
        printf("read %08x\n", data);
    }
    //printf("wait_for_completion\n");
    psram_wait_for_completion(pio);
    psram_set_ce(pio, sm);
}
