/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "hardware/pwm.h"
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
    uint32_t d[2] = {0,0};

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

    // puts("sys: set 48Mhz...");
    // set_sys_clock_48mhz();
    // puts("sys: set 84Mhz...");
    // set_sys_clock_khz( 84000, true);
    // puts("sys: set 104Mhz...");
    // set_sys_clock_khz(104000, true);
    // puts("sys: set 133Mhz...");
    // set_sys_clock_khz(133000, true);
    // puts("sys: set 144Mhz...");
    // set_sys_clock_khz(144000, true);
    // puts("sys: set 208Mhz...");
    // set_sys_clock_khz(208000, true);
    puts("sys: set 266Mhz...");
    set_sys_clock_khz(266000, true);

    // 288MHz panics :(
    // puts("sys: set 288Mhz...");
    // set_sys_clock_khz(288000, true);

#if LOGIC_ANALYZER
#define CAPTURE_N_SAMPLES 384
#define CAPTURE_PIN_COUNT 6
#define CAPTURE_PIN_BASE PCS_B_PSRAM_SIO0

    uint      la_buf_size_words = la_calc_buf_size_words(CAPTURE_N_SAMPLES, CAPTURE_PIN_COUNT);
    uint32_t *la_capture_buf = malloc(la_buf_size_words * sizeof(uint32_t));
    hard_assert(la_capture_buf);

    PIO  la_pio = pio1;
    uint la_sm = pio_claim_unused_sm(la_pio, true);
    uint la_dma_chan = dma_claim_unused_channel(true);

    puts("la_init...");
    la_init(la_pio, la_sm, CAPTURE_PIN_BASE, CAPTURE_PIN_COUNT, 1.f);
#endif

#if LOGIC_ANALYZER
    puts("la_arm...");
    la_arm(la_pio, la_sm, la_dma_chan, la_capture_buf, la_buf_size_words, PCS_B_PSRAM_CE, false);
#endif

    puts("psram: init...");
    psram_init();

#if LOGIC_ANALYZER
    // The logic analyser should have started capturing as soon as it saw the
    // first transition. Wait until the last sample comes in from the DMA.
    dma_channel_wait_for_finish_blocking(la_dma_chan);

    la_print_capture_buf(la_capture_buf, CAPTURE_PIN_BASE, CAPTURE_PIN_COUNT, CAPTURE_N_SAMPLES);

    la_arm(la_pio, la_sm, la_dma_chan, la_capture_buf, la_buf_size_words, PCS_B_PSRAM_CE, false);
#endif

#if 0
    puts("psram: read_eid");
    psram_read_eid();

#if LOGIC_ANALYZER
    // The logic analyser should have started capturing as soon as it saw the
    // first transition. Wait until the last sample comes in from the DMA.
    dma_channel_wait_for_finish_blocking(la_dma_chan);

    la_print_capture_buf(la_capture_buf, CAPTURE_PIN_BASE, CAPTURE_PIN_COUNT, CAPTURE_N_SAMPLES);

    la_arm(la_pio, la_sm, la_dma_chan, la_capture_buf, la_buf_size_words, PCS_B_PSRAM_CE, false);
#endif
#endif

#if 1
    puts("psram: write");
    d[0] = 0x55aa0000UL;
    d[1] = 0;
    psram_write(0x000000UL, d, 2);

#if LOGIC_ANALYZER
    // The logic analyser should have started capturing as soon as it saw the
    // first transition. Wait until the last sample comes in from the DMA.
    dma_channel_wait_for_finish_blocking(la_dma_chan);

    la_print_capture_buf(la_capture_buf, CAPTURE_PIN_BASE, CAPTURE_PIN_COUNT, CAPTURE_N_SAMPLES);

    la_arm(la_pio, la_sm, la_dma_chan, la_capture_buf, la_buf_size_words, PCS_B_PSRAM_CE, false);
#endif
#endif

#if 1
    puts("psram: read");
    d[0] = 0;
    d[1] = 0;
    psram_read(0x000000UL, d, 2);

    printf("read %02x %02x\n", d[0], d[1]);

#if LOGIC_ANALYZER
    // The logic analyser should have started capturing as soon as it saw the
    // first transition. Wait until the last sample comes in from the DMA.
    dma_channel_wait_for_finish_blocking(la_dma_chan);

    la_print_capture_buf(la_capture_buf, CAPTURE_PIN_BASE, CAPTURE_PIN_COUNT, CAPTURE_N_SAMPLES);

    la_arm(la_pio, la_sm, la_dma_chan, la_capture_buf, la_buf_size_words, PCS_B_PSRAM_CE, false);
#endif
#endif

#if 1
    puts("psram: quad mode");
    psram_set_qpi_mode();

#if LOGIC_ANALYZER
    // The logic analyser should have started capturing as soon as it saw the
    // first transition. Wait until the last sample comes in from the DMA.
    dma_channel_wait_for_finish_blocking(la_dma_chan);

    la_print_capture_buf(la_capture_buf, CAPTURE_PIN_BASE, CAPTURE_PIN_COUNT, CAPTURE_N_SAMPLES);

    la_arm(la_pio, la_sm, la_dma_chan, la_capture_buf, la_buf_size_words, PCS_B_PSRAM_CE, false);
#endif
#endif

#if 1
    puts("psram: quad read");
    psram_qread(0x000000UL, d, 2);

    printf("read %02x %02x\n", d[0], d[1]);

#if LOGIC_ANALYZER
    // The logic analyser should have started capturing as soon as it saw the
    // first transition. Wait until the last sample comes in from the DMA.
    dma_channel_wait_for_finish_blocking(la_dma_chan);

    la_print_capture_buf(la_capture_buf, CAPTURE_PIN_BASE, CAPTURE_PIN_COUNT, CAPTURE_N_SAMPLES);

    la_arm(la_pio, la_sm, la_dma_chan, la_capture_buf, la_buf_size_words, PCS_B_PSRAM_CE, false);
#endif
#endif

#if 1
    puts("psram: spi mode");
    psram_set_spi_mode();

#if LOGIC_ANALYZER
    // The logic analyser should have started capturing as soon as it saw the
    // first transition. Wait until the last sample comes in from the DMA.
    dma_channel_wait_for_finish_blocking(la_dma_chan);

    la_print_capture_buf(la_capture_buf, CAPTURE_PIN_BASE, CAPTURE_PIN_COUNT, CAPTURE_N_SAMPLES);
#endif
#endif

    printf("done\n");
    stdio_flush();
    sleep_ms(250);

    // Set up the gpclk generator
    clocks_hw->clk[clk_gpout0].ctrl = (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS << CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB) |
                                 CLOCKS_CLK_GPOUT0_CTRL_ENABLE_BITS;
    clocks_hw->clk[clk_gpout0].div = 100 << CLOCKS_CLK_GPOUT0_DIV_INT_LSB;
    // Set gpio pin to gpclock function
    gpio_set_function(21, GPIO_FUNC_GPCK);

    // wait for keypress and then reboot:
    getchar();
    reset_usb_boot(0, 0);

    return 0;
}
