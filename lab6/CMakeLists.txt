project(lab6 LANGUAGES C CXX ASM)

# TARGET_NAME, MCU, ARDUINO_VARIANT, ARDUINO_MACRO, PROGRAMMER
add_avr_firmware(${PROJECT_NAME} "atmega328p" "standard" "ARDUINO_AVR_UNO" "arduino"
  src/main.cpp
)
