function(add_avr_firmware TARGET_NAME MCU ARDUINO_VARIANT ARDUINO_MACRO PROGRAMMER)
  set(PORT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/port.txt")
  if(EXISTS "${PORT_FILE}")
    file(READ "${PORT_FILE}" UPLOAD_PORT)
    string(STRIP "${UPLOAD_PORT}" UPLOAD_PORT)
  else()
    set(UPLOAD_PORT "COM3")
  endif()

  set(F_CPU "16000000UL")

  add_compile_options(
    -mmcu=${MCU} -DF_CPU=${F_CPU} -DARDUINO=10819 -D${ARDUINO_MACRO} -DARDUINO_ARCH_AVR
    -Os -Wall -ffunction-sections -fdata-sections
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++11>
    $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
    $<$<COMPILE_LANGUAGE:C>:-std=gnu11>
    $<$<COMPILE_LANGUAGE:ASM>:-x>
    $<$<COMPILE_LANGUAGE:ASM>:assembler-with-cpp>
  )

  add_link_options(-mmcu=${MCU} -Os -Wl,--gc-sections)

  file(GLOB ARDUINO_CORE_SRC
    "${ARDUINO_CORE_DIR}/cores/arduino/*.c"
    "${ARDUINO_CORE_DIR}/cores/arduino/*.cpp"
    "${ARDUINO_CORE_DIR}/cores/arduino/*.S"
  )

  add_library(${TARGET_NAME}_core OBJECT ${ARDUINO_CORE_SRC})
  target_include_directories(${TARGET_NAME}_core PUBLIC
    "${ARDUINO_CORE_DIR}/cores/arduino"
    "${ARDUINO_CORE_DIR}/variants/${ARDUINO_VARIANT}"
  )
  target_compile_options(${TARGET_NAME}_core PRIVATE -w)

  add_executable(${TARGET_NAME}.elf ${ARGN})
  target_link_libraries(${TARGET_NAME}.elf PRIVATE ${TARGET_NAME}_core m)

  add_custom_command(TARGET ${TARGET_NAME}.elf POST_BUILD
    COMMAND "${AVR_OBJCOPY}" -O ihex -R .eeprom $<TARGET_FILE:${TARGET_NAME}.elf> ${TARGET_NAME}.hex
    COMMAND "${AVR_SIZE}" -B $<TARGET_FILE:${TARGET_NAME}.elf>
    COMMENT "Generating ${TARGET_NAME}.hex and checking memory usage..."
  )

  add_custom_target(upload_${TARGET_NAME}
    COMMAND avrdude -p ${MCU} -c ${PROGRAMMER} -P ${UPLOAD_PORT} -b 115200 -D -U flash:w:${TARGET_NAME}.hex:i
    DEPENDS ${TARGET_NAME}.elf
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Flashing ${TARGET_NAME}.hex to ${MCU} on ${UPLOAD_PORT}..."
  )
endfunction()
