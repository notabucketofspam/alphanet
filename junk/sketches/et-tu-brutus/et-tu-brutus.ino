#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// PORTD Bitmasks (Arduino Uno Pins 2..7)
#define MASK_RED_CLK   (1 << PD2) // 0x04 - Ribbon Pin 1 (Idles LOW)
#define MASK_ROW_RESET (1 << PD3) // 0x08 - Ribbon Pin 2 (Idles LOW: LEDs ON & '1' token)
#define MASK_GRN_DAT   (1 << PD4) // 0x10 - Ribbon Pin 3 (Idles HIGH: LEDs OFF)
#define MASK_LATCH     (1 << PD5) // 0x20 - Ribbon Pin 4 (Idles HIGH)
#define MASK_RED_DAT   (1 << PD6) // 0x40 - Ribbon Pin 5 (Idles HIGH: LEDs OFF)
#define MASK_GRN_CLK   (1 << PD7) // 0x80 - Ribbon Pin 6 (Idles LOW)

#define MASK_BOTH_CLKS (MASK_RED_CLK | MASK_GRN_CLK) // 0x84
#define MASK_BOTH_DATS (MASK_RED_DAT | MASK_GRN_DAT) // 0x50

// Idle PORTD state during the 1,179 us display hold window:
// LATCH = HIGH, RED_DAT = HIGH, GRN_DAT = HIGH | Clocks = LOW, ROW_RESET = LOW
#define PORTD_IDLE (MASK_LATCH | MASK_BOTH_DATS)

// 1 Pixel Shift: 5 cycles (312.5 ns) Clocks LOW, 5 cycles (312.5 ns) Clocks HIGH
#define SHIFT_BIT(lo, hi) __asm__ __volatile__( \
  "out %[port], %[v_lo]\n\t" \
  "nop\n\t" \
  "nop\n\t" \
  "nop\n\t" \
  "nop\n\t" \
  "out %[port], %[v_hi]\n\t" \
  "nop\n\t" \
  "nop\n\t" \
  "nop\n\t" \
  "nop\n\t" \
  : \
  : [port] "I" (_SFR_IO_ADDR(PORTD)), [v_lo] "r" (lo), [v_hi] "r" (hi) \
  : "memory" \
)

// Latch pulse: 18 us LOW enclosed inside a 25 us ROW_RESET window
static inline void latchWithZeroToken() {
  // 1. Blank display & set 164 A,B = '0' (ROW_RESET HIGH)
  PORTD = PORTD_IDLE | MASK_ROW_RESET;
  _delay_us(3.5);

  // 2. Pulse LATCH LOW for 18 us (drives Q16/C5/Q15 watchdog & clocks 595/164)
  PORTD = (PORTD_IDLE | MASK_ROW_RESET) & ~MASK_LATCH;
  _delay_us(18.0);

  // 3. Return LATCH HIGH, hold ROW_RESET HIGH for 3.5 us (Total blank = 25 us)
  PORTD = PORTD_IDLE | MASK_ROW_RESET;
  _delay_us(3.5);

  // 4. Unblank display (ROW_RESET LOW)
  PORTD = PORTD_IDLE;
}

static inline void latchWithOneToken() {
  // 1. Keep ROW_RESET LOW (feeds '1' token to 164 A,B)
  PORTD = PORTD_IDLE;
  _delay_us(3.5);

  // 2. Pulse LATCH LOW for 18 us while ROW_RESET is LOW
  PORTD = PORTD_IDLE & ~MASK_LATCH;
  _delay_us(18.0);

  // 3. Return LATCH HIGH
  PORTD = PORTD_IDLE;
  _delay_us(3.5);
}

void setup() {
  // Disable Timer0 overflow interrupt so millis() never causes pulse jitter
  TIMSK0 = 0;

  // Configure PD2..PD7 as outputs
  DDRD |= 0xFC;
  PORTD = PORTD_IDLE;
}

void loop() {
  // Precompute PORTD register values for ON (Orange: Red+Grn LOW) and OFF (Red+Grn HIGH)
  const uint8_t on_clk_lo  = MASK_LATCH;                                  // Data LOW (ON), Clocks LOW
  const uint8_t on_clk_hi  = MASK_LATCH | MASK_BOTH_CLKS;                 // Data LOW (ON), Clocks HIGH
  const uint8_t off_clk_lo = MASK_LATCH | MASK_BOTH_DATS;                 // Data HIGH (OFF), Clocks LOW
  const uint8_t off_clk_hi = MASK_LATCH | MASK_BOTH_DATS | MASK_BOTH_CLKS;// Data HIGH (OFF), Clocks HIGH

  // 14-row multiplexing scan (1,504 us per row = 665 Hz row rate / 47.5 Hz frame rate)
  for (int8_t row = 13; row >= 0; row--) {

    // 1. Burst-shift 480 pixels in ~300 us (60 groups of 8 pixels: 3 ON, 5 OFF)
    // 3/8 ON pixels during the 300 us burst gives the exact ~4.62V DC average on Pin 3 & Pin 5
    for (uint8_t grp = 0; grp < 60; grp++) {
      SHIFT_BIT(on_clk_lo,  on_clk_hi);
      SHIFT_BIT(on_clk_lo,  on_clk_hi);
      SHIFT_BIT(on_clk_lo,  on_clk_hi);
      SHIFT_BIT(off_clk_lo, off_clk_hi);
      SHIFT_BIT(off_clk_lo, off_clk_hi);
      SHIFT_BIT(off_clk_lo, off_clk_hi);
      SHIFT_BIT(off_clk_lo, off_clk_hi);
      SHIFT_BIT(off_clk_lo, off_clk_hi);
    }

    // 2. Return Clocks LOW and Data HIGH for the 1,179 us display hold window
    PORTD = PORTD_IDLE;
    _delay_us(1179.0);

    // 3. Normal Row Latch & Advance (25 us blanking, 18 us latch)
    latchWithZeroToken();

    // 4. Frame Reset Double-Pulse on wrap-around row (matches 0x0289 in eprom.bin)
    if (row == 7) {
      latchWithOneToken();  // Pulse #1: Inject '1' token into first 164 QA
      latchWithZeroToken(); // Pulse #2: Shift '1' into QB, load '0' into QA
    }
  }
}
