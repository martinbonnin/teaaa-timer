CC=msp430-gcc
CFLAGS=-Os -Wall -g -mmcu=msp430g2553
#CFLAGS=-O0 -Wall -g -mmcu=msp430g2452

all: main

main_OBJS=main.o

main: $(main_OBJS)
	$(CC) $(CFLAGS) -o $@.elf $(main_OBJS)

main.o: bwaaa.h mario.h note.h beep.h

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -fr main.elf *.o *.s
	
