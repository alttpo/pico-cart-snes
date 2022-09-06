#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"

#include "logic_analyzer.h"
#include "cart.h"
#include "psram.pio.h"

PIO     psram_pio = pio0;
bool    psram_qpi_mode = false;
uint    psram_spi_sm = 0;
uint    psram_spi_offset = 0;

uint    psram_qpiw_sm = 0;
uint    psram_qpiw_offset = 0;
uint    psram_qpir_sm = 1;
uint    psram_qpir_offset = 0;

void __time_critical_func(psram_init)(void) {
    pio_sm_claim(psram_pio, psram_spi_sm);
    psram_spi_offset = pio_add_program(psram_pio, &psram_spi_write_program);
    pio_sm_config c = psram_spi_write_program_get_default_config(psram_spi_offset);

    sm_config_set_out_pins(&c, PCS_B_PSRAM_SIO0, 1);
    sm_config_set_set_pins(&c, PCS_B_PSRAM_SIO0, 1);
    sm_config_set_sideset_pins(&c, PCS_B_PSRAM_CE);
    sm_config_set_out_shift(&c, false, true, 8);
    sm_config_set_clkdiv(&c, 1.f);

    pio_gpio_init(psram_pio, PCS_B_PSRAM_SIO0);
    pio_gpio_init(psram_pio, PCS_B_PSRAM_SIO1);
    pio_gpio_init(psram_pio, PCS_B_PSRAM_SIO2);
    pio_gpio_init(psram_pio, PCS_B_PSRAM_SIO3);
    pio_gpio_init(psram_pio, PCS_B_PSRAM_CE);
    pio_gpio_init(psram_pio, PCS_B_CLK_133MHZ);

    pio_sm_init(psram_pio, psram_spi_sm, psram_spi_offset, &c);

    pio_sm_set_pindirs_with_mask(
        psram_pio,
        psram_spi_sm,
        // dirs: (all outputs)
        PCS_MS_PSRAM_SIO0 | PCS_MS_PSRAM_CE | PCS_MS_CLK_133MHZ,
        // mask:
        PCS_MS_PSRAM_SIO0 | PCS_MS_PSRAM_CE | PCS_MS_CLK_133MHZ
    );
    pio_sm_set_pins_with_mask(
        psram_pio,
        psram_spi_sm,
        // values:
        PCS_MS_PSRAM_CE,
        // mask:
        PCS_MS_PSRAM_CE
    );

    pio_sm_set_enabled(psram_pio, psram_spi_sm, true);
}

static inline void __time_critical_func(psram_spi_command0)(uint32_t command) {
    pio_sm_put_blocking(psram_pio, psram_spi_sm, 7);
    pio_sm_put(psram_pio, psram_spi_sm, command << 24);
}

void __time_critical_func(psram_reset)(void) {
    psram_spi_command0(0x66U);
    psram_spi_command0(0x99U);
}

