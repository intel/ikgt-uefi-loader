/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <mon_defs.h>
#include <string.h>
#include "error_code.h"
#include "ctype.h"

#define ULONG_MAX 0xffffffffffffffffUL


/*
 * From: FreeBSD sys/libkern/strcmp.c
 */
int strcmp(s1, s2)
register const char *s1, *s2;
{
	while (*s1 == *s2++)
		if (*s1++ == 0) {
			return 0;
		}
	return *(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1);
}

/*
 * From: FreeBSD sys/libkern/strncpy.c
 */
char *strncpy(char *__restrict dst, const char *__restrict src, size_t n)
{
	if (n != 0) {
		register char *d = dst;
		register const char *s = src;

		do {
			*d = *s++;
			if ((*d++) == 0) {
				/* NUL pad the remaining n-1 bytes */
				while (--n != 0)
					*d++ = 0;
				break;
			}
		} while (--n != 0);
	}
	return dst;
}

/*
 * From:  FreeBSD:  sys/libkern/index.c
 */
char *strchr(p, ch)
const char *p;
int ch;
{
	union {
		const char *cp;
		char *p;
	} u;

	u.cp = p;
	for (;; ++u.p) {
		if (*u.p == ch) {
			return u.p;
		}
		if (*u.p == '\0') {
			return NULL;
		}
	}
	/* NOTREACHED */
}

/*
 * From: FreeBSD sys/libkern/strncmp.c
 */
int strncmp(s1, s2, n)
register const char *s1, *s2;
register size_t n;
{
	if (n == 0) {
		return 0;
	}
	do {
		if (*s1 != *s2++) {
			return *(const unsigned char *)s1 -
			       *(const unsigned char *)(s2 - 1);
		}
		if (*s1++ == 0) {
			break;
		}
	} while (--n != 0);
	return 0;
}


/* From: @(#)strtoul.c	8.1 (Berkeley) 6/4/93 */
/* $FreeBSD: src/sys/libkern/strtoul.c,v 1.6.32.1 2010/02/10 00:26:20 kensmith Exp $ */
unsigned long strtoul(const char *nptr, char **endptr, int base)
{
	const char *s = nptr;
	unsigned long acc;
	unsigned char c;
	unsigned long cutoff;
	int neg = 0, any, cutlim;

	/*
	 * See strtol for comments as to the logic used.
	 */
	do
		c = *s++;
	while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+') {
		c = *s++;
	}
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0) {
		base = c == '0' ? 8 : 10;
	}
	cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
	cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c)) {
			c -= '0';
		} else if (isalpha(c)) {
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		} else {
			break;
		}
		if (c >= base) {
			break;
		}
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim)) {
			any = -1;
		} else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = (unsigned long)ULONG_MAX;
	} else if (neg) {
		acc = -acc;
	}
	if (endptr != 0) {
		*((const char **)endptr) = any ? s - 1 : nptr;
	}
	return acc;
}

size_t strlen(const char *str)
{
    const char *s = str;

    while (*s)
        ++s;

    return s - str;
}

