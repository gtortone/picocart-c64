cmake_minimum_required(VERSION 3.12)

set(PICO_PLATFORM rp2350)
set(PROJECT picocart-c64)

include(/usr/share/pico-sdk/pico_sdk_init.cmake)

project(${PROJECT} C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

# Debug
# set(CMAKE_BUILD_TYPE Debug)

# Initialize the SDK
pico_sdk_init()

include(/usr/share/pico-sdk/pico_sdk_init.cmake)

set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(HEADER_DIR ${CMAKE_SOURCE_DIR}/include)

file(GLOB SOURCES ${SOURCE_DIR}/*.cpp)
file(GLOB HEADERS ${HEADER_DIR}/*.h)

add_executable(${PROJECT} ${SOURCES} ${HEADERS})

#pico_generate_pio_header(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/c64_roml.pio)

target_include_directories(${PROJECT} PRIVATE ${HEADER_DIR})

target_link_libraries(${PROJECT}
   pico_stdlib 
   pico_stdio_uart
   pico_multicore
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