void __time_critical_func(psram_set_qpi_mode)() {
    psram_spi_command0(0x35UL); // enter quad mode
    while (pio_sm_get_pc(psram_pio, psram_spi_sm) != psram_spi_offset)
        tight_loop_contents();

    // disable SPI state machine:
    pio_sm_set_enabled(psram_pio, psram_spi_sm, false);
    // unload SPI program:
    pio_remove_program(psram_pio, &psram_spi_write_program, psram_spi_offset);
    pio_sm_unclaim(psram_pio, psram_spi_sm);

    // load both QPI programs:
    pio_sm_claim(psram_pio, psram_qpiw_sm);
    //pio_sm_claim(psram_pio, psram_qpir_sm);
    psram_qpiw_offset = pio_add_program(psram_pio, &psram_qpiw_program);
    //psram_qpir_offset = pio_add_program(psram_pio, &psram_qpir_program);

    pio_sm_config cw = psram_qpiw_program_get_default_config(psram_qpiw_offset);
    //pio_sm_config cr = psram_qpir_program_get_default_config(psram_qpir_offset);

    sm_config_set_out_pins(&cw, PCS_B_PSRAM_SIO0, 4);
    sm_config_set_set_pins(&cw, PCS_B_PSRAM_SIO0, 4);
    sm_config_set_sideset_pins(&cw, PCS_B_PSRAM_CE);
    sm_config_set_out_shift(&cw, false, false, 32);
    sm_config_set_clkdiv(&cw, 1.f);

    //sm_config_set_out_pins(&cr, PCS_B_PSRAM_SIO0, 4);
    //sm_config_set_set_pins(&cr, PCS_B_PSRAM_SIO0, 4);
    //sm_config_set_in_pins(&cr, PCS_B_PSRAM_SIO0);
    //sm_config_set_sideset_pins(&cr, PCS_B_PSRAM_CE);
    //sm_config_set_out_shift(&cr, false, false, 32);
    //sm_config_set_in_shift(&cr, false, true, 8);
    //sm_config_set_clkdiv(&cr, 1.f);

    pio_gpio_init(psram_pio, PCS_B_PSRAM_SIO0);
    pio_gpio_init(psram_pio, PCS_B_PSRAM_SIO1);
    pio_gpio_init(psram_pio, PCS_B_PSRAM_SIO2);
    pio_gpio_init(psram_pio, PCS_B_PSRAM_SIO3);
    pio_gpio_init(psram_pio, PCS_B_PSRAM_CE);
    pio_gpio_init(psram_pio, PCS_B_CLK_133MHZ);

    pio_sm_init(psram_pio, psram_qpiw_sm, psram_qpiw_offset, &cw);
    //pio_sm_init(psram_pio, psram_qpir_sm, psram_qpir_offset, &cr);

    // this only needs to be done once and can use either state machine:
    pio_sm_set_pindirs_with_mask(
        psram_pio,
        psram_qpiw_sm,
        // dirs: (all outputs)
        PCS_MS_PSRAM_SIO0 | PCS_MS_PSRAM_SIO1 | PCS_MS_PSRAM_SIO2 | PCS_MS_PSRAM_SIO3 | PCS_MS_PSRAM_CE | PCS_MS_CLK_133MHZ,
        // mask:
        PCS_MS_PSRAM_SIO0 | PCS_MS_PSRAM_SIO1 | PCS_MS_PSRAM_SIO2 | PCS_MS_PSRAM_SIO3 | PCS_MS_PSRAM_CE | PCS_MS_CLK_133MHZ
    );
    pio_sm_set_pins_with_mask(
        psram_pio,
        psram_qpiw_sm,
        // values:
        PCS_MS_PSRAM_CE,
        // mask:
        PCS_MS_PSRAM_CE
    );

    pio_sm_set_enabled(psram_pio, psram_qpiw_sm, true);
    //pio_sm_set_enabled(psram_pio, psram_qpir_sm, true);

    psram_qpi_mode = true;
}

void __time_critical_func(psram_qpi_write)(uint32_t address, uint32_t data) {
    pio_sm_put_blocking(psram_pio, psram_qpiw_sm, (address << 8) | (data & 0xFF));
}

PIO  la_pio = pio1;
uint la_sm = 3;

#define SAMPLE_COUNT 1024

int main(void) {
    // set sys clock to 288MHz:
    set_sys_clock_khz(288000, true);

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

    stdio_usb_init();

    while(!stdio_usb_connected()) {
        sleep_ms(10);
    }

    uint      la_buf_size_words = la_calc_buf_size_words(SAMPLE_COUNT, 6);
    uint32_t *la_capture_buf = malloc(la_buf_size_words * sizeof(uint32_t));
    hard_assert(la_capture_buf);

    pio_sm_claim(la_pio, la_sm);
    uint la_dma_chan = dma_claim_unused_channel(true);

    psram_init();

    la_init(la_pio, la_sm, PCS_B_PSRAM_SIO0, 6, 1.f);
    la_arm(la_pio, la_sm, la_dma_chan, la_capture_buf, la_buf_size_words, PCS_B_PSRAM_CE, false);

    // reset the PSRAM chip:
    psram_reset();

    // enter QPI mode:
    psram_set_qpi_mode();

    dma_channel_wait_for_finish_blocking(la_dma_chan);
    la_print_capture_buf(la_capture_buf, PCS_B_PSRAM_SIO0, 6, SAMPLE_COUNT);

    la_arm(la_pio, la_sm, la_dma_chan, la_capture_buf, la_buf_size_words, PCS_B_PSRAM_CE, false);

    psram_qpi_write(0x000000U, 0xAA);

    dma_channel_wait_for_finish_blocking(la_dma_chan);
    la_print_capture_buf(la_capture_buf, PCS_B_PSRAM_SIO0, 6, SAMPLE_COUNT);

    stdio_flush();
    sleep_ms(500);

    return 0;
}
