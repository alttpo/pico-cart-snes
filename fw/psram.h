#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/spi.h"

void __time_critical_func(pio_spi_write8_blocking)(PIO pio, uint sm, const uint8_t *src, size_t len);
void __time_critical_func(pio_spi_read8_blocking)(PIO pio, uint sm, uint8_t *dst, size_t len);
void __time_critical_func(pio_spi_write8_read8_blocking)(PIO pio, uint sm, uint8_t *src, uint8_t *dst, size_t len);

void __time_critical_func(psram_reset)(PIO pio, uint sm);
void __time_critical_func(psram_read_eid)(PIO pio, uint sm);
