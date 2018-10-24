#ifndef _ASM_SAETMM_H
#define _ASM_SAETMM_H

#include <asm/ioctl.h>
#include <linux/param.h>

#define SAETMM_NUM	16

/* etraxgpio _IOC_TYPE, bits 8 to 15 in ioctl cmd */

#define SAETMM_IOCTYPE	46

/* supported ioctl _IOC_NR's */

#define MM_OVERFLOW	_IO(SAETMM_IOCTYPE, 0x1)  /* read overflow counter */
//#define MM_TEST_RD	_IO(SAETMM_IOCTYPE, 0x2)  /* TEST - write gpio register */
//#define MM_TEST_WR	_IO(SAETMM_IOCTYPE, 0x3)  /* TEST - read gpio register */
//#define MM_TEST_CONF	_IO(SAETMM_IOCTYPE, 0x4)  /* TEST - write conf gpio register */
#define MM_RESET	_IO(SAETMM_IOCTYPE, 0x5)  /* reset master module */

#endif
