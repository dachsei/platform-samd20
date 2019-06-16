set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

find_program(CMAKE_C_COMPILER arm-none-eabi-gcc HINTS
	${ARM_TOOLCHAIN_PATH}
	[HKEY_LOCAL_MACHINE\\SOFTWARE\\ARM\\GNU Tools for ARM Embedded Processors;InstallFolder]/bin
	[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\ARM\\GNU Tools for ARM Embedded Processors;InstallFolder]/bin
)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER} CACHE FILEPATH "asm compiler")
find_program(CMAKE_CXX_COMPILER arm-none-eabi-g++ HINTS
	${ARM_TOOLCHAIN_PATH}
	[HKEY_LOCAL_MACHINE\\SOFTWARE\\ARM\\GNU Tools for ARM Embedded Processors;InstallFolder]/bin
	[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\ARM\\GNU Tools for ARM Embedded Processors;InstallFolder]/bin
)

get_filename_component(ARM_TOOLCHAIN_PATH ${CMAKE_C_COMPILER} DIRECTORY)
# Without that flag CMake is not able to pass test compilation check
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_OBJCOPY ${ARM_TOOLCHAIN_PATH}/arm-none-eabi-objcopy CACHE INTERNAL "objcopy tool")
set(CMAKE_SIZE_UTIL ${ARM_TOOLCHAIN_PATH}/arm-none-eabi-size CACHE INTERNAL "size tool")
set(CMAKE_DEBUGGER ${ARM_TOOLCHAIN_PATH}/arm-none-eabi-gdb CACHE INTERNAL "debugger")

# Bug in arm-none-eabi-gcc -isystem includes are implicitly wrapped in extern C
set(CMAKE_NO_SYSTEM_FROM_IMPORTED TRUE)

set(CMAKE_FIND_ROOT_PATH ${ARM_TOOLCHAIN_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
