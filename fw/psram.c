#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
//#include "hardware/spi.h"
#include "hardware/dma.h"
#include "psram.pio.h"
#include "cart.h"
#include "psram.h"

PIO pio = pio0;
uint sm = 0;
const uint offset = 0;
int dma_chan = -1;
dma_channel_config dma_config = {0};

// reverses bit order of each nibble in each byte:
// 1000 -> 0001
// 1011 -> 1101
static inline uint32_t reverse_nibbles(uint32_t num) {
    num = (((num & 0xaaaaaaaaUL) >> 1) | ((num & 0x55555555UL) << 1));
    num = (((num & 0xccccccccUL) >> 2) | ((num & 0x33333333UL) << 2));
    return num;
}

static inline bool __time_critical_func(psram_is_completed)() {
    return pio_interrupt_get(pio, 0);
}

static inline void __time_critical_func(psram_clear_completion)() {
    pio_interrupt_clear(pio, 0);
}

static inline void __time_critical_func(psram_wait_for_completion)() {
    while (!psram_is_completed(pio)) tight_loop_contents();
    psram_clear_completion(pio);
}

static inline void __time_critical_func(psram_clr_ce)() {
    gpio_put(PCS_B_PSRAM_CE, false);
}

static inline void __time_critical_func(psram_set_ce)() {
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

static inline void __time_critical_func(psram_cmd0)(uint32_t cmd) {
    psram_clr_ce();
    // write 8 bits:
    pio_sm_put_blocking(pio, sm, psram_op(spi_offset_write, 8, true));
    pio_sm_put_blocking(pio, sm, cmd << 24);
    psram_wait_for_completion();
    psram_set_ce();
}

void __time_critical_func(psram_qcmd0)(uint32_t cmd) {
    psram_clr_ce();
    // write 2 nibbles:
    pio_sm_put_blocking(pio, sm, psram_op(spi_offset_write, 2, true));
    pio_sm_put_blocking(pio, sm, cmd << 24);
    psram_wait_for_completion();
    psram_set_ce();
}

void __time_critical_func(psram_reset)() {
    psram_cmd0(0x66UL); // reset enable
    psram_cmd0(0x99UL); // reset
}

void __time_critical_func(psram_read_eid)() {
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

void __time_critical_func(psram_write)() {
    psram_clr_ce();
    // write 32 bits, cmd 0x02, address 0x00_00_00:
    pio_sm_put_blocking(pio, sm, psram_op(spi_offset_write, 32, false));
    pio_sm_put_blocking(pio, sm, 0x02000000UL);
    // write 8 bits:
    pio_sm_put_blocking(pio, sm, psram_op(spi_offset_write, 16, true));
    pio_sm_put_blocking(pio, sm, 0x55AA0000UL);
    psram_wait_for_completion();
    psram_set_ce();
}

void __time_critical_func(psram_read)(uint32_t addr, uint32_t* d, uint size) {
    dma_channel_configure(
        dma_chan,
        &dma_config,
        d,              // Destination pointer
        &pio->rxf[sm],  // Source pointer
        size,           // Number of transfers in words (32-bit)
        true            // Start immediately
    );

    psram_clr_ce();
    // write 32 bits, cmd 0x0B, address 0x00_00_00
    pio_sm_put_blocking(pio, sm, psram_op(spi_offset_write, 32, false));
    pio_sm_put_blocking(pio, sm, 0x0B000000UL | (addr & 0x00FFFFFFUL));
    // wait 8 clocks:
    pio_sm_put_blocking(pio, sm, psram_op(spi_offset_write,  8, false));
    pio_sm_put_blocking(pio, sm, 0x00000000UL);
    // read 16 bits:
    pio_sm_put_blocking(pio, sm, psram_op(spi_offset_read,  16,  true));
    //printf("wait_for_completion\n");
    psram_wait_for_completion();
    psram_set_ce();
}

void __time_critical_func(psram_set_qpi_mode)() {
    psram_cmd0(0x35UL); // enter quad mode

    // disable SPI state machine:
    pio_sm_set_enabled(pio, sm, false);
    // unload SPI program:
    pio_remove_program(pio, &spi_program, offset);

    // load PIO program for QSPI:
    pio_add_program_at_offset(pio, &qspi_program, offset);
    pio_qspi_init(pio, sm, offset,
        1.0f,
        false,
        PCS_B_CLK_133MHZ,
        PCS_B_PSRAM_SIO0
    );
}

void __time_critical_func(psram_set_spi_mode)() {
    psram_qcmd0(0xF5UL); // exit quad mode

    // disable QSPI state machine:    
    pio_sm_set_enabled(pio, sm, false);
    // unload QSPI program:
    pio_remove_program(pio, &qspi_program, offset);

    // load PIO program for SPI:
    pio_add_program_at_offset(pio, &spi_program, offset);
    pio_spi_init(pio, sm, offset,
        1.0f,
        false,
        PCS_B_CLK_133MHZ,
        PCS_B_PSRAM_SIO0,
        PCS_B_PSRAM_SIO1
    );
}

void __time_critical_func(psram_qread)(uint32_t addr, uint32_t* d, uint size) {
    dma_channel_configure(
        dma_chan,
        &dma_config,
        d,              // Destination pointer
        &pio->rxf[sm],  // Source pointer
        size,           // Number of transfers in words (32-bit)
        true            // Start immediately
    );

    psram_clr_ce();
    // write 32 bits, cmd 0xEB, address
    pio_sm_put_blocking(pio, sm, psram_op(qspi_offset_write, 8, false));
    pio_sm_put(pio, sm, (0xEB000000UL | (addr & 0x00FFFFFFUL)));
    // wait 6 clocks:
    pio_sm_put_blocking(pio, sm, psram_op(qspi_offset_write, 6, false));
    pio_sm_put(pio, sm, (0x00000000UL));
    // read `size*2` nibbles from PSRAM, transfer to DMA in words:
    pio_sm_put_blocking(pio, sm, psram_op(qspi_offset_read, size*2,  true));
    //printf("wait_for_completion\n");
    psram_wait_for_completion();
    psram_set_ce();
}

void psram_init() {
    // load PIO program for SPI:
    pio_add_program_at_offset(pio, &spi_program, offset);
    sm = pio_claim_unused_sm(pio, true);
    pio_spi_init(pio, sm, offset,
        1.0f,
        false,
        PCS_B_CLK_133MHZ,
        PCS_B_PSRAM_SIO0,
        PCS_B_PSRAM_SIO1
    );

    // set up a DMA channel to read from RX FIFO:
    dma_chan = dma_claim_unused_channel(true);
    dma_config = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&dma_config, false);
    channel_config_set_write_increment(&dma_config, true);
    channel_config_set_dreq(&dma_config, pio_get_dreq(pio, sm, false));

    // reset psram chip:
    psram_reset();
}
