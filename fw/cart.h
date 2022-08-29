// PCS prefix is for pico-cart-snes
// PCS_B prefix is for bit numbers of named pins:
#define PCS_B_D0               0
#define PCS_B_D1               1
#define PCS_B_D2               2
#define PCS_B_D3               3
#define PCS_B_D4               4
#define PCS_B_D5               5
#define PCS_B_D6               6
#define PCS_B_D7               7
#define PCS_B_ADDR1QH          8
#define PCS_B_ADDR2QH          9
#define PCS_B_ADDR3QH         10
#define PCS_B_ADDR4QH         11
#define PCS_B_PSRAM_SIO0      12
#define PCS_B_PSRAM_SIO1      13
#define PCS_B_PSRAM_SIO2      14
#define PCS_B_PSRAM_SIO3      15
#define PCS_B_PSRAM_CE        16
#define PCS_B_CLK_133MHZ      17
#define PCS_B_ADDR_CE         18
#define PCS_B_ADDR_LD         19
#define PCS_B_DDIR            20
#define PCS_B_WRAM_OR_CLK     21
#define PCS_B_RESET           22
#define PCS_B_RD              26
#define PCS_B_WR              27
#define PCS_B_CE              28

// PCS_MS prefix are mask SET values of the above bit numbers:
#define PCS_MS_D0             (1UL << PCS_B_D0)
#define PCS_MS_D1             (1UL << PCS_B_D1)
#define PCS_MS_D2             (1UL << PCS_B_D2)
#define PCS_MS_D3             (1UL << PCS_B_D3)
#define PCS_MS_D4             (1UL << PCS_B_D4)
#define PCS_MS_D5             (1UL << PCS_B_D5)
#define PCS_MS_D6             (1UL << PCS_B_D6)
#define PCS_MS_D7             (1UL << PCS_B_D7)
#define PCS_MS_ADDR1QH        (1UL << PCS_B_ADDR1QH)
#define PCS_MS_ADDR2QH        (1UL << PCS_B_ADDR2QH)
#define PCS_MS_ADDR3QH        (1UL << PCS_B_ADDR3QH)
#define PCS_MS_ADDR4QH        (1UL << PCS_B_ADDR4QH)
#define PCS_MS_PSRAM_SIO0     (1UL << PCS_B_PSRAM_SIO0)
#define PCS_MS_PSRAM_SIO1     (1UL << PCS_B_PSRAM_SIO1)
#define PCS_MS_PSRAM_SIO2     (1UL << PCS_B_PSRAM_SIO2)
#define PCS_MS_PSRAM_SIO3     (1UL << PCS_B_PSRAM_SIO3)
#define PCS_MS_PSRAM_CE       (1UL << PCS_B_PSRAM_CE)
#define PCS_MS_CLK_133MHZ     (1UL << PCS_B_CLK_133MHZ)
#define PCS_MS_ADDR_CE        (1UL << PCS_B_ADDR_CE)
#define PCS_MS_ADDR_LD        (1UL << PCS_B_ADDR_LD)
#define PCS_MS_DDIR           (1UL << PCS_B_DDIR)
#define PCS_MS_WRAM_OR_CLK    (1UL << PCS_B_WRAM_OR_CLK)
#define PCS_MS_RESET          (1UL << PCS_B_RESET)
#define PCS_MS_RD             (1UL << PCS_B_RD)
#define PCS_MS_WR             (1UL << PCS_B_WR)
#define PCS_MS_CE             (1UL << PCS_B_CE)

// PCS_MC prefix are mask CLEAR values of the above bit numbers:
#define PCS_MC_D0             (0UL << PCS_B_D0)
#define PCS_MC_D1             (0UL << PCS_B_D1)
#define PCS_MC_D2             (0UL << PCS_B_D2)
#define PCS_MC_D3             (0UL << PCS_B_D3)
#define PCS_MC_D4             (0UL << PCS_B_D4)
#define PCS_MC_D5             (0UL << PCS_B_D5)
#define PCS_MC_D6             (0UL << PCS_B_D6)
#define PCS_MC_D7             (0UL << PCS_B_D7)
#define PCS_MC_ADDR1QH        (0UL << PCS_B_ADDR1QH)
#define PCS_MC_ADDR2QH        (0UL << PCS_B_ADDR2QH)
#define PCS_MC_ADDR3QH        (0UL << PCS_B_ADDR3QH)
#define PCS_MC_ADDR4QH        (0UL << PCS_B_ADDR4QH)
#define PCS_MC_PSRAM_SIO0     (0UL << PCS_B_PSRAM_SIO0)
#define PCS_MC_PSRAM_SIO1     (0UL << PCS_B_PSRAM_SIO1)
#define PCS_MC_PSRAM_SIO2     (0UL << PCS_B_PSRAM_SIO2)
#define PCS_MC_PSRAM_SIO3     (0UL << PCS_B_PSRAM_SIO3)
#define PCS_MC_PSRAM_CE       (0UL << PCS_B_PSRAM_CE)
#define PCS_MC_CLK_133MHZ     (0UL << PCS_B_CLK_133MHZ)
#define PCS_MC_ADDR_CE        (0UL << PCS_B_ADDR_CE)
#define PCS_MC_ADDR_LD        (0UL << PCS_B_ADDR_LD)
#define PCS_MC_DDIR           (0UL << PCS_B_DDIR)
#define PCS_MC_WRAM_OR_CLK    (0UL << PCS_B_WRAM_OR_CLK)
#define PCS_MC_RESET          (0UL << PCS_B_RESET)
#define PCS_MC_RD             (0UL << PCS_B_RD)
#define PCS_MC_WR             (0UL << PCS_B_WR)
#define PCS_MC_CE             (0UL << PCS_B_CE)
