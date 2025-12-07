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
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/app_tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/bsp_board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/DS18B20.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/File_handling_RTOS.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/LCD_Show.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/fatfs_sd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/fonts.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/ssd1306.c
)

message(STATUS "Including user_sources.cmake: added BSP/app tasks and user drivers")
