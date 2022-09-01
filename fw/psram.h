#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/spi.h"

void __time_critical_func(psram_reset)();
void __time_critical_func(psram_read_eid)();
void __time_critical_func(psram_write)(uint32_t addr, uint32_t* words, uint size_bytes);
void __time_critical_func(psram_read)(uint32_t addr, uint32_t* d, uint size);
void __time_critical_func(psram_qread)(uint32_t addr, uint32_t* d, uint size);

void __time_critical_func(psram_set_qpi_mode)();
void __time_critical_func(psram_set_spi_mode)();
void psram_init();
