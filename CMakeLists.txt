# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

# PICO_BOARD should be set to 'pico_w', 'pico2_w' or not set at all.
if(DEFINED PICO_BOARD)
    message(DEBUG, "Using PICO_BOARD ${PICO_BOARD}")
else()
    message(VERBOSE, "PICO_BOARD not defined. Using 'pico_w'")
    set(PICO_BOARD "pico_w")
endif()

set(BLUEPAD32_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# To use BTstack from Pico SDK do
set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)

# If you want to use latest BTstack (included in Bluepad32 repo),
# uncomment the following lines:
# -- uncomment from here:
#set(BTSTACK_ROOT ${BLUEPAD32_ROOT}/external/btstack)
#set(PICO_BTSTACK_PATH ${BTSTACK_ROOT})
# Add missing files here. See:
# https://github.com/bluekitchen/btstack/issues/649
# https://github.com/raspberrypi/pico-sdk/issues/2142
#set(BTSTACK_MISSING_SOURCES
#    ${BTSTACK_ROOT}/src/hci_event_builder.c
#)
# -- uncomment until here
# And you need to apply this patch in Pico SDK
# https://github.com/raspberrypi/pico-sdk/pull/2157

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(arcadehub_mini_app C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

include_directories(include)


add_executable(${PROJECT_NAME}
    src/main.c
    src/Controllers/Bluetooth/Switch.c
    src/include/usb.c
    src/include/report.c
    src/include/usb_descriptors.c
    src/include/platform_selector.c
    ${BTSTACK_MISSING_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/include
    src/Controllers
    src/Controllers/Bluetooth
    src/Controllers/Bluetooth/Descriptors
    ${BLUEPAD32_ROOT}/src/components/bluepad32/include
)

# Useful for debugging
#target_compile_definitions(${PROJECT_NAME} PRIVATE
#        PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
#        WANT_HCI_DUMP=1 # This enables btstack debug
#        ENABLE_SEGGER_RTT=1
#)

# Needed for btstack_config.h / sdkconfig.h
# so that libbluepad32 can include them
include_directories(${PROJECT_NAME} src)

# Needed when using BTstack from our branch
include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/encoder/include)
include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/decoder/include)
# Need for certain IDEs, like CLion. Otherwise it won't find btstack
include_directories(${BTSTACK_ROOT}/src)

target_link_libraries(${PROJECT_NAME} PUBLIC
    pico_stdlib
    pico_cyw43_arch_none
    pico_btstack_classic
    pico_btstack_cyw43
    bluepad32    
    tinyusb_device
    tinyusb_board
    pico_multicore   
)

add_subdirectory(${BLUEPAD32_ROOT}/src/components/bluepad32 libbluepad32)

pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${PROJECT_NAME})
