/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#if LOGIC_ANALYZER
#include "hardware/dma.h"
#endif

#include "cart.h"
#include "psram.pio.h"
#include "psram.h"

#if 0
uint8_t psram_spi_read() {
    // assumes CE#=0
    uint8_t d = 0;
    printf("reading...\n");
    // send out bits of cmd MSB to LSB order to SIO0 pin:
    for (int i = 0; i < 8; i++) {
        // clr clock:
        gpio_clr_mask(PCS_MS_CLK_133MHZ);
        printf("%018b CLK=0\n", gpio_get_all());
        // set clock:
        gpio_set_mask(PCS_MS_CLK_133MHZ);
        printf("%018b CLK=1 (read)\n", gpio_get_all());
        // read:
        d |= gpio_get(PCS_B_PSRAM_SIO1);
        d <<= 1;
    }
    printf("read %02x (%08b)\n", d, d);
    return d;
}

void psram_spi_write(uint8_t d) {
    // assumes CE#=0
    printf("writing %02x (%08b)...\n", d, d);
    // send out bits of cmd MSB to LSB order to SIO0 pin:
    for (int i = 0; i < 8; i++, d <<= 1) {
        uint dpin = ((((uint32_t)d & 0x80U) >> 7) << PCS_B_PSRAM_SIO0);
        // clr clock and set data:
        gpio_put_masked(
            // mask:
            PCS_MS_PSRAM_SIO0 |
            PCS_MS_CLK_133MHZ,
            // value:
            PCS_MC_CLK_133MHZ | // clr CLK
            dpin                // data bit from MSB
        );
        printf("%018b CLK=0\n", gpio_get_all());
        // set clock:
        gpio_set_mask(PCS_MS_CLK_133MHZ);
        printf("%018b CLK=1 (write)\n", gpio_get_all());
    }
}

void psram_spi_cmd0(uint8_t cmd) {
    // CE# clr:
    printf("%018b command start\n", gpio_get_all());
    gpio_clr_mask(PCS_MS_PSRAM_CE | PCS_MS_CLK_133MHZ);
    printf("%018b CE#=0\n", gpio_get_all());
    printf("%018b CE#=0\n", gpio_get_all());

    // send out bits of cmd MSB to LSB order to SIO0 pin:
    psram_spi_write(cmd);
    
    // CE# set:
    printf("%018b\n", gpio_get_all());
    printf("%018b\n", gpio_get_all());
    gpio_clr_mask(PCS_MS_CLK_133MHZ);
    printf("%018b CLK=0\n", gpio_get_all());
    gpio_set_mask(PCS_MS_PSRAM_CE);
    printf("%018b CE#=1\n", gpio_get_all());
    printf("%018b command end\n", gpio_get_all());
}

void psram_spi_read_eid() {
    // CE# clr:
    printf("%018b read eid start\n", gpio_get_all());
    gpio_clr_mask(PCS_MS_PSRAM_CE | PCS_MS_CLK_133MHZ);
    printf("%018b CE#=0, CLK=0\n", gpio_get_all());
    printf("%018b\n", gpio_get_all());
    // send out bits of cmd MSB to LSB order to SIO0 pin:
    psram_spi_write(0x9F);
    psram_spi_write(0xAA);
    psram_spi_write(0xAA);
    psram_spi_write(0xAA);

    uint8_t mfid, kgd;
    mfid = psram_spi_read();
    kgd = psram_spi_read();
    uint8_t eid[6];
    eid[0] = psram_spi_read();
    eid[1] = psram_spi_read();
    eid[2] = psram_spi_read();
    eid[3] = psram_spi_read();
    eid[4] = psram_spi_read();
    eid[5] = psram_spi_read();
    printf("%018b\n", gpio_get_all());
    // CE# set:
    gpio_clr_mask(PCS_MS_CLK_133MHZ);
    printf("%018b CLK=0\n", gpio_get_all());
    gpio_set_mask(PCS_MS_PSRAM_CE);
    printf("%018b CE#=1\n", gpio_get_all());
    printf("%018b read eid end\n", gpio_get_all());

    printf("%02x %02x %02x%02x%02x%02x%02x%02x\n", mfid, kgd, eid[0], eid[1], eid[2], eid[3], eid[4], eid[5]);
}
#endif

