ArduinoMusicPlayer
==================

Music player for an Arduino that reads 8 bit 11 kHz WAV files from an ext2 formatted SD card

* Donovan McKelvey
* Andrew Chen

To build the program either run `make` or `make program_5`

To build the program and flash it to the SD card, put the port of the arduino (eg /dev/tty.usbmodem1451) into a file called `arduino_port`. Once you've done this, you can run `make program` to flash it to the device. The `run` script starts a screen session for the device specified by the `arduino_port` file.
