## Connecting Serial to the ESP8266 (the hard way)##

### Overview ###

An example project illustrating how to connect the esp8266 to an stm32f4 development board using SPI and UART buses.

### Why do it the hard way? ###

- TODO: flush this out better 
- basically UART1 is used for programming the esp8266 so this allows you to leave the programming interface exposed and still interract with the chip
- Esp8266 setup as a SPI slave because SPI is implemented in hardware
- Esp8266 transmits to stm32f4 over uart because SPI slave transmission is unreliable 


### Stuff you'll need ###
- Keil uVision5
- esp-open-sdk
- uGfx

### Stuff I have that's optional (but useful)###
- Sublime 3 editor
- Sublime Clang plugin
- Bus Pirate
- Saleae Logic Analyzer

### Things to call out specifically ###
- It uses RTX
- I modified `SPI_USR_COMMAND_BITLEN` in `include/driver/spi_register.h` to be only 4 bits
-- TODO: explain how the spi registers work for the 8266

