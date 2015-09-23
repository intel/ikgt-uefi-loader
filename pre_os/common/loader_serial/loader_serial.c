/*******************************************************************************
* Copyright (c) 2015 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/


#include "mon_defs.h"

#define UART_REGISTER_THR 0     /* WO Transmit Holding Register */
#define UART_REGISTER_RBR 0     /* RO Receive Buffer Register */
#define UART_REGISTER_DLL 0     /* R/W Divisor Latch LSB */
#define UART_REGISTER_DLM 1     /* R/W Divisor Latch MSB */
#define UART_REGISTER_IER 1     /* R/W Interrupt Enable Register */
#define UART_REGISTER_IIR 2     /* RO Interrupt Identification Register */
#define UART_REGISTER_FCR 2     /* WO FIFO Cotrol Register */
#define UART_REGISTER_LCR 3     /* R/W Line Control Register */
#define UART_REGISTER_MCR 4     /* R/W Modem Control Register */
#define UART_REGISTER_LSR 5     /* R/W Line Status Register */
#define UART_REGISTER_MSR 6     /* R/W Modem Status Register */
#define UART_REGISTER_SCR 7     /* R/W Scratch Pad Register */

#define PACKED  __attribute((packed))

static uint16_t debug_port;

typedef struct {
	unsigned int ravie:1;
	unsigned int theie:1;
	unsigned int rie:1;
	unsigned int mie:1;
	unsigned int reserved:4;
} PACKED uart_ier_bits_t;

typedef union {
	uart_ier_bits_t bits;
	unsigned char data;
} uart_ier_t;

typedef struct {
	unsigned int trfifoe:1;
	unsigned int resetrf:1;
	unsigned int resettf:1;
	unsigned int dms:1;
	unsigned int reserved:2;
	unsigned int rtb:2;
} PACKED uart_fcr_bits_t;

typedef union {
	uart_fcr_bits_t bits;
	unsigned char data;
} uart_fcr_t;

typedef struct {
	unsigned int serialdb:2;
	unsigned int stopb:1;
	unsigned int paren:1;
	unsigned int evenpar:1;
	unsigned int sticpar:1;
	unsigned int brcon:1;
	unsigned int dlab:1;
} PACKED uart_lcr_bits_t;

typedef union {
	uart_lcr_bits_t bits;
	unsigned char data;
} uart_lcr_t;

typedef struct {
	unsigned int dtrc:1;
	unsigned int rts:1;
	unsigned int out1:1;
	unsigned int out2:1;
	unsigned int lme:1;
	unsigned int reserved:3;
} PACKED uart_mcr_bits_t;

typedef union {
	uart_mcr_bits_t bits;
	unsigned char data;
} uart_mcr_t;

typedef struct {
	unsigned int dr:1;
	unsigned int oe:1;
	unsigned int pe:1;
	unsigned int fe:1;
	unsigned int bi:1;
	unsigned int thre:1;
	unsigned int temt:1;
	unsigned int fifoe:1;
} PACKED uart_lsr_bits_t;

typedef union {
	uart_lsr_bits_t bits;
	uint8_t data;
} uart_lsr_t;

void hw_write_port_8(uint16_t port, uint8_t val8)
{
	__asm__ __volatile__ (
		"out %1, %0"
		:
		: "d" (port), "a" (val8)
		);
}

uint8_t hw_read_port_8(uint16_t port)
{
	uint8_t val8;

	__asm__ __volatile__ (
		"in %1, %0"
		: "=a" (val8)
		: "d" (port)
		);

	return val8;
}


