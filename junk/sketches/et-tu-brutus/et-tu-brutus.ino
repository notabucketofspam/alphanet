#include <avr/io.h>

// Bitmasks for PORTD (Pins 2 to 7 -> 0xFC)
#define MASK_RED_CLK   (1 << PD2) // 0x04 - Ribbon Pin 1
#define MASK_ROW_RESET (1 << PD3) // 0x08 - Ribbon Pin 2
#define MASK_GRN_DAT   (1 << PD4) // 0x10 - Ribbon Pin 3
#define MASK_LATCH     (1 << PD5) // 0x20 - Ribbon Pin 4
#define MASK_RED_DAT   (1 << PD6) // 0x40 - Ribbon Pin 5
#define MASK_GRN_CLK   (1 << PD7) // 0x80 - Ribbon Pin 6

#define MASK_BOTH_CLKS (MASK_RED_CLK | MASK_GRN_CLK) // 0x84
#define MASK_BOTH_DATS (MASK_RED_DAT | MASK_GRN_DAT) // 0x50

// 8 CPU cycles at 16 MHz = 0.5 microseconds (500 ns) clean settling time
#define HOLD_500NS() __asm__ __volatile__( \
  "nop\n\t" \
  "nop\n\t" \
  "nop\n\t" \
  "nop\n\t" \
  "nop\n\t" \
  "nop\n\t" \
  "nop\n\t" \
  "nop\n\t" \
  ::: "memory" \
)

static inline void pulseLatch() {
  // Drop LATCH low (creates rising edge after 74HC14 inverter)
  PORTD &= ~MASK_LATCH;
  HOLD_500NS();
  PORTD |= MASK_LATCH;
  HOLD_500NS();
}

void setup() {
  // Set PD2..PD7 as outputs (0xFC), leaving PD0/PD1 (RX/TX) untouched
  DDRD |= 0xFC;

  // Idle state: Clocks HIGH, Latch HIGH, ROW_RESET LOW (Display Unblanked)
  PORTD = (PORTD & 0x03) | MASK_BOTH_CLKS | MASK_LATCH;
}

void loop() {
  // ROW_RESET = LOW (0), RED_DAT & GRN_DAT = HIGH (1)
  PORTD = (PORTD & ~(MASK_ROW_RESET | MASK_BOTH_CLKS | MASK_LATCH)) | MASK_BOTH_DATS;
  HOLD_500NS();
  // Clocks & Latch HIGH simultaneously
  PORTD |= (MASK_BOTH_CLKS | MASK_LATCH);
  HOLD_500NS();
}
