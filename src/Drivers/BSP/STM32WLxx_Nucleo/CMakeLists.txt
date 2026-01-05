cmake_minimum_required(VERSION 3.22)

add_library(STM32WLxx_Nucleo STATIC
    stm32wlxx_nucleo.c
    stm32wlxx_nucleo_radio.c
)

target_include_directories(STM32WLxx_Nucleo PUBLIC
    .
)

target_link_libraries(STM32WLxx_Nucleo PUBLIC
    stm32cubemx
)
