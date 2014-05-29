CCFLAGS=-mmcu=atmega328p -DF_CPU=16000000 -O3 -Wall
DUDEFLAGS=-pm328p -P `cat arduino_port` -c arduino -F -u -U flash:w:

program_5: program5.c os.c serial.c synchro.c os.h globals.h synchro.h ext2.c ext2.h
	avr-gcc $(CCFLAGS) -o $@.elf $^
	avr-objcopy -O ihex $@.elf $@.hex
	avr-size $@.elf

program: program_5
	avrdude $(DUDEFLAGS)$<.hex

clean:
	rm -rf *.elf *.hex *.o

