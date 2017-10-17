# UF2 Bootloader for Arduino UNO R3

This projects allows flashing of Arduino UNO R3 via USB Mass Storage
(i.e., file copy or drag and drop without needed to install any software).

It implements it in firmware of the ATmega16u2 - the USB interface chip
on the Arduino UNO. This firmware in turn talks to the `optiboot` bootloader
running on the main ATmega328p chip.

The mass storage interface accepts files in [UF2](https://github.com/microsoft/uf2)
format. The new format (i.e., not `.hex` and not `.bin`) is necessary to implement
a reliable mass storage flashing in the 512 bytes of RAM of the ATmega. The UF2 repository
contains some conversion tools, and recent PXT versions have `pxt hex2uf2` command.

There's now a [blog post](https://makecode.com/blog/uf2-for-arduino-uno) up about how it 
works and how it came about.


## Building

```
$ git clone https://github.com/abcminiuser/lufa lufa
$ cd lufa/Projects
$ git clone https://github.com/mmoskal/uf2-uno
$ cd uf2-uno
$ make
```

I'm using LUFA `170418` right now. You need to have `avr-gcc` etc in `PATH`.
I'm using 4.9.

## Installing

Check out [binary releases](https://github.com/mmoskal/uf2-uno/releases).

* place the ATmega in DFU update mode by shorting two pins sticking out closest to the USB plug
* under macOS or Linux install `dfu-programer` and run `make burn`
* under Windows use [Atmel FLIP](http://www.atmel.com/tools/flip.aspx)

Arduino provides [more detailed instructions](https://www.arduino.cc/en/Hacking/DFUProgramming8U2).

Note: this bootloader is not currently compatible with Arduino IDE. If you want
that, go back to the original bootloader (also following the instructions from the Arduino
page above).

## HID

The firmware exposes a custom raw HID interface, implementing a small subset of 
[HF2 protocol](https://github.com/microsoft/uf2/blob/master/hf2.md), in particular
the serial-forwarding parts. The serial can be accessed using the C-based
`uf2tool` or using [PXT command line](https://makecode.com/cli).

On the Arduino side you have to use `Serial.init(115200);` in `setup()`,
and not any other baud rate. 

## License

MIT

This code is based on the Mass Storage example from [LUFA](http://www.fourwalledcubicle.com/LUFA.php),
has a few lines and the RingBuffer from the 
[Arduino USBSERIAL firmware](https://github.com/arduino/Arduino/tree/master/hardware/arduino/avr/firmwares/atmegaxxu2/arduino-usbserial),
and lifts some code from the [SAMD21 UF2 Bootloader](https://github.com/microsoft/uf2-samd21).
All these are MIT licensed.
