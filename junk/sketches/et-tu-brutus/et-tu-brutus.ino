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

// Idle PORTD state during the display hold window:
#define PORTD_IDLE (MASK_LATCH | MASK_BOTH_DATS)

// Row hold time in microseconds (500 us + 300 us shift = ~815 us/row -> ~88 Hz frame rate)
#define ROW_HOLD_US 500.0

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

// Normal Row Latch: 18 us LATCH pulse inside a 25 us blanking window
static inline void latchNormalRow() {
  PORTD = PORTD_IDLE | MASK_ROW_RESET;                  // Blank & 164 A,B = '0'
  _delay_us(3.5);
  PORTD = (PORTD_IDLE | MASK_ROW_RESET) & ~MASK_LATCH;  // LATCH LOW (18 us)
  _delay_us(18.0);
  PORTD = PORTD_IDLE | MASK_ROW_RESET;                  // LATCH HIGH
  _delay_us(3.5);
  PORTD = PORTD_IDLE;                                   // Unblank (ROW_RESET LOW)
}

// Frame Reset Sequence (row == 7): Matches 0x027F-0x02A5 in eprom.bin
// Performs the normal '0' shift, injects the '1' token, and shifts it past unused QA
// using crisp 2.5 us pulses so the display never flashes on QA or starves C5.
static inline void latchFrameResetRow() {
  // 1. Blank & shift '0' (also pre-charges C5 with 14 us LOW pulse)
  PORTD = PORTD_IDLE | MASK_ROW_RESET;                  // Blank & 164 A,B = '0'
  _delay_us(2.5);
  PORTD = (PORTD_IDLE | MASK_ROW_RESET) & ~MASK_LATCH;  // Pulse #1 LOW
  _delay_us(14.0);
  PORTD = PORTD_IDLE | MASK_ROW_RESET;                  // Pulse #1 HIGH
  _delay_us(2.5);

  // 2. Inject '1' token into QA (ROW_RESET LOW briefly for 2.5 us clock edge)
  PORTD = PORTD_IDLE;                                   // 164 A,B = '1'
  _delay_us(1.5);
  PORTD = PORTD_IDLE & ~MASK_LATCH;                     // Pulse #2 LOW
  _delay_us(2.5);
  PORTD = PORTD_IDLE;                                   // Pulse #2 HIGH
  _delay_us(1.5);

  // 3. Immediately Blank & shift '1' from QA into QB, loading '0' into QA
  PORTD = PORTD_IDLE | MASK_ROW_RESET;                  // Blank & 164 A,B = '0'
  _delay_us(1.5);
  PORTD = (PORTD_IDLE | MASK_ROW_RESET) & ~MASK_LATCH;  // Pulse #3 LOW
  _delay_us(2.5);
  PORTD = PORTD_IDLE | MASK_ROW_RESET;                  // Pulse #3 HIGH
  _delay_us(1.5);

  // 4. Unblank display for the new row
  PORTD = PORTD_IDLE;
}

void setup() {
  cli();        // Disable all global interrupts (prevents Timer0 jitter completely)
  TIMSK0 = 0;
  DDRD |= 0xFC;
  PORTD = PORTD_IDLE;
}

void loop() {
  const uint8_t on_clk_lo  = MASK_LATCH;
  const uint8_t on_clk_hi  = MASK_LATCH | MASK_BOTH_CLKS;
  const uint8_t off_clk_lo = MASK_LATCH | MASK_BOTH_DATS;
  const uint8_t off_clk_hi = MASK_LATCH | MASK_BOTH_DATS | MASK_BOTH_CLKS;

  // Lock inside while(1) so Arduino main() never interrupts the scan loop
  while (1) {
    for (int8_t row = 13; row >= 0; row--) {

      // 1. Burst-shift 480 pixels in ~300 us (3 ON, 5 OFF)
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

      // 2. Display hold window
      PORTD = PORTD_IDLE;
      _delay_us(ROW_HOLD_US);

      // 3. Latch & advance row
      if (row == 7) {
        latchFrameResetRow();
      } else {
        latchNormalRow();
      }
    }
  }
}
