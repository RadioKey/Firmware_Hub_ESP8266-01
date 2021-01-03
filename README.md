# ESP8266-o1 Formware

## Flash

* [Add ESP8266 to Arduino IDE Board Manager](https://github.com/esp8266/Arduino/#installing-with-boards-manager)
* Pin numbers in Arduino correspond directly to the ESP8266 GPIO pin numbers. pinMode, digitalRead, and digitalWrite functions work as usual, so to read GPIO2, call digitalRead(2).

## Boot into flash mode

CH_PD has to be HIGH
GPIO0 has to be LOW during boot (can be HIGH once in bootloader mode)
You can provoque a reset of the card by a rising edge on the RESET pin.
