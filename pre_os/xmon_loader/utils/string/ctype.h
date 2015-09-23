/*
 * Copyright (c) 2010, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CTYPE_H__
#define __CTYPE_H__

#include "string.h"

/* from:
 *  http://fxr.watson.org/fxr/source/dist/acpica/acutils.h?v=NETBSD5
 */

#define _XA     0x00    /* extra alphabetic - not supported */
#define _XS     0x40    /* extra space */
#define _BB     0x00    /* BEL, BS, etc. - not supported */
#define _CN     0x20    /* CR, FF, HT, NL, VT */
#define _DI     0x04    /* ''-'9' */
#define _LO     0x02    /* 'a'-'z' */
#define _PU     0x10    /* punctuation */
#define _SP     0x08    /* space */
#define _UP     0x01    /* 'A'-'Z' */
#define _XD     0x80    /* ''-'9', 'A'-'F', 'a'-'f' */


extern const uint8_t _ctype[];

static __inline__ __attribute__ ((always_inline)) bool isdigit(int c)
{
	return _ctype[(unsigned char)(c)] & (_DI);
}
static __inline__ __attribute__ ((always_inline)) bool isspace(int c)
{
	return _ctype[(unsigned char)(c)] & (_SP);
}
static __inline__ __attribute__ ((always_inline)) bool isxdigit(int c)
{
	return _ctype[(unsigned char)(c)] & (_XD);
}
static __inline__ __attribute__ ((always_inline)) bool isupper(int c)
{
	return _ctype[(unsigned char)(c)] & (_UP);
}
static __inline__ __attribute__ ((always_inline)) bool islower(int c)
{
	return _ctype[(unsigned char)(c)] & (_LO);
}
static __inline__ __attribute__ ((always_inline)) bool isprint(int c)
{
	return _ctype[(unsigned char)(c)] & (_LO | _UP | _DI |
					     _SP | _PU);
}
static __inline__ __attribute__ ((always_inline)) bool isalpha(int c)
{
	return _ctype[(unsigned char)(c)] & (_LO | _UP);
}


#endif	/* __CTYPE_H__ */
