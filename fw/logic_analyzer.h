
void la_init(PIO pio, uint sm, uint pin_base, uint pin_count, float div);
void la_arm(PIO pio, uint sm, uint dma_chan, uint32_t *capture_buf, size_t capture_size_words, uint trigger_pin, bool trigger_level);
void la_reset(PIO pio, uint sm, uint dma_chan, uint32_t *capture_buf, size_t capture_size_words);
void la_print_capture_buf(const uint32_t *buf, uint pin_base, uint pin_count, uint32_t n_samples);
void la_print_vertical(const uint32_t *buf, uint pin_base, uint pin_count, uint32_t n_samples);
uint la_calc_buf_size_words(uint samples, uint pins);
