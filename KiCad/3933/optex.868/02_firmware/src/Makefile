PREFIX = msp430-
CC = $(PREFIX)gcc
DUMP = $(PREFIX)objdump
COPY = $(PREFIX)objcopy
SIZE = $(PREFIX)size
CFLAGS += -Wall -mmcu=msp430g2433 -Os -g
#CFLAGS += -Wall -mmcu=msp430g2433 -O2 -g
LDFLAGS += -mmcu=msp430g2433 -Wl,--sort-common

all: interfaccia.hex

interfaccia.elf: interfaccia.o
	$(CC) $(LDFLAGS) $< -o $@
	$(SIZE) $@
	
interfaccia.hex: interfaccia.elf
	$(COPY) -O ihex $< $@

.PHONY: interfaccia.asm
asm: interfaccia.asm
interfaccia.asm: interfaccia.elf
	$(DUMP) -d -S $< > $@
	
flash:
	mspdebug tilib -j -d /dev/ttyACM0 "prog interfaccia.hex"

clean:
	rm -f *.o *.elf *.asm *.txt *.hex

