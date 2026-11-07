#ifndef ALPHA_SIGN_H
#define ALPHA_SIGN_H

#include <stdint.h>

// Dimensions for the 12-board daisy chain
#define ALPHA_ROWS    14
#define ALPHA_COLS    480
#define ALPHA_PIXELS  (ALPHA_ROWS * ALPHA_COLS)
#define ALPHA_BYTES_PER_ROW ((ALPHA_COLS + 7) / 8) // 60 bytes per color per row

// The raw frame buffer (1,680 bytes)
// For each row (14 total), there are 60 bytes of RED data, followed by 60 bytes of GREEN data.
// A bit value of '1' means the LED is ON.
typedef struct {
  uint8_t red[ALPHA_ROWS][ALPHA_BYTES_PER_ROW];
  uint8_t grn[ALPHA_ROWS][ALPHA_BYTES_PER_ROW];
} alpha_frame_t;

#ifdef __cplusplus
extern "C" {
#endif

  // Starts the Core 1 multiplexing task
  void alpha_sign_init(void);

  // Updates the display buffer safely from another task
  void alpha_sign_update_frame(const alpha_frame_t* new_frame);

#ifdef __cplusplus
}
#endif

#endif // ALPHA_SIGN_H

