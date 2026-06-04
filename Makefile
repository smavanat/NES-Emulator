CC := gcc
CFLAGS := -Wall -Wextra

main: main.c
	$(CC) $(CFLAGS) main.c -o main

assembler: assembler.c
	$(CC) $(CFLAGS) assembler.c -o asm

clean:
	rm -f main
