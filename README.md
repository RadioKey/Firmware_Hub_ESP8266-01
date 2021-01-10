# ESP8266-01 Firmware

[Common info about ESP8266](https://github.com/RadioKey/Documentation#esp8266)

## Boot into flash mode

* CH_PD has to be HIGH
* GPIO0 has to be LOW during boot (can be HIGH once in bootloader mode)
* You can provoque a reset of the card by a rising edge on the RESET pin.