void loader_serial_init(uint16_t io_base)
{
	uart_ier_t ier;
	uart_fcr_t fcr;
	uart_lcr_t lcr;
	uart_mcr_t mcr;

	if (!io_base) {
		return;
	}

	debug_port = io_base;

	/* mcr: reset dtr, rts, out1, out2 & loop */

	mcr.bits.dtrc = 0;
	mcr.bits.rts = 0;
	mcr.bits.out1 = 0;
	mcr.bits.out2 = 0;
	mcr.bits.lme = 0;
	mcr.bits.reserved = 0;
	hw_write_port_8(debug_port + UART_REGISTER_MCR, mcr.data);

	/* lcr: reset dlab */

	lcr.bits.serialdb = 0x03;       /* 8 data bits */
	lcr.bits.stopb = 0;             /* 1 stop bit */
	lcr.bits.paren = 0;             /* no parity */
	lcr.bits.evenpar = 0;           /* n/a */
	lcr.bits.sticpar = 0;           /* n/a */
	lcr.bits.brcon = 0;             /* no break */
	lcr.bits.dlab = 0;
	hw_write_port_8(debug_port + UART_REGISTER_LCR, lcr.data);

	/* ier: disable interrupts */

	ier.bits.ravie = 0;
	ier.bits.theie = 0;
	ier.bits.rie = 0;
	ier.bits.mie = 0;
	ier.bits.reserved = 0;
	hw_write_port_8(debug_port + UART_REGISTER_IER, ier.data);

	/* fcr: disable fifos */

	fcr.bits.trfifoe = 0;
	fcr.bits.resetrf = 0;
	fcr.bits.resettf = 0;
	fcr.bits.dms = 0;
	fcr.bits.reserved = 0;
	fcr.bits.rtb = 0;
	hw_write_port_8(debug_port + UART_REGISTER_FCR, fcr.data);

	/* scr: scratch register */

	hw_write_port_8(debug_port + UART_REGISTER_SCR, 0x00);

	/* lcr: set dlab */

	lcr.bits.dlab = 1;
	hw_write_port_8(debug_port + UART_REGISTER_LCR, lcr.data);

	/* dll & dlm: divisor value 1 for 115200 baud */

	hw_write_port_8(debug_port + UART_REGISTER_DLL, 0x01);
	hw_write_port_8(debug_port + UART_REGISTER_DLM, 0x00);

	/* lcr: reset dlab */

	lcr.bits.dlab = 0;
	hw_write_port_8(debug_port + UART_REGISTER_LCR, lcr.data);

	/* fcr: enable and reset rx & tx fifos */

	fcr.bits.trfifoe = 1;
	fcr.bits.resetrf = 1;
	fcr.bits.resettf = 1;
	hw_write_port_8(debug_port + UART_REGISTER_FCR, fcr.data);

	/* mcr: set dtr, rts */

	mcr.bits.dtrc = 1;
	mcr.bits.rts = 1;
	hw_write_port_8(debug_port + UART_REGISTER_MCR, mcr.data);
}


char                                    /* ret: character that was sent */
loader_serial_putc(char c)              /* in:  character to send */
{
	uart_lsr_t lsr;

	if (!debug_port)
		return (char)0;

	hw_write_port_8(debug_port + UART_REGISTER_THR, c);

	do
		lsr.data = hw_read_port_8(debug_port + UART_REGISTER_LSR);
	while (lsr.bits.thre != 1);

	return c;
}

void loader_serial_puts(char *s)
{
	unsigned int i;

	if (!debug_port)
		return;

	for (i = 0; s[i] != 0; i++)
		loader_serial_putc(s[i]);
}

static char convert_to_hex_char(char c)
{
	c &= 0x0f;

	if (c >= 10) {
		return (char)('a' + c - 10);
	} else {
		return (char)('0' + c);
	}
}

void loader_serial_put_hex(unsigned int hex, int need_prefix)
{
	char ch[8]; /* for 32 bit only */
	int i;
	int highest_valid_bit = 0;

	if (!debug_port)
		return;

	for (i = 0; i < 8; i++) {
		ch[i] = (char)(hex & 0xf);
		hex = hex >> 4;
		if (ch[i] != 0) {
			highest_valid_bit = i;
		}
		ch[i] = convert_to_hex_char(ch[i]);
	}

	loader_serial_puts("0x");

	if (need_prefix) {
		highest_valid_bit = 7;
	}

	for (i = highest_valid_bit; i >= 0; i--)
		loader_serial_putc(ch[i]);
}
