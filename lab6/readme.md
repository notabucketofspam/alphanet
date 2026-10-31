# lab6

A lot of this was copy-pasted from [celery](https://github.com/notabucketofspam/celery), so im working on a way to streamline it.

computer software that you need to download and then put in your system `path`:
- [the avr-gcc toolchain](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio/gcc-compilers)
  - on the path, it should look more-or-less like `C:\opt\atmel\avr8-gnu-toolchain\bin`
  - at least, that's what the cmake project right here assumes
- [avrdude](https://github.com/avrdudes/avrdude)
  - on the path, it's `C:\opt\atmel`
  - the `windows_mingw-x64` build seems to work well enough.
- you can get to the **"Edit Environment Variables"** window super quickly with `C:\Windows\System32\rundll32.exe sysdm.cpl,EditEnvironmentVariables`
  - and you can put this into a shortcut, too.

other source files that you need to download:
- [ArduinoCore-avr](https://github.com/arduino/ArduinoCore-avr)
  - this one ends up as `C:\opt\atmel\ArduinoCore-avr`

## How do I build /flash?

**IMPORTANT**: in the specific project folder, you need to make a `port.txt` file, and this needs to have in it the port that you want to flash to.

- `CMake Targets View` in the Solution Explorer.
- click the `upload` for the project
- it'll build and flash
