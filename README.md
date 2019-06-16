# platform-samd20
CMake library for SAMD20 Microcontrollers

# Usage
## Library
Set `DEVICE` variable to the device you are using (e.g. SAMD20E17).
Then add the library as subdirectory and link to it in your project.

## Toolchain
To cross compile with arm-none-eabi-gcc set `CMAKE_TOOLCHAIN_FILE` to `arm-none-eabi.cmake`

## JLink Flashing and Debugging
Set `TARGET` to the name of your binary and include `jlink.cmake`.
This adds the targets flash and debug-server.
