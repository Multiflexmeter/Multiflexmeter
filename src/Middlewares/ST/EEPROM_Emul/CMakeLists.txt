add_library(EEPROM_Emul STATIC
    Core/eeprom_emul.c
    Porting/STM32WL/flash_interface.c
)

target_include_directories(EEPROM_Emul PUBLIC
    Core
    Porting/STM32WL
)

target_link_libraries(EEPROM_Emul PRIVATE
    stm32cubemx
    STM32WLxx_Nucleo
)
