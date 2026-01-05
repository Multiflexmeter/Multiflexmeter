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
set(CMAKE_PROJECT_NAME P22296-10-SW)

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

# Add EEPROM Emulation library
add_subdirectory(Middlewares/ST/EEPROM_Emul)

# Add BSP STM32WLxx_Nucleo library
add_subdirectory(Drivers/BSP/STM32WLxx_Nucleo)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    FATFS/App/fatfs_mmc.c

    App/pingpong/app_subghz_phy.c
    App/pingpong/subghz_phy_app.c
    App/productiontestTask.c
    App/BatMon_BQ35100/BatMon_functions.c
    App/BatMon_BQ35100/bq35100.c
    App/I2CMaster/SensorRegister.c
    App/I2CMaster/SensorFunctions.c
    App/I2CMaster/I2C_Master.c
    App/CommConfig_usr.c
    App/dataflash/user_config.c
    App/dataflash/spi_driver.c
    App/dataflash/dataflash.c
    App/dataflash/standardflash.c
    App/dataflash/helper_functions.c
    App/dataflash/fusion.c
    App/dataflash/moneta.c
    App/dataflash/test_main.c
    App/dataflash/dataflash_functions.c
    App/dataflash/test.c
    App/IO_Expander/TCA9535.c
    App/IO_Expander/IO_Expander.c
    App/measurement.c
    App/common/common.c
    App/common/crc16.c
    App/common/uart.c
    App/common/softwareVersion.c
    App/IO/board_io.c
    App/IO/board_io_functions.c
    App/IO/board_io_test.c
    App/IO/led.c
    App/CommConfig.c
    App/mainTask.c
    App/FRAM/FRAM_functions.c
    App/FRAM/FRAM.c
    App/RTC_AM1805/RTC_functions.c
    App/RTC_AM1805/am1805.c
    App/productiontest/ADS7138.c
    App/productiontest/productionIO.c
    App/MFMconfiguration.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    App
    App/common
    App/dataflash
    App/FATFS/App
    App/FATFS/Target
    App/FRAM
    App/I2CMaster
    App/IO
    App/IO_Expander
    App/RTC_AM1805
    App/BatMon_BQ35100)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    EEPROM_Emul

    # Add user defined libraries
)
