CC=msp430-gcc
# -Tmsp430g2433.ld
CFLAGS=-Wall -mmcu=msp430g2433 -Os
#CFLAGS=-Wall -mmcu=msp430g2433 -g

SRCS=Test.c

all: Test.txt size
#all: Test.hex size

Test.elf: $(SRCS)
	$(CC) $(CFLAGS) -Wl,--sort-common -o Test.elf $(SRCS)

size:
	msp430-size Test.elf

Test.txt: Test.elf
	msp430-objcopy -O ihex Test.elf Test.hex
#	ihex2titext.py Test.hex -o Test.txt
#	zip Test.zip Test.txt

flash:
	#mspdebug tilib -j -d /dev/ttyACM0 "prog Test.txt"
	mspdebug rf2500 "prog Test.hex"

asm:
	msp430-objdump -d Test.elf > Test.asm

