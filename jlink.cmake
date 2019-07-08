configure_file(${CMAKE_CURRENT_LIST_DIR}/flash.in ${CMAKE_CURRENT_BINARY_DIR}/flash.jlink)

find_program(JLINK_EXE NAMES JLink JLinkExe HINTS
	${JLINK_PATH}
	[HKEY_LOCAL_MACHINE\\SOFTWARE\\SEGGER\\J-Link;InstallPath]
	[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\SEGGER\\J-Link;InstallPath]
)
find_program(JLINK_GDBSERVER JLinkGDBServer HINTS
	${JLINK_PATH}
	[HKEY_LOCAL_MACHINE\\SOFTWARE\\SEGGER\\J-Link;InstallPath]
	[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\SEGGER\\J-Link;InstallPath]
)

add_custom_command(OUTPUT ${TARGET}.bin
	COMMAND ${CMAKE_OBJCOPY} -O binary ${TARGET} ${TARGET}.bin
	DEPENDS ${TARGET}
)
add_custom_target(flash
	COMMAND ${JLINK_EXE} -device AT${DEVICE} -speed 4000 -if SWD -CommanderScript ${CMAKE_CURRENT_BINARY_DIR}/flash.jlink
	DEPENDS ${TARGET}.bin
)
add_custom_target(debug-server COMMAND ${JLINK_GDBSERVER} -device AT${DEVICE} -speed 4000 -if SWD)
