<<<<<<< HEAD
#Compile the code

program_3: program_3.c os.c synchro.c serial.c os.h globals.h synchro.h
	avr-gcc -mmcu=atmega328p -DF_CPU=16000000 -O2 -o pg3.elf program_3.c os.c serial.c synchro.c
	avr-objcopy -O ihex pg3.elf pg3.hex
	avr-size pg3.elf

#Flash the Arduino
#Be sure to change the device (the argument after -P) to match the device on your computer
#On Windows, change the argument after -P to appropriate COM port
program: pg3.hex
	avrdude -pm328p -P /dev/tty.usbmodem1411 -c arduino -F -u -U flash:w:pg3.hex

#remove build files
clean:
	rm -fr *.elf *.hex *.o
=======
CC=gcc-4.8
CFLAGS=-O3 -g

program_4: ext2.h ext2.c program4.c
	$(CC) $(CFLAGS) $^ -o ext2reader

debug: ext2.h ext2.c program4.c
	$(CC) $(CFLAGS) -DDEBUG $^ -o $@

clean:
	rm -rf ext2reader debug

>>>>>>> ba8e152ed2827ef05cfbfce0758ddfec88644207
