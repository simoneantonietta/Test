#ifndef _ASM_SAETPXL_H
#define _ASM_SAETPXL_H

/* etraxgpio _IOC_TYPE, bits 8 to 15 in ioctl cmd */

#define SAETPXL_IOCTYPE 45

/* supported ioctl _IOC_NR's */

#define IO_READBITS    0x1  /* read and return current port bits */
#define IO_WRITEBITS   0x2  /* write to port bits */
#define IO_RESET       0x5  /* reset emulated master module's buffer */

#endif
