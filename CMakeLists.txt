cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME stm32f4discovery)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
        os/scheduler/context_switch.s
        os/common/queue.c
        os/common/ring.c
        os/common/list.c
        os/common/linear_pool.c
        os/common/os_init.c
        os/common/sys_time.c
        os/common/sys_mutex.c
        os/scheduler/tcb_t.c
        os/scheduler/thread_scheduler.c
        os/scheduler/semaphore.c
        os/scheduler/mutex.c
        os/log/log.c
        os/task/task.c
        os/task/task_config.c
        os/task/task_manager.c
        os/task/os/idle_task.c
        os/task/os/monitor_task.c
        os/task/os/log_task.c
        os/task/os/systick_task.c
        os/task/os/os_cb_task.c
        os/driver/hal/hal_scb.c
        os/driver/hal/hal_svc.c
        os/driver/hal/hal_svc_handler.s
        os/driver/hal/hal_fault_diag.c
        os/driver/hal/hal_mpu.c
        os/driver/hal/hal_flash.c
        os/driver/hal/hal_dwt.c
        os/driver/hal/hal_nvic.c
        os/driver/hal/hal_systick.c
        os/driver/hal/hal_rng.c
        os/driver/hal/hal_crc.c
        os/driver/hal/hal_cryp.c
        os/driver/hal/hal_hash.c
        os/driver/hal/hal_dac.c
        os/driver/hal/hal_usart.c
        os/driver/hal/hal_wdg.c
        os/driver/hal/hal_timer.c
        os/driver/hal/hal_spi.c
        os/driver/hal/hal_i2c.c
        os/driver/hal/hal_adc.c
        os/driver/hal/hal_gpio.c
        os/driver/hal/hal_sdio.c
        os/driver/hal/hal_exti.c
        os/driver/hal/hal_dma.c
        os/driver/hal/hal_can.c
        os/driver/hal/hal_rtc.c
        os/driver/hal/hal_i2s.c
        os/driver/hal/hal_fsmc.c
        os/driver/hal/hal_fpu.c
        os/driver/hal/hal_rcc.c
        os/driver/common/device.c
        os/driver/device_config.c
        os/driver/common/pinmux.c
        os/driver/common/dma.c
        os/driver/common/nvic.c
        os/driver/common/rcc.c
        os/driver/common/gpio.c
        os/driver/svc.c
        os/driver/usart.c
        os/driver/adc.c
        os/driver/spi.c
        os/driver/i2c.c
        os/driver/can.c
        os/driver/sdio.c
        os/driver/systick.c
        os/driver/timer.c
        os/driver/exti.c
        os/driver/wdg.c
        os/driver/rtc.c
        os/driver/pwm.c
        os/driver/i2s.c
        os/driver/fsmc.c
        os/driver/device_manager.c
        os/test/task_test1.c
        os/test/time_task.c

)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
