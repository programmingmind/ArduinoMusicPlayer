CC=gcc-4.8
CFLAGS=-O3 -g

program_4: ext2.h ext2.c program4.c
	$(CC) $(CFLAGS) $^ -o ext2reader

debug: ext2.h ext2.c program4.c
	$(CC) $(CFLAGS) -DDEBUG $^ -o $@

clean:
	rm -rf ext2reader debug

