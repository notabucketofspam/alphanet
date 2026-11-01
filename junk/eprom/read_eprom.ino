// thanks gemini

// AT27C010 EPROM Reader for Arduino Mega
// Capacity: 1-Megabit = 131,072 Bytes (Addresses 0x00000 to 0x1FFFF)

// Pin mapping definitions matching your configuration
const int START_ADDRESS_PIN = 30; // A0 -> D30, A1 -> D31, ..., A16 -> D46
const int TOTAL_ADDRESS_LINES = 17;
const long TOTAL_BYTES = 131072;  // 128 KB

void setup() {
  // Initialize USB Serial Communication at a high speed to prevent bottlenecking
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial monitor/terminal connection
  }

  // Configure Data Pins (PORTA / Digital 22-29) as Inputs
  DDRA = 0x00; 

  // Configure Address Pins (Digital 30 to 46) as Outputs
  for (int i = 0; i < TOTAL_ADDRESS_LINES; i++) {
    pinMode(START_ADDRESS_PIN + i, OUTPUT);
  }

  Serial.println("--- START OF AT27C010 DUMP ---");
  Serial.println("Click clear or start recording text log now.");
  delay(3000); // 3-second grace period to configure your terminal logger

  long runningChecksum = 0;

  // Loop through every single byte address location inside the EPROM
  for (long address = 0; address < TOTAL_BYTES; address++) {
    
    // Set address lines by extracting individual bits from our counter
    for (int bitPosition = 0; bitPosition < TOTAL_ADDRESS_LINES; bitPosition++) {
      int bitValue = (address >> bitPosition) & 0x01;
      digitalWrite(START_ADDRESS_PIN + bitPosition, bitValue);
    }

    // Give the chip silicon matrix a brief moment to stabilize data onto the pins
    delayMicroseconds(2); 

    // Directly read the physical hardware state of PORTA (Pins 22-29) in 1 clock cycle
    byte dataByte = PINA;

    // Accumulate the byte values to calculate a 16-bit matching checksum
    runningChecksum += dataByte;

    // Print out the byte as a formatted 2-character Hexadecimal string
    if (dataByte < 0x10) Serial.print("0");
    Serial.print(dataByte, HEX);

    // Format output with a space between bytes, and a new line every 16 bytes for human readability
    if ((address + 1) % 16 == 0) {
      Serial.println();
    } else {
      Serial.print(" ");
    }
  }

  // Keep the lower 16 bits of our running checksum calculation
  runningChecksum &= 0xFFFF; 

  Serial.println("--- END OF AT27C010 DUMP ---");
  Serial.print("Arduino Calculated Checksum (Hex): ");
  Serial.println(runningChecksum, HEX);
}

void loop() {
  // Do nothing after the full dump completes
}
