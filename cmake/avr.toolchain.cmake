set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)
set(CMAKE_CROSSCOMPILING 1)

# Point this to the "bin" folder of your avr-gcc installation (use forward slashes)
set(AVR_TOOLCHAIN_DIR "C:/opt/atmel/avr8-gnu-toolchain/bin")

set(CMAKE_C_COMPILER   "${AVR_TOOLCHAIN_DIR}/avr-gcc.exe"     CACHE FILEPATH "C Compiler" FORCE)
set(CMAKE_CXX_COMPILER "${AVR_TOOLCHAIN_DIR}/avr-g++.exe"     CACHE FILEPATH "CXX Compiler" FORCE)
set(CMAKE_ASM_COMPILER "${AVR_TOOLCHAIN_DIR}/avr-gcc.exe"     CACHE FILEPATH "ASM Compiler" FORCE)
set(CMAKE_AR           "${AVR_TOOLCHAIN_DIR}/avr-gcc-ar.exe"  CACHE FILEPATH "Archiver" FORCE)
set(CMAKE_RANLIB       "${AVR_TOOLCHAIN_DIR}/avr-gcc-ranlib.exe" CACHE FILEPATH "Ranlib" FORCE)
set(AVR_OBJCOPY        "${AVR_TOOLCHAIN_DIR}/avr-objcopy.exe" CACHE FILEPATH "Objcopy" FORCE)
set(AVR_SIZE           "${AVR_TOOLCHAIN_DIR}/avr-size.exe"    CACHE FILEPATH "Size" FORCE)

# Stop CMake from failing the initial compiler test before -mmcu is passed
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Restrict CMake header searches to the AVR toolchain sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
