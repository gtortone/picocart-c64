cmake_minimum_required(VERSION 3.12)

set(PROJECT GOCart.64)

set(PICO_PLATFORM rp2350)

# set boards directory
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards)

include(/usr/share/pico-sdk/pico_sdk_init.cmake)

project(${PROJECT} C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# Debug
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# Initialize the SDK
pico_sdk_init()

include(/usr/share/pico-sdk/pico_sdk_init.cmake)

# fetch elehobica/pico_fatfs
include(FetchContent)

set(PICO_FATFS_DIR ${CMAKE_SOURCE_DIR}/lib/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico)

FetchContent_Declare(
    pico_fatfs
    GIT_REPOSITORY https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
    GIT_TAG main
    SOURCE_DIR ${PICO_FATFS_DIR}
)

FetchContent_MakeAvailable(pico_fatfs)

add_subdirectory(
    lib/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src
)

set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(HEADER_DIR ${CMAKE_SOURCE_DIR}/include)

file(GLOB SOURCES ${SOURCE_DIR}/*.cpp)
file(GLOB HEADERS ${HEADER_DIR}/*.h)

add_executable(${PROJECT} ${SOURCES} ${HEADERS})

#pico_generate_pio_header(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/src/ultimax.pio)

target_include_directories(${PROJECT} PRIVATE ${HEADER_DIR})

target_link_libraries(${PROJECT}
   pico_stdlib 
   pico_stdio_uart
   pico_multicore
   pico_i2c_slave
   hardware_i2c
   hardware_flash
   no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
)

# print statistics
target_link_options(${PROJECT_NAME} PRIVATE
    -Wl,--print-memory-usage
)

# enable uart output
pico_enable_stdio_usb(${PROJECT} 0)
pico_enable_stdio_uart(${PROJECT} 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT})
