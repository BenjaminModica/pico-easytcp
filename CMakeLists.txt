cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(easytcp_project C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(easytcp
easytcp.c
)

pico_enable_stdio_usb(easytcp 0)
pico_enable_stdio_uart(easytcp 1)

pico_add_extra_outputs(easytcp)

target_include_directories(easytcp PRIVATE ${CMAKE_CURRENT_LIST_DIR} )
target_link_libraries(easytcp pico_cyw43_arch_lwip_threadsafe_background pico_stdlib)