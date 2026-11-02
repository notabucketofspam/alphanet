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
  // 8051 counts row index from 13 down to 0
  for (int8_t row = 13; row >= 0; row--) {

    // 1. Shift 480 columns while ROW_RESET stays LOW (LEDs stay ON)
    for (uint16_t col = 0; col < 480; col++) {
      
      // Set Red and Green data bits simultaneously
      if (col & 8) {
        // Red ON (LOW), Green OFF (HIGH)
        PORTD = (PORTD & ~MASK_RED_DAT) | MASK_GRN_DAT;
      } else {
        // Red OFF (HIGH), Green ON (LOW)
        PORTD = (PORTD & ~MASK_GRN_DAT) | MASK_RED_DAT;
      }
      HOLD_500NS();

      // Pulse both Red and Green clocks LOW simultaneously (rising edge at 595s)
      PORTD &= ~MASK_BOTH_CLKS;
      HOLD_500NS();
      PORTD |= MASK_BOTH_CLKS;
      HOLD_500NS();
    }

    // Hold the lit row for 1 millisecond (since shifting 480 pixels now only takes ~0.8ms!)
    delayMicroseconds(1000);

    // 2. Normal Row Latch & Advance (Blank & shift '0' into 164)
    PORTD |= MASK_ROW_RESET;  // HIGH = Blank 595s & feed '0' to 164
    HOLD_500NS();
    pulseLatch();

    // 3. Frame Reset Double-Pulse (from 0x0390 in eprom.bin)
    if (row == 7) {
      // Pulse #1: Inject '1' token (ROW_RESET LOW)
      PORTD &= ~MASK_ROW_RESET;
      HOLD_500NS();
      pulseLatch();

      // Pulse #2: Shift '1' into QB, load '0' into QA (ROW_RESET HIGH)
      PORTD |= MASK_ROW_RESET;
      HOLD_500NS();
      pulseLatch();
    }

    // 4. Immediately Unblank Display (ROW_RESET LOW)
    PORTD &= ~MASK_ROW_RESET;
  }
}
