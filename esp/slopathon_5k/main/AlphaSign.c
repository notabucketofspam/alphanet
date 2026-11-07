#include "AlphaSign.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "soc/gpio_reg.h"
#include "esp_rom_sys.h"
#include "esp_task_wdt.h"
#include "esp_attr.h"

// Pin Definitions
#define PIN_RED_CLK   14
#define PIN_ROW_RESET 27
#define PIN_GRN_DAT   26
#define PIN_LATCH     25
#define PIN_RED_DAT   33
#define PIN_GRN_CLK   32

#define B0_RED_CLK    (1UL << PIN_RED_CLK)
#define B0_ROW_RESET  (1UL << PIN_ROW_RESET)
#define B0_GRN_DAT    (1UL << PIN_GRN_DAT)
#define B0_LATCH      (1UL << PIN_LATCH)
#define B1_RED_DAT    (1UL << (PIN_RED_DAT - 32))
#define B1_GRN_CLK    (1UL << (PIN_GRN_CLK - 32))

#define B0_SET(mask)  REG_WRITE(GPIO_OUT_W1TS_REG, (mask))
#define B0_CLR(mask)  REG_WRITE(GPIO_OUT_W1TC_REG, (mask))
#define B1_SET(mask)  REG_WRITE(GPIO_OUT1_W1TS_REG, (mask))
#define B1_CLR(mask)  REG_WRITE(GPIO_OUT1_W1TC_REG, (mask))

#define ROW_HOLD_US   500

#define HOLD_300NS() __asm__ __volatile__( \
    "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" \
    "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" \
    "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" \
    "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" \
    "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" \
    "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" \
    ::: "memory" \
)

// The global frame buffer
static alpha_frame_t active_frame = { 0 };

void alpha_sign_update_frame(const alpha_frame_t* new_frame) {
  if (new_frame) {
    // Because the scanner runs infinitely on Core 1, updating this 
    // pointer/memory from Core 0 might cause tearing if a frame writes 
    // mid-scan. For strict perfection, you would use double-buffering here.
    // For UDP streaming, a raw memcpy is usually fast enough to avoid severe tearing.
    memcpy(&active_frame, new_frame, sizeof(alpha_frame_t));
  }
}

static inline void IRAM_ATTR setPinsIdle(void) {
  B0_SET(B0_LATCH | B0_GRN_DAT);
  B0_CLR(B0_RED_CLK | B0_ROW_RESET);
  B1_SET(B1_RED_DAT);
  B1_CLR(B1_GRN_CLK);
}

// 1. Data LOW (ON), Clocks HIGH
// 2. Clocks LOW (Rising edge at 595s)
#define SHIFT_BIT_ON(is_red, is_grn) do { \
    if (is_grn) B0_CLR(B0_GRN_DAT); else B0_SET(B0_GRN_DAT); \
    if (is_red) B1_CLR(B1_RED_DAT); else B1_SET(B1_RED_DAT); \
    B0_SET(B0_RED_CLK); \
    B1_SET(B1_GRN_CLK); \
    HOLD_300NS(); \
    B0_CLR(B0_RED_CLK); \
    B1_CLR(B1_GRN_CLK); \
    HOLD_300NS(); \
} while (0)


static inline void IRAM_ATTR latchNormalRow(void) {
  B0_SET(B0_LATCH | B0_GRN_DAT | B0_ROW_RESET);
  B0_CLR(B0_RED_CLK);
  B1_SET(B1_RED_DAT);
  B1_CLR(B1_GRN_CLK);
  esp_rom_delay_us(4);

  B0_CLR(B0_LATCH);
  esp_rom_delay_us(18);

  B0_SET(B0_LATCH);
  esp_rom_delay_us(4);
  B0_CLR(B0_ROW_RESET);
}

static inline void IRAM_ATTR latchFrameResetRow(void) {
  B0_SET(B0_LATCH | B0_GRN_DAT | B0_ROW_RESET);
  B0_CLR(B0_RED_CLK);
  B1_SET(B1_RED_DAT);
  B1_CLR(B1_GRN_CLK);
  esp_rom_delay_us(3);

  B0_CLR(B0_LATCH);
  esp_rom_delay_us(14);
  B0_SET(B0_LATCH);
  esp_rom_delay_us(3);

  B0_CLR(B0_ROW_RESET);
  esp_rom_delay_us(2);
  B0_CLR(B0_LATCH);
  esp_rom_delay_us(3);
  B0_SET(B0_LATCH);
  esp_rom_delay_us(2);

  B0_SET(B0_ROW_RESET);
  esp_rom_delay_us(2);
  B0_CLR(B0_LATCH);
  esp_rom_delay_us(3);
  B0_SET(B0_LATCH);
  esp_rom_delay_us(2);

  B0_CLR(B0_ROW_RESET);
}

static void IRAM_ATTR alpha_sign_task(void* pvParameters) {
  setPinsIdle();

  while (1) {
    for (int8_t row = 13; row >= 0; row--) {

      // Shift 480 pixels using the active frame buffer
      // We iterate through 60 bytes per row (60 * 8 = 480 bits)
      for (uint8_t byte_idx = 0; byte_idx < 60; byte_idx++) {
        uint8_t red_byte = active_frame.red[row][byte_idx];
        uint8_t grn_byte = active_frame.grn[row][byte_idx];

        // Shift out MSB first
        for (int8_t bit = 7; bit >= 0; bit--) {
          bool is_red = (red_byte & (1 << bit)) != 0;
          bool is_grn = (grn_byte & (1 << bit)) != 0;
          SHIFT_BIT_ON(is_red, is_grn);
        }
      }

      setPinsIdle();
      esp_rom_delay_us(ROW_HOLD_US);

      if (row == 7) {
        latchFrameResetRow();
      } else {
        latchNormalRow();
      }
    }
  }
}

void alpha_sign_init(void) {
  const gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << PIN_RED_CLK) |
                      (1ULL << PIN_ROW_RESET) |
                      (1ULL << PIN_GRN_DAT) |
                      (1ULL << PIN_LATCH) |
                      (1ULL << PIN_RED_DAT) |
                      (1ULL << PIN_GRN_CLK),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&io_conf);
  setPinsIdle();

  TaskHandle_t idle_core1 = xTaskGetIdleTaskHandleForCore(1);
  if (idle_core1 != NULL) {
    esp_task_wdt_delete(idle_core1);
  }

  xTaskCreatePinnedToCore(
    alpha_sign_task,
    "alpha_sign",
    4096,
    NULL,
    configMAX_PRIORITIES - 1,
    NULL,
    1 // Core 1
  );
}