#if LOGIC_ANALYZER
void la_init(PIO pio, uint sm, uint pin_base, uint pin_count, float div);
void la_arm(PIO pio, uint sm, uint dma_chan, uint32_t *capture_buf, size_t capture_size_words, uint trigger_pin, bool trigger_level);
void la_print_capture_buf(const uint32_t *buf, uint pin_base, uint pin_count, uint32_t n_samples);
uint la_calc_buf_size_words(uint samples, uint pins);
#endif

int main() {
    stdio_init_all();

    // take software control over PSRAM to initialize it:
    gpio_set_function(PCS_B_PSRAM_SIO0, GPIO_FUNC_SIO);
    gpio_set_function(PCS_B_PSRAM_SIO1, GPIO_FUNC_SIO);
    gpio_set_function(PCS_B_PSRAM_SIO2, GPIO_FUNC_SIO);
    gpio_set_function(PCS_B_PSRAM_SIO3, GPIO_FUNC_SIO);
    gpio_set_function(PCS_B_PSRAM_CE, GPIO_FUNC_SIO);
    gpio_set_function(PCS_B_CLK_133MHZ, GPIO_FUNC_SIO);

    // init GPIO:
    gpio_set_dir_in_masked(
        PCS_MS_D0 |
        PCS_MS_D1 |
        PCS_MS_D2 |
        PCS_MS_D3 |
        PCS_MS_D4 |
        PCS_MS_D5 |
        PCS_MS_D6 |
        PCS_MS_D7 |
        PCS_MS_ADDR1QH |
        PCS_MS_ADDR2QH |
        PCS_MS_ADDR3QH |
        PCS_MS_ADDR4QH |
        PCS_MS_PSRAM_SIO1 |
        PCS_MS_PSRAM_SIO2 |
        PCS_MS_PSRAM_SIO3 |
        PCS_MS_WRAM_OR_CLK |
        PCS_MS_RD |
        PCS_MS_WR |
        PCS_MS_CE
    );

    gpio_set_dir_out_masked(
        PCS_MS_PSRAM_SIO0 |
        PCS_MS_PSRAM_CE |
        PCS_MS_CLK_133MHZ |

        PCS_MS_ADDR_CE |
        PCS_MS_ADDR_LD |
        PCS_MS_DDIR |
        PCS_MS_RESET
    );

    gpio_set_outover(PCS_B_PSRAM_CE, GPIO_OVERRIDE_NORMAL);

    // init PSRAM:
    gpio_put_masked(
        // mask:
        PCS_MS_PSRAM_CE |   // set
        PCS_MS_PSRAM_SIO0 | // clr
        PCS_MS_PSRAM_SIO1 | // clr
        PCS_MS_PSRAM_SIO2 | // clr
        PCS_MS_PSRAM_SIO3 | // clr
        PCS_MS_CLK_133MHZ,  // clr
        // value:
        PCS_MS_PSRAM_CE |   // set
        PCS_MC_PSRAM_SIO0 | // clr
        PCS_MC_PSRAM_SIO1 | // clr
        PCS_MC_PSRAM_SIO2 | // clr
        PCS_MC_PSRAM_SIO3 | // clr
        PCS_MC_CLK_133MHZ   // clr
    );

    // wait for PSRAM to initialize:
    sleep_us(160);

    // JSD: wait for USB connection:
    while (!stdio_usb_connected()) {
        sleep_ms(10);
    }

    puts("sys: set 48Mhz...");
    set_sys_clock_48mhz();

#if LOGIC_ANALYZER
#define CAPTURE_N_SAMPLES 224
#define CAPTURE_PIN_COUNT 6
#define CAPTURE_PIN_BASE PCS_B_PSRAM_SIO0

    uint      la_buf_size_words = la_calc_buf_size_words(CAPTURE_N_SAMPLES, CAPTURE_PIN_COUNT);
    uint32_t *la_capture_buf = malloc(la_buf_size_words * sizeof(uint32_t));
    hard_assert(la_capture_buf);

    PIO  la_pio = pio1;
    uint la_sm = 0;
    uint la_dma_chan = 0;

    puts("la_init...");
    la_init(la_pio, la_sm, CAPTURE_PIN_BASE, CAPTURE_PIN_COUNT, 1.f);
#endif

#if 0
    // reset PSRAM:
    puts("psram: reset enable...");
    psram_spi_cmd0(0x66);  // Reset Enable
    puts("psram: reset...");
    psram_spi_cmd0(0x99);  // Reset

    puts("psram: read eid...");
    psram_spi_read_eid();
#endif

    puts("psram: load SPI PIO...");

    // Load the psram_qspi_write program, and configure a free state machine
    // to run the program.
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &spi_cs_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_spi_cs_init(pio, sm, offset,
        8,
        1.0f,
        false,
        PCS_B_CLK_133MHZ,
        PCS_B_PSRAM_SIO0,
        PCS_B_PSRAM_SIO1
    );
 
#if LOGIC_ANALYZER
    puts("la_arm...");
    la_arm(la_pio, la_sm, la_dma_chan, la_capture_buf, la_buf_size_words, PCS_B_PSRAM_CE, false);
#endif

    // to reset PSRAM, n-2=6 h66 reset-enable, n-2=6 h99 reset:
    psram_reset(pio, sm);

#if LOGIC_ANALYZER
    // The logic analyser should have started capturing as soon as it saw the
    // first transition. Wait until the last sample comes in from the DMA.
    dma_channel_wait_for_finish_blocking(la_dma_chan);

    la_print_capture_buf(la_capture_buf, CAPTURE_PIN_BASE, CAPTURE_PIN_COUNT, CAPTURE_N_SAMPLES);

    la_arm(la_pio, la_sm, la_dma_chan, la_capture_buf, la_buf_size_words, PCS_B_PSRAM_CE, false);
#endif

    psram_read_eid(pio, sm);

#if LOGIC_ANALYZER
    // The logic analyser should have started capturing as soon as it saw the
    // first transition. Wait until the last sample comes in from the DMA.
    dma_channel_wait_for_finish_blocking(la_dma_chan);

    la_print_capture_buf(la_capture_buf, CAPTURE_PIN_BASE, CAPTURE_PIN_COUNT, CAPTURE_N_SAMPLES);
#endif

#if 0
    puts("psram: enter quad mode...");

    // switch PSRAM to quad mode:
    psram_spi_cmd0(0x35);

    puts("psram: load PIO...");

    // Load the psram_qspi_write program, and configure a free state machine
    // to run the program.
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &psram_qspi_program);
    uint sm = pio_claim_unused_sm(pio, true);
    psram_qspi_program_init(pio, sm, offset, PCS_B_PSRAM_SIO0);
 
    // write 8 nybbles, 2 for cmd and 6 for addr:
    puts("psram: write 8 nybbles...");
    psram_write(pio, sm, offset);
    pio_sm_put_blocking(pio, sm, 0x07UL);                           // count-1 of data
    pio_sm_put_blocking(pio, sm, reverse_nybbles(0x38000000UL));    // cmd, addr
    pio_sm_put_blocking(pio, sm, reverse_nybbles(0x12345678UL));    // data

    puts("psram: wait...");
    while (!pio_sm_is_exec_stalled(pio, sm)) tight_loop_contents();
#endif

    puts("done\n");
}