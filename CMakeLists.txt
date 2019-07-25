cmake_minimum_required(VERSION 3.9)

project(platform-samd20)

set(AVAILABLE_DEVICES
	"SAMD20E14" "SAMD20E15" "SAMD20E16" "SAMD20E17" "SAMD20E18"
	"SAMD20G14" "SAMD20G15" "SAMD2GE16" "SAMD20G17" "SAMD20G18"
	"SAMD20J14" "SAMD20J15" "SAMD20J16" "SAMD20J17" "SAMD20J18"
)

if (NOT ${DEVICE} IN_LIST AVAILABLE_DEVICES)
	message(FATAL_ERROR "DEVICE must be one of ${AVAILABLE_DEVICES}")
endif()

add_library(platform-samd20 "")
target_link_libraries(platform-samd20 INTERFACE "-T\"${CMAKE_CURRENT_LIST_DIR}/samd20/gcc/gcc/${DEVICE}_flash.ld\"")
target_sources(platform-samd20 INTERFACE "${CMAKE_CURRENT_LIST_DIR}/samd20/gcc/gcc/startup_samd20.c")

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_compile_options(platform-samd20 PUBLIC -mcpu=cortex-m0plus -mthumb -ffunction-sections)
target_link_libraries(platform-samd20 PUBLIC -mcpu=cortex-m0plus -mthumb -Wl,--gc-sections --specs=nano.specs --specs=nosys.specs)

target_include_directories(platform-samd20 PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/include
	${CMAKE_CURRENT_LIST_DIR}/GSL/include
)
target_compile_definitions(platform-samd20 PUBLIC "-D__${DEVICE}__")
target_include_directories(platform-samd20 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/CMSIS/Core/Include" "${CMAKE_CURRENT_LIST_DIR}/samd20/include")

target_sources(platform-samd20 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/flash_eeprom.cpp")
