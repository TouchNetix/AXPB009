cmake_minimum_required(VERSION 3.20)

project(AXPB009 LANGUAGES C ASM)

set(AXPB009_TARGETS "STM32F070CB;STM32F072CB" CACHE STRING
    "Semicolon-separated firmware targets to build")

set(AXPB009_COMMON_SOURCES
    Src/_AXPB-009_Main.c
    Src/Command_Processor.c
    Src/Comms.c
    Src/Delay.c
    Src/Digitizer.c
    Src/Flash_Control.c
    Src/I2C_Comms.c
    Src/Init.c
    Src/Mode_Control.c
    Src/Peripheral_Setups/stm32f0xx_hal_msp.c
    Src/Peripheral_Setups/stm32f0xx_it.c
    Src/Peripheral_Setups/syscalls.c
    Src/Peripheral_Setups/system_stm32f0xx.c
    Src/Press_driver.c
    Src/Proxy_driver.c
    Src/SPI_comms.c
    Src/Timers_and_LEDs.c
    Src/Usage_Builder.c
    Src/USB/usb_device.c
    Src/USB/usbd_conf.c
    Src/USB/usbd_desc.c
    Src/USB/usbd_generic_if.c
    Src/USB/usbd_mouse_if.c
    Src/USB/usbd_press_if.c
    Middlewares/ST/STM32_USB_Device_Library/Class/Composite/usbd_composite.c
    Middlewares/ST/STM32_USB_Device_Library/Class/HID/HID_Generic/usbd_generic.c
    Middlewares/ST/STM32_USB_Device_Library/Class/HID/HID_Mouse/usbd_mouse.c
    Middlewares/ST/STM32_USB_Device_Library/Class/HID/HID_Press/usbd_press.c
    Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c
    Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c
    Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c
)

file(GLOB_RECURSE AXPB009_HAL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/stm32f0xx-hal-driver/Src/*.c")
list(FILTER AXPB009_HAL_SOURCES EXCLUDE REGEX "_template\\.c$")

# Keep the object order deterministic and consistent with the previous build
# script, which sorted every absolute source path before compiling and linking.
list(TRANSFORM AXPB009_COMMON_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set(AXPB009_ALL_SOURCES ${AXPB009_COMMON_SOURCES} ${AXPB009_HAL_SOURCES})
list(SORT AXPB009_ALL_SOURCES)

set(AXPB009_INCLUDE_DIRECTORIES
    Drivers/stm32f0xx-hal-driver/Inc
    Drivers/stm32f0xx-hal-driver/Inc/Legacy
    Drivers/CMSIS/Device/ST/STM32F0xx/Include
    Drivers/CMSIS/Include
    Middlewares/ST/STM32_USB_Device_Library/Class/Composite
    Middlewares/ST/STM32_USB_Device_Library/Class/HID/HID_Generic
    Middlewares/ST/STM32_USB_Device_Library/Class/HID/HID_Mouse
    Middlewares/ST/STM32_USB_Device_Library/Class/HID/HID_Press
    Middlewares/ST/STM32_USB_Device_Library/Core/Inc
    Inc
    Inc/Peripheral_Setups
    Inc/USB
)

set(AXPB009_CPU_OPTIONS
    -mcpu=cortex-m0
    -mthumb
    -mfloat-abi=soft
)

function(add_axpb009_target target device_define startup_file linker_script)
    set(output_directory "${CMAKE_BINARY_DIR}/${target}")

    add_executable(${target}
        ${AXPB009_ALL_SOURCES}
        ${startup_file}
    )

    set_target_properties(${target} PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS YES
        OUTPUT_NAME axpb009
        SUFFIX .elf
        RUNTIME_OUTPUT_DIRECTORY "${output_directory}"
    )

    target_compile_definitions(${target} PRIVATE
        USE_HAL_DRIVER
        ${device_define}
    )

    target_include_directories(${target} PRIVATE
        ${AXPB009_INCLUDE_DIRECTORIES}
    )

    target_compile_options(${target} PRIVATE
        ${AXPB009_CPU_OPTIONS}
        $<$<COMPILE_LANGUAGE:C>:--specs=nano.specs>
        $<$<COMPILE_LANGUAGE:C>:-Os>
        $<$<COMPILE_LANGUAGE:C>:-ffunction-sections>
        $<$<COMPILE_LANGUAGE:C>:-fdata-sections>
        $<$<COMPILE_LANGUAGE:C>:-fstack-usage>
        $<$<COMPILE_LANGUAGE:C>:-Wall>
    )

    target_link_options(${target} PRIVATE
        ${AXPB009_CPU_OPTIONS}
        --specs=nano.specs
        --specs=nosys.specs
        -static
        "-T${CMAKE_CURRENT_SOURCE_DIR}/${linker_script}"
        "-Wl,-Map=${output_directory}/axpb009.map"
        -Wl,--gc-sections
    )

    target_link_libraries(${target} PRIVATE
        -Wl,--start-group
        c
        m
        -Wl,--end-group
    )

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target}>
        COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:${target}>
            "${output_directory}/axpb009.hex"
        VERBATIM
    )
endfunction()

foreach(target IN LISTS AXPB009_TARGETS)
    if(target STREQUAL "STM32F042F6")
        add_axpb009_target(${target} STM32F042x6
            Startups/startup_stm32f042x6.s
            Linkers/STM32F042F6_FLASH.ld)
    elseif(target STREQUAL "STM32F070CB")
        add_axpb009_target(${target} STM32F070xB
            Startups/startup_stm32f070xb.s
            Linkers/STM32F070CB_FLASH.ld)
    elseif(target STREQUAL "STM32F072CB")
        add_axpb009_target(${target} STM32F072xB
            Startups/startup_stm32f072xb.s
            Linkers/STM32F072CB_FLASH.ld)
    else()
        message(FATAL_ERROR "Unsupported firmware target: ${target}")
    endif()
endforeach()
