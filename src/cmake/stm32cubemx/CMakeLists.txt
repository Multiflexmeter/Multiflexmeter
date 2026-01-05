cmake_minimum_required(VERSION 3.22)
# Enable CMake support for ASM and C languages
enable_language(C ASM)
# STM32CubeMX generated symbols (macros)
set(MX_Defines_Syms 
	CORE_CM4 
	USE_HAL_DRIVER 
	STM32WL55xx
    $<$<CONFIG:Debug>:DEBUG>
)

# STM32CubeMX generated include paths
set(MX_Include_Dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../FATFS/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/../../FATFS/App
    ${CMAKE_CURRENT_SOURCE_DIR}/../../LoRaWAN/App
    ${CMAKE_CURRENT_SOURCE_DIR}/../../LoRaWAN/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/trace/adv_trace
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/misc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/sequencer
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/timer
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/lpm/tiny_lpm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FatFs/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/LmHandler/Packages
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/Device/ST/STM32WLxx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/LmHandler
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/SubGHz_Phy
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/Include
)

# STM32CubeMX generated application sources
set(MX_Application_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../LoRaWAN/Target/radio_board_if.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../LoRaWAN/App/app_lorawan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../LoRaWAN/App/CayenneLpp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../LoRaWAN/App/lora_app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../LoRaWAN/App/lora_info.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../FATFS/Target/user_diskio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../FATFS/App/app_fatfs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/crc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/iwdg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/flash_if.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32_lpm_if.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/sys_app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/sys_debug.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/sys_sensors.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/timer_if.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/adc_if.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/usart_if.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/subghz.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32wlxx_it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32wlxx_hal_msp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/sysmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/syscalls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../startup_stm32wl55xx_cm4.s
)

# STM32 HAL/LL Drivers
set(STM32_Drivers_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/system_stm32wlxx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_adc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_ll_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_rcc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_flash_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_dma_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_pwr_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_cortex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_crc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_crc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_i2c_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_iwdg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_rtc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_spi_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_subghz.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_uart_ex.c
)

# Drivers Midllewares


set(Utilities_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/trace/adv_trace/stm32_adv_trace.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/misc/stm32_mem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/misc/stm32_systime.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/misc/stm32_tiny_sscanf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/misc/stm32_tiny_vsnprintf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/sequencer/stm32_seq.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/timer/stm32_timer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/lpm/tiny_lpm/stm32_lpm.c
)
set(LoRaWAN_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpCompliance.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpPackagesRegistration.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Crypto/cmac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Crypto/soft-se.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Crypto/lorawan_aes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionEU433.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionIN865.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionCommon.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionCN779.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionAU915.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionCN470.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionEU868.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionRU864.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionCN470B26.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionAS923.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionBaseUS.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionCN470A20.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionUS915.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionKR920.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionCN470B20.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/Region.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionCN470A26.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/LoRaMacAdr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/LoRaMacCrypto.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/LoRaMacParser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/LoRaMacConfirmQueue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/LoRaMacSerializer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/LoRaMac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/LoRaMacClassB.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Mac/LoRaMacCommands.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/LmHandler/LmHandler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/LmHandler/NvmDataMgmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LoRaWAN/Utilities/utilities.c
)
set(FatFs_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FatFs/src/diskio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FatFs/src/ff.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FatFs/src/ff_gen_drv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FatFs/src/option/syscall.c
)
set(SubGHz_Phy_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/radio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/radio_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/radio_fw.c
)

# Link directories setup
set(MX_LINK_DIRS

)
# Project static libraries
set(MX_LINK_LIBS 
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
    Utilities	LoRaWAN	FatFs	SubGHz_Phy	
)
# Interface library for includes and symbols
add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE ${MX_Include_Dirs})
target_compile_definitions(stm32cubemx INTERFACE ${MX_Defines_Syms})

# Create STM32_Drivers static library
add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE ${STM32_Drivers_Src})
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)


# Create Utilities static library
add_library(Utilities OBJECT)
target_sources(Utilities PRIVATE ${Utilities_Src})
target_link_libraries(Utilities PUBLIC stm32cubemx)

# Create LoRaWAN static library
add_library(LoRaWAN OBJECT)
target_sources(LoRaWAN PRIVATE ${LoRaWAN_Src})
target_link_libraries(LoRaWAN PUBLIC stm32cubemx)

# Create FatFs static library
add_library(FatFs OBJECT)
target_sources(FatFs PRIVATE ${FatFs_Src})
target_link_libraries(FatFs PUBLIC stm32cubemx)

# Create SubGHz_Phy static library
add_library(SubGHz_Phy OBJECT)
target_sources(SubGHz_Phy PRIVATE ${SubGHz_Phy_Src})
target_link_libraries(SubGHz_Phy PUBLIC stm32cubemx)

# Add STM32CubeMX generated application sources to the project
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${MX_Application_Src})

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${MX_LINK_DIRS})

# Add libraries to the project
target_link_libraries(${CMAKE_PROJECT_NAME} ${MX_LINK_LIBS})

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_PROJECT_NAME}.map)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()
