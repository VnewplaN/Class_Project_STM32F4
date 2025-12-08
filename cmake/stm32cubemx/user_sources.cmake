###
# user_sources.cmake
#
# Add any project-specific / user-added sources here. This file is intentionally
# separate so it won't be overwritten by STM32CubeMX code generation. Simply
# list new sources to append to ${MX_Application_Src}.
###

## Example:
# set(MX_Application_Src ${MX_Application_Src}
#     ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/my_user_file.c
# )

set(MX_Application_Src ${MX_Application_Src}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/Application/app_tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/BSP/board/bsp_board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/Drivers/Devices/temp_sensor/DS18B20.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/File_handling_RTOS.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/BSP/bsp_display/LCD_Show.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/Drivers/Devices/sd_card/fatfs_sd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/Drivers/Devices/display/fonts.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/Drivers/Devices/display/ssd1306.c
)

message(STATUS "Including user_sources.cmake: added BSP/app tasks and user drivers from Source tree")
