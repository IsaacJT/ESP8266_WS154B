# ESP8266_WS154B #

Bare-metal library for interfacing with the Waveshare 1.54" three-colour ePaper module using an ESP8266. Communicates with the module using 4-wire SPI.

See <https://www.waveshare.com/wiki/1.54inch_e-Paper_Module_(B)> for information on the module itself.

Some code was inspired by the STM32 example code, and from the GxEPD library <https://github.com/ZinggJM/GxEPD>.

## Building ##

Run the following command to generate a static library `esp8266_ws154b.a` in build:

    make

Run the following command to generate a test program which fills the screen with a black and white pattern:

    make test

Run the following command to flash the binary to an ESP8266 board (assumes it's using `/dev/ttyUSB0`)

    make flash

Modify the Makefile as needed if your board has different requirements for flashing. The board I used has a 4MB ESP8266 module and needs `DIO` mode.
